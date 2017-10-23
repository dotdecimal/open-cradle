#include <cradle/disk_cache.hpp>
#include <cradle/io/file.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sqlite3.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

#ifdef WIN32
#include <cradle/external/windows.hpp>
#include <shlobj.h>
#include <accctrl.h>
#include <aclapi.h>
#include <cradle/external/clean.hpp>
#endif

namespace cradle {

struct disk_cache_impl
{
    file_path dir;

    string key_prefix;

    sqlite3* db;

    int64_t size_limit;

    // used to track when we need to check if the cache is too big
    int64_t bytes_inserted_since_last_sweep;

    // list of IDs that whose usage needs to be recorded
    std::vector<int64_t> usage_record_buffer;

    boost::posix_time::ptime latest_activity;

    // protects all access to the cache
    boost::mutex mutex;
};

// SQLITE UTILITIES

static void open_db(sqlite3** db, file_path const& file)
{
    if (sqlite3_open(file.string().c_str(), db) != SQLITE_OK)
        throw file_error(file, "error creating disk cache index file");
}

static string escape_string(string const& s)
{
    return boost::replace_all_copy(s, "'", "''");
}

static void throw_query_error(disk_cache_impl const& cache, string const& sql,
    string const& error)
{
    throw exception(
        cache.dir.string() + ": " +
        "error executing SQL query in index.db\n" +
        "SQL query: " + sql + "\n" +
        "error: " + error);
}

static string copy_and_free_message(char* msg)
{
    if (msg)
    {
        string s = msg;
        sqlite3_free(msg);
        return s;
    }
    else
        return "";
}

static void exec_sql(disk_cache_impl const& cache, string const& sql)
{
    char *msg;
    int code = sqlite3_exec(cache.db, sql.c_str(), 0, 0, &msg);
    string error = copy_and_free_message(msg);
    if (code != SQLITE_OK)
        throw_query_error(cache, sql, error);
}

struct db_transaction
{
    db_transaction(disk_cache_impl& cache)
      : cache_(&cache), committed_(false)
    {
        exec_sql(*cache_, "begin transaction;");
    }
    void commit()
    {
        exec_sql(*cache_, "commit transaction;");
        committed_ = true;
    }
    ~db_transaction()
    {
        if (!committed_)
            exec_sql(*cache_, "rollback transaction;");
    }
    disk_cache_impl* cache_;
    bool committed_;
};

// QUERIES

// Get the total size of all entries in the cache.
struct size_result
{
    int64_t size;
    bool success;
};
static int size_callback(void* data, int n_columns, char** columns,
    char** types)
{
    if (n_columns != 1)
        return -1;

    size_result* r = reinterpret_cast<size_result*>(data);

    if (!columns[0])
    {
        // Apparently if there are no entries, it triggers this case.
        r->size = 0;
        r->success = true;
        return 0;
    }

    try
    {
        r->size = boost::lexical_cast<int64_t>(columns[0]);
        r->success = true;
        return 0;
    }
    catch (...)
    {
        return -1;
    }
}
static int64_t get_cache_size(disk_cache_impl& cache)
{
    size_result result;
    result.success = false;

    string sql = "select sum(size) from entries;";

    char* msg;
    int code = sqlite3_exec(cache.db, sql.c_str(), size_callback, &result,
        &msg);
    string error = copy_and_free_message(msg);
    if (code != SQLITE_OK)
        throw_query_error(cache, sql, error);

    if (!result.success)
        throw_query_error(cache, sql, "no result");

    return result.size;
}

// Get the total number of valid entries in the cache.
struct entry_count_result
{
    int64_t n_entries;
    bool success;
};
static int entry_count_callback(void* data, int n_columns, char** columns,
    char** types)
{
    if (n_columns != 1 || !columns[0])
        return -1;

    entry_count_result* r = reinterpret_cast<entry_count_result*>(data);

    try
    {
        r->n_entries = boost::lexical_cast<int64_t>(columns[0]);
        r->success = true;
        return 0;
    }
    catch (...)
    {
        return -1;
    }
}
static int64_t get_cache_entry_count(disk_cache_impl& cache)
{
    entry_count_result result;
    result.success = false;

    string sql = "select count(id) from entries where valid = 1;";

    char* msg;
    int code = sqlite3_exec(cache.db, sql.c_str(), entry_count_callback,
        &result, &msg);
    string error = copy_and_free_message(msg);
    if (code != SQLITE_OK)
        throw_query_error(cache, sql, error);

    if (!result.success)
        throw_query_error(cache, sql, "no result");

    return result.n_entries;
}

// Get a list of entries in the cache.
static int cache_entries_callback(void* data, int n_columns, char** columns,
    char** types)
{
    if (n_columns != 3)
        return -1;

    auto r = reinterpret_cast<std::vector<disk_cache_entry>*>(data);

    try
    {
        disk_cache_entry e;
        e.id = boost::lexical_cast<int64_t>(columns[0]);
        e.size = columns[1] ? boost::lexical_cast<int64_t>(columns[1]) : 0;
        e.crc32 = columns[2] ? boost::lexical_cast<uint32_t>(columns[2]) : 0;
        r->push_back(e);
        return 0;
    }
    catch (...)
    {
        return -1;
    }
}
static void get_entry_list(std::vector<disk_cache_entry>& entries,
    disk_cache_impl& cache)
{
    entries.clear();

    string sql =
        "select id, size, crc32 from entries where valid = 1"
        " order by last_accessed;";

    char* msg;
    int code = sqlite3_exec(cache.db, sql.c_str(), cache_entries_callback,
        &entries, &msg);
    string error = copy_and_free_message(msg);
    if (code != SQLITE_OK)
        throw_query_error(cache, sql, error);
}

// Get a list of entries in the cache in LRU order.
struct lru_entry
{
    int64_t id, size;
};
struct lru_entry_list
{
    std::vector<lru_entry> entries;
};
static int lru_entries_callback(void* data, int n_columns, char** columns,
    char** types)
{
    if (n_columns != 2)
        return -1;

    lru_entry_list* r = reinterpret_cast<lru_entry_list*>(data);

    try
    {
        lru_entry e;
        e.id = boost::lexical_cast<int64_t>(columns[0]);
        e.size = columns[1] ? boost::lexical_cast<int64_t>(columns[1]) : 0;
        r->entries.push_back(e);
        return 0;
    }
    catch (...)
    {
        return -1;
    }
}
static void get_lru_entries(lru_entry_list& entries, disk_cache_impl& cache)
{
    entries.entries.clear();

    string sql =
        "select id, size from entries order by valid, last_accessed;";

    char* msg;
    int code = sqlite3_exec(cache.db, sql.c_str(), lru_entries_callback,
        &entries, &msg);
    string error = copy_and_free_message(msg);
    if (code != SQLITE_OK)
        throw_query_error(cache, sql, error);
}

// Check if a particular key exists in the cache.
struct exists_result
{
    bool exists;
    int64_t id;
    bool valid;
    uint32_t crc32;
};
static int exists_callback(void* data, int n_columns, char** columns,
    char** types)
{
    if (n_columns != 3)
        return -1;

    exists_result* r = reinterpret_cast<exists_result*>(data);

    try
    {
        r->id = boost::lexical_cast<int64_t>(columns[0]);
        r->valid = boost::lexical_cast<int>(columns[1]) != 0;
        r->crc32 = columns[2] ? boost::lexical_cast<uint32_t>(columns[2]) : 0;
        r->exists = true;
        return 0;
    }
    catch (...)
    {
        return -1;
    }
}
static bool exists(disk_cache_impl const& cache, string const& key,
    int64_t* id, uint32_t* crc32, bool only_if_valid)
{
    string sql = "select id, valid, crc32 from entries where key='" +
        escape_string(cache.key_prefix + key) + "';";

    exists_result result;
    result.exists = false;
    char* msg;
    int code = sqlite3_exec(cache.db, sql.c_str(), exists_callback, &result,
        &msg);
    string error = copy_and_free_message(msg);
    if (code != SQLITE_OK)
        throw_query_error(cache, sql, error);

    if (result.exists && (!only_if_valid || result.valid))
    {
        *id = result.id;
        if (crc32)
            *crc32 = result.crc32;
        return true;
    }
    else
        return false;
}

// DIRECTORY STUFF

#ifdef WIN32

bool static
create_directory_with_user_full_control_acl(string const& path)
{
    LPCTSTR lpPath = path.c_str();

    if (!CreateDirectory(lpPath, NULL))
        return false;

    HANDLE hDir = CreateFile(lpPath, READ_CONTROL |WRITE_DAC, 0, NULL,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if(hDir == INVALID_HANDLE_VALUE)
        return false;

    ACL* pOldDACL;
    SECURITY_DESCRIPTOR* pSD = NULL;
    GetSecurityInfo(hDir,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL, NULL, &pOldDACL, NULL, (void**)&pSD);

    PSID pSid = NULL;
    SID_IDENTIFIER_AUTHORITY authNt = SECURITY_NT_AUTHORITY;
    AllocateAndInitializeSid(&authNt,2,SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_USERS,0,0,0,0,0,0,&pSid);

    EXPLICIT_ACCESS ea={0};
    ea.grfAccessMode = GRANT_ACCESS;
    ea.grfAccessPermissions = GENERIC_ALL;
    ea.grfInheritance = CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE;
    ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.ptstrName = (LPTSTR)pSid;

    ACL* pNewDACL = 0;
    DWORD err = SetEntriesInAcl(1,&ea,pOldDACL,&pNewDACL);

    if (pNewDACL)
        SetSecurityInfo(hDir,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL, NULL, pNewDACL, NULL);

    FreeSid(pSid);
    LocalFree(pNewDACL);
    LocalFree(pSD);
    CloseHandle(hDir);

    return true;
}

void static
create_directory_if_needed(file_path const& dir)
{
    if (!exists(dir))
        create_directory_with_user_full_control_acl(dir.string());
}

file_path get_default_cache_dir(string const& app_name)
{
    try
    {
        TCHAR path[MAX_PATH];
        if (SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE,
                NULL, 0, path) == S_OK)
        {
            file_path app_data_dir(path, boost::filesystem::native);
            file_path app_dir = app_data_dir / app_name;
            create_directory_if_needed(app_dir);
            file_path cache_dir = app_dir / "cache";
            create_directory_if_needed(cache_dir);
            return cache_dir;
        }
        else
            return file_path();
    }
    catch (...)
    {
        return file_path();
    }
}

#else // Unix-based systems

void static
create_directory_if_needed(file_path const& dir)
{
    if (!exists(dir))
        create_directory(dir);
}

file_path get_default_cache_dir(string const& app_name)
{
    file_path shared_cache_dir("/var/cache");
    create_directory_if_needed(shared_cache_dir);
    file_path this_cache_dir = shared_cache_dir / app_name;
    create_directory_if_needed(this_cache_dir);
    return this_cache_dir;
}

#endif

// OTHER UTILITIES

static file_path get_path_for_id(disk_cache_impl& cache, int64_t id)
{
    return cache.dir / boost::lexical_cast<string>(id);
}

static void remove_entry(disk_cache_impl& cache, int64_t id)
{
    file_path path = get_path_for_id(cache, id);
    if (exists(path))
        remove(path);

    string sql = "delete from entries where id=" +
        boost::lexical_cast<string>(id) + ";";

    char* msg;
    int code = sqlite3_exec(cache.db, sql.c_str(), 0, 0, &msg);
    string error = copy_and_free_message(msg);
    if (code != SQLITE_OK)
        throw_query_error(cache, sql, error);
}

void static
enforce_cache_size_limit(disk_cache_impl& cache)
{
    try
    {
        int64_t size = get_cache_size(cache);
        lru_entry_list lru_entries;
        if (size > cache.size_limit)
        {
            get_lru_entries(lru_entries, cache);
            std::vector<lru_entry>::const_iterator
                i = lru_entries.entries.begin(),
                end_i = lru_entries.entries.end();
            while (size > cache.size_limit && i != end_i)
            {
                try
                {
                    remove_entry(cache, i->id);
                    size -= i->size;
                    ++i;
                }
                catch (...)
                {
                }
            }
        }
        cache.bytes_inserted_since_last_sweep = 0;
    }
    catch (...)
    {
    }
}

static void record_activity(disk_cache_impl& cache)
{
    cache.latest_activity = boost::posix_time::microsec_clock::local_time();
}

void static
initialize(disk_cache_impl& cache, file_path const& dir,
    string const& key_prefix, int64_t size_limit)
{
    cache.db = 0;

    sqlite3_enable_shared_cache(1);

    create_directory_if_needed(dir);

    cache.dir = dir;
    cache.key_prefix = key_prefix;
    cache.size_limit = size_limit;
    cache.bytes_inserted_since_last_sweep = 0;

    open_db(&cache.db, dir / "index.db");

    exec_sql(cache,
        "create table if not exists entries(\n"
        "   id integer primary key,\n"
        "   key text unique not null,\n"
        "   valid boolean not null,\n"
        "   last_accessed datetime,\n"
        "   size integer, crc32 integer);");

    exec_sql(cache,
        "pragma synchronous = off;");

    // The disk cache database will be accessed concurrently from multiple
    // threads and even multiple processes, but each access should be very
    // short. If the database is busy when we try to access it, we want SQLite
    // to keep trying for a while before it gives up.
    sqlite3_busy_timeout(cache.db, 1000);

    record_activity(cache);

    enforce_cache_size_limit(cache);
}

void static
shut_down(disk_cache_impl& cache)
{
    if (cache.db)
    {
        sqlite3_close(cache.db);
        cache.db = 0;
    }
}

// API

disk_cache::~disk_cache()
{
    if (impl)
    {
        shut_down(*impl);
        delete impl;
        impl = 0;
    }
}

void initialize(disk_cache& cache_ref, file_path const& dir,
    string const& key_prefix, int64_t size_limit)
{
    cache_ref.impl = new disk_cache_impl;
    disk_cache_impl& cache = *cache_ref.impl;
    initialize(cache, dir, key_prefix, size_limit);
}

void reset(disk_cache& cache_ref, file_path const& dir,
    string const& key_prefix, int64_t size_limit)
{
    disk_cache_impl& cache = *cache_ref.impl;
    boost::lock_guard<boost::mutex> lock(cache.mutex);
    shut_down(cache);
    initialize(cache, dir, key_prefix, size_limit);
}

bool is_initialized(disk_cache const& cache)
{
    return cache.impl != 0;
}

disk_cache_info get_summary_info(disk_cache const& cache_ref)
{
    disk_cache_impl& cache = *cache_ref.impl;
    boost::lock_guard<boost::mutex> lock(cache.mutex);

    // Note that these are actually inconsistent since the size includes
    // invalid entries, while the entry count does not, but I think that's
    // reasonable behavior and in any case not a big deal.
    disk_cache_info info;
    info.n_entries = get_cache_entry_count(cache);
    info.total_size = get_cache_size(cache);
    return info;
}

std::vector<disk_cache_entry> get_entry_list(disk_cache const& cache_ref)
{
    disk_cache_impl& cache = *cache_ref.impl;
    boost::lock_guard<boost::mutex> lock(cache.mutex);

    std::vector<disk_cache_entry> entries;
    get_entry_list(entries, cache);
    return entries;
}

void enforce_cache_size_limit(disk_cache& cache_ref)
{
    disk_cache_impl& cache = *cache_ref.impl;
    boost::lock_guard<boost::mutex> lock(cache.mutex);

    enforce_cache_size_limit(cache);
}

void remove_entry(disk_cache& cache_ref, int64_t id)
{
    disk_cache_impl& cache = *cache_ref.impl;
    boost::lock_guard<boost::mutex> lock(cache.mutex);

    remove_entry(cache, id);
}

void clear(disk_cache& cache_ref)
{
    disk_cache_impl& cache = *cache_ref.impl;
    boost::lock_guard<boost::mutex> lock(cache.mutex);

    lru_entry_list lru_entries;
    get_lru_entries(lru_entries, cache);
    auto i = lru_entries.entries.begin(), end_i = lru_entries.entries.end();
    while (i != end_i)
    {
        try
        {
            remove_entry(cache, i->id);
            ++i;
        }
        catch (...)
        {
        }
    }
}

bool entry_exists(disk_cache const& cache_ref, string const& key,
    int64_t* id, uint32_t* crc32)
{
    disk_cache_impl& cache = *cache_ref.impl;
    boost::lock_guard<boost::mutex> lock(cache.mutex);

    record_activity(cache);

    return exists(cache, key, id, crc32, true);
}

int64_t initiate_insert(disk_cache const& cache_ref, string const& key)
{
    disk_cache_impl& cache = *cache_ref.impl;
    boost::lock_guard<boost::mutex> lock(cache.mutex);

    record_activity(cache);

    int64_t id;
    if (exists(cache, key, &id, 0, false))
        return id;

    exec_sql(cache, "insert into entries(key, valid) values ('" +
        escape_string(cache.key_prefix + key) + "', 0);");

    // Get the ID that was inserted.
    if (!exists(cache, key, &id, 0, false))
    {
        // If the insert succceeded, we really shouldn't get here.
        throw exception(cache.dir.string() + ": " +
            "failed to create entry in index.db");
    }

    return id;
}

void finish_insert(disk_cache const& cache_ref, int64_t id, uint32_t crc32)
{
    disk_cache_impl& cache = *cache_ref.impl;
    boost::lock_guard<boost::mutex> lock(cache.mutex);

    record_activity(cache);

    int64_t size = file_size(get_path_for_id(cache, id));

    exec_sql(cache, "update entries set valid=1,"
        " size=" + boost::lexical_cast<string>(size) + ","
        " crc32=" + boost::lexical_cast<string>(crc32) + ","
        " last_accessed=datetime('now')"
        " where id=" + boost::lexical_cast<string>(id) + ";");

    cache.bytes_inserted_since_last_sweep += size;
    if (cache.bytes_inserted_since_last_sweep > 0x40000000)
        enforce_cache_size_limit(cache);
}

file_path get_path_for_id(disk_cache const& cache_ref, int64_t id)
{
    disk_cache_impl& cache = *cache_ref.impl;
    // Note that this doesn't need to acquire the mutex because it only
    // accesses the directory path, which is never modified after cache
    // initialization.

    return get_path_for_id(cache, id);
}

void record_usage(disk_cache& cache_ref, int64_t id)
{
    disk_cache_impl& cache = *cache_ref.impl;
    boost::lock_guard<boost::mutex> lock(cache.mutex);

    cache.usage_record_buffer.push_back(id);
}

static void record_usage_to_db(disk_cache_impl const& cache, int64_t id)
{
    exec_sql(cache, "update entries set last_accessed=datetime('now')"
        " where id=" + boost::lexical_cast<string>(id) + ";");
}

void write_usage_records(disk_cache& cache_ref)
{
    disk_cache_impl& cache = *cache_ref.impl;
    boost::lock_guard<boost::mutex> lock(cache.mutex);

    {
        db_transaction t(cache);
        for (auto const& record : cache.usage_record_buffer)
            record_usage_to_db(cache, record);
        cache.usage_record_buffer.clear();
    }
}

void do_idle_processing(disk_cache& cache_ref)
{
    disk_cache_impl& cache = *cache_ref.impl;
    boost::lock_guard<boost::mutex> lock(cache.mutex);

    if ((boost::posix_time::microsec_clock::local_time() -
            cache.latest_activity).total_milliseconds() > 1000)
    {
        write_usage_records(cache_ref);
    }
}

}