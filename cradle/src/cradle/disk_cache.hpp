#ifndef CRADLE_DISK_CACHE_HPP
#define CRADLE_DISK_CACHE_HPP

#include <cradle/common.hpp>
#include <cradle/io/forward.hpp>
#include <vector>

namespace cradle {

// A disk cache is used for caching immutable data on the local hard drive to
// avoid redownloading it or recomputing it.

// The cache is implemented as a directory of files with an SQLite index
// database file that aids in tracking usage information.

// Note that a disk cache will generate exceptions any time an operation fails.
// Of course, since caching is by definition not essential to the correct
// operation of a program, there should always be a way to recover from these
// exceptions.

// A cache is internally protected by a mutex, so it can be used concurrently
// from multiple threads.

struct disk_cache_impl;

struct disk_cache : noncopyable
{
    disk_cache() : impl(0) {}
    ~disk_cache();

    disk_cache_impl* impl;
};

// Get the default directory path where the cache should reside for the given
// application.
file_path get_default_cache_dir(string const& app_name);

// Initialize the cache.
// key_prefix is an optional string which is prepended to all keys requested
// from the cache. It can be used to differentiate this context's data from
// another context that may be sharing the cache.
void initialize(disk_cache& cache, file_path const& dir,
    string const& key_prefix, int64_t size_limit);

// Reset the cache with new settings.
void reset(disk_cache& cache, file_path const& dir,
    string const& key_prefix, int64_t size_limit);

bool is_initialized(disk_cache const& cache);

// Get summary information about the cache.
struct disk_cache_info
{
    int64_t n_entries, total_size;
};
disk_cache_info get_summary_info(disk_cache const& cache);

// Get a list of all entries in the cache.
struct disk_cache_entry
{
    int64_t id;
    string key;
    int64_t size;
    uint32_t crc32;
};
std::vector<disk_cache_entry> get_entry_list(disk_cache const& cache);

// Remove an individual entry from the cache.
void remove_entry(disk_cache& cache, int64_t id);

// Clear the cache of all data.
void clear(disk_cache& cache);

// Check if the given key exists in the cache.
// If it does, the 64-bit ID associated with the key is written to *id.
// Also, if crc32 is not null, *crc32 receives the 32-bit CRC of the entry.
bool entry_exists(disk_cache const& cache, string const& key, int64_t* id,
    uint32_t* crc32 = 0);

// Adding an entry to the cache is a two-part process.
// First, you initiate the insert to get the ID for the entry.
// Then, once the entry is written to disk, you finish the insert.
// (If an error occurs, it's OK to simply abandon the entry, as it will be
// marked as invalid initially.)
int64_t initiate_insert(disk_cache const& cache, string const& key);
void finish_insert(disk_cache const& cache, int64_t id, uint32_t crc32);

// Given an ID within the cache, this computes the path of the file that would
// store the data associated with that ID.
file_path get_path_for_id(disk_cache const& cache, int64_t id);

// Record that an ID within the cache was just used.
// When a lot of small objects are being read from the cache, the calls to
// record_usage() can significantly slow down the loading process, as they
// require locking the database file.
// To address this, calls are buffered and sent all at once when the cache is
// idle.
void record_usage(disk_cache& cache, int64_t id);

// If you know that the cache is idle, you can call this to force the cache to
// write out its buffered usage records.
// (This is automatically called when the cache is destructed.)
void write_usage_records(disk_cache& cache);

// Another approach is to call this function periodically.
// It checks to see how long it's been since the cache was last used, and if
// the cache appears idle, it automatically writes the usage records.
void do_idle_processing(disk_cache& cache);

}

#endif