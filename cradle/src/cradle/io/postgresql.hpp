#ifndef CRADLE_IO_POSTGRESQL_HPP
#define CRADLE_IO_POSTGRESQL_HPP

#include <cradle/common.hpp>
#include <cradle/io/forward.hpp>

namespace cradle {

struct connection_info
{
    string host, database, user, password;
    int port;
};

// a connection to a PostgreSQL database via libpq
struct connection : noncopyable
{
    connection() : conn_(0) {}
    connection(connection_info const& info) : conn_(0) { initialize(info); }
    ~connection();

    void initialize(connection_info const& info);

    // This returns a pointer to the underlying PGconn object.
    void* get() const { return conn_; }

    connection_info const& info() const { return info_; }

 private:
    connection_info info_;
    void* conn_;
    friend struct transaction;
    unsigned nested_transaction_count_;
};

struct connection_error : exception
{
    connection_error(connection_info const& info, string const& msg)
      : cradle::exception(info.host + ":" + info.database + ": " + msg)
      , info_(new connection_info(info))
    {}
    ~connection_error() throw() {}
    connection_info const& info() const { return *info_; }
 private:
    alia__shared_ptr<connection_info> info_;
};

struct query_error : exception
{
    query_error(string const& message, string const& query)
      : exception("database error: " + message + "\n" +
            "while executing: " + query)
      , message_(new std::string(message))
      , query_(new std::string(query))
    {}
    ~query_error() throw() {}
    std::string const& message() const
    { return *message_; }
    std::string const& query() const
    { return *query_; }
 private:
    alia__shared_ptr<string> message_;
    alia__shared_ptr<string> query_;
};

// thrown when the object a query is looking for is not found
struct object_not_found : query_error
{
    object_not_found(string const& query)
      : query_error("object not found", query)
    {}
    ~object_not_found() throw() {}
};

// thrown when there are duplicate objects in the database
// (This should be prevented by database constraints anyway.)
struct duplicate_objects : query_error
{
    duplicate_objects(string const& query)
      : query_error("duplicate objects", query)
    {}
    ~duplicate_objects() throw() {}
};

// a scoped object that manages a database transaction
// If the transaction object is destroyed before commit is called, the
// transaction is rolled back.
struct transaction : noncopyable
{
    transaction(connection& conn);
    void commit();
    ~transaction();
 private:
    connection* conn_;
};

// The following functions provide a method for working with database tables using
// cradle::values. Table rows are analagous to structures, with table columns treated as
// structure fields. The types of fields/columns must be primitive (i.e., booleans,
// numbers, strings or datetimes).

// Issue a 'select' query and return the result as an array of dynamic values.
std::vector<value>
select_rows(connection& conn, string const& select_query);

// Insert a dynamic value as a row in a database table.
void
insert_row(connection& conn, string const& table, value const& row);

// Insert a dynamic value as a row in a database table and return the integer value
// for the oid primary key for the row inserted.
integer
insert_row_and_return_oid(
    connection& conn,
    string const& table,
    value const& row,
    string const& oid_name);


// Update a row in a database table to be equal to the given dynamic value.
// :where is the SQL 'where' clause (including the word 'where') that identifies the
// row(s) to be updated.
void
update_row(connection& conn, string const& table, value const& row, string const& where);

}

#endif
