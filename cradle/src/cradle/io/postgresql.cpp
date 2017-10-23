#include <cradle/io/postgresql.hpp>

#include <cradle/io/libpq.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>

#include <cradle/date_time.hpp>
#include <cradle/endian.hpp>

namespace cradle {

static string make_connection_string(connection_info const& info)
{
    return "host=" + info.host + " dbname=" + info.database +
        " user=" + info.user + " password=" + info.password +
        " port=" + to_string(info.port) + " connect_timeout=5";
}
void connection::initialize(connection_info const& info)
{
    assert(!conn_);

    info_ = info;

    string connection_string = make_connection_string(info);

    PGconn* conn = PQconnectdb(connection_string.c_str());
    if (PQstatus(conn) != CONNECTION_OK)
        throw connection_error(info, PQerrorMessage(conn));
    conn_ = conn;

    nested_transaction_count_ = 0;
}
connection::~connection()
{
    if (conn_)
        PQfinish(reinterpret_cast<PGconn*>(conn_));
}

transaction::transaction(connection& conn)
{
    conn_ = &conn;
    if (conn.nested_transaction_count_ == 0)
    {
        PGresult* r = PQexec(get_pgconn(conn), "begin");
        scoped_result sr(r);
        if (!r || PQresultStatus(r) != PGRES_COMMAND_OK)
            throw query_error(PQerrorMessage(get_pgconn(conn)), "begin");
    }
    ++conn.nested_transaction_count_;
}
void transaction::commit()
{
    if (conn_)
    {
        --conn_->nested_transaction_count_;
        if (conn_->nested_transaction_count_ == 0)
        {
            PGconn* conn = get_pgconn(*conn_);
            PGresult* r = PQexec(conn, "commit");
            scoped_result sr(r);
            if (!r || PQresultStatus(r) != PGRES_COMMAND_OK)
                throw query_error(PQerrorMessage(conn), "commit");
        }
        conn_ = 0;
    }
}
transaction::~transaction()
{
    if (conn_)
    {
        // If the destructor gets called without commit(), then there must've
        // been an exception, so we need to rollback the transaction.  We're
        // assuming here that if this is a nested transaction, then whatever
        // aborted it will also abort the outermost transaction, so we don't
        // bother issuing a rollback.
        --conn_->nested_transaction_count_;
        if (conn_->nested_transaction_count_ == 0)
        {
            PGresult* r = PQexec(get_pgconn(*conn_), "rollback");
            scoped_result sr(r);
        }
    }
}

void static
stream_sql_value(connection& conn, std::ostream& sql, value const& v)
{
    switch (v.type())
    {
     case value_type::BOOLEAN:
     case value_type::INTEGER:
     case value_type::FLOAT:
        sql << to_string(v);
        break;
     case value_type::STRING:
      {
        auto const& x = cast<string>(v);
        // Allocate enough space for every character to be escaped and a terminating '\0'.
        boost::scoped_array<char> buffer(new char[x.length() * 2 + 1]);
        PQescapeStringConn(get_pgconn(conn), buffer.get(), x.c_str(), x.length(), 0);
        sql << "E\'" << buffer.get() << '\'';
        break;
      }
     case value_type::DATETIME:
        sql << "'" << to_string(v) << "'";
        break;
     default:
        throw exception("unsupported SQL value type: " + to_string(v.type()));
    }
}

std::vector<value>
select_rows(connection& conn, string const& select_query)
{
    PGresult* r = PQexec(get_pgconn(conn), select_query.c_str());
    scoped_result sr(r);
    if (!r || PQresultStatus(r) != PGRES_TUPLES_OK)
    {
        throw query_error(PQerrorMessage(get_pgconn(conn)), select_query);
    }

    int n_rows = PQntuples(r);
    int n_fields = PQnfields(r);
    std::vector<value> rows;
    rows.reserve(n_rows);
    for (int row_index = 0; row_index != n_rows; ++row_index)
    {
        value_map fields;
        for (int field_index = 0; field_index != n_fields; ++field_index)
        {
            if (!PQgetisnull(r, row_index, field_index))
            {
                auto& field = fields[value(PQfname(r, field_index))];

                char const* content = PQgetvalue(r, row_index, field_index);

                switch (PQftype(r, field_index))
                {
                 case BOOLOID:
                    field.set(*content == 'f');
                    break;
                 case INT2OID:
                 case INT4OID:
                 case INT8OID:
                    field.set(boost::lexical_cast<integer>(content));
                    break;
                 case FLOAT4OID:
                 case FLOAT8OID:
                    field.set(boost::lexical_cast<double>(content));
                    break;
                 case TEXTOID:
                 case VARCHAROID:
                    field.set(content);
                    break;
                 case TIMESTAMPOID:
                  {
                    std::istringstream is(content);
                    is.imbue(
                        std::locale(
                            std::cout.getloc(),
                            new boost::posix_time::time_input_facet(
                                "%Y-%m-%d %H:%M:%S")));
                    try
                    {
                        time t;
                        is >> t;
                        if (t != time() && !is.fail() &&
                            is.peek() == std::istringstream::traits_type::eof())
                        {
                            field.set(t);
                            break;
                        }
                    }
                    catch (...)
                    {
                    }
                  }
                  {
                    std::istringstream is(content);
                    is.imbue(
                        std::locale(
                            std::cout.getloc(),
                            new boost::posix_time::time_input_facet(
                                "%Y-%m-%d %H:%M:%s")));
                    try
                    {
                        time t;
                        is >> t;
                        if (t != time() && !is.fail() &&
                            is.peek() == std::istringstream::traits_type::eof())
                        {
                            field.set(t);
                            break;
                        }
                    }
                    catch (...)
                    {
                    }
                    throw exception("unrecognized datetime format: " + string(content));
                  }
                 default:
                    throw exception("unsupported PostgreSQL field type: " +
                        to_string(PQftype(r, field_index)));
                }
            }
        }
        rows.emplace_back(fields);
    }
    return rows;
}

void
insert_row(connection& conn, string const& table, value const& row)
{
    if (row.type() != value_type::MAP)
    {
        throw exception("insert_row: row must be a MAP value");
    }
    auto const& map = cast<value_map>(row);

    std::stringstream sql;
    sql << "insert into " << table << "(";

    // Add the column names to the SQL string.
    {
        bool first = true;
        for (auto const& field : map)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                sql << ", ";
            }
            if (field.first.type() != value_type::STRING)
            {
                throw exception("insert_row: column names must be strings");
            }
            sql << cast<string>(field.first);
        }
    }

    sql << ") values(";

    // Add the column values to the SQL string.
    {
        bool first = true;
        for (auto const& field : map)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                sql << ", ";
            }
            stream_sql_value(conn, sql, field.second);
        }
    }

    sql << ");";

    PGresult* r = PQexec(get_pgconn(conn), sql.str().c_str());
    scoped_result sr(r);
    if (!r || PQresultStatus(r) != PGRES_COMMAND_OK)
    {
        throw query_error(PQerrorMessage(get_pgconn(conn)), sql.str());
    }
}

integer
insert_row_and_return_oid(
    connection& conn,
    string const& table,
    value const& row,
    string const& oid_name)
{
    if (row.type() != value_type::MAP)
    {
        throw exception("insert_row: row must be a MAP value");
    }
    auto const& map = cast<value_map>(row);

    std::stringstream sql;
    sql << "insert into " << table << "(";

    // Add the column names to the SQL string.
    {
        bool first = true;
        for (auto const& field : map)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                sql << ", ";
            }
            if (field.first.type() != value_type::STRING)
            {
                throw exception("insert_row: column names must be strings");
            }
            sql << cast<string>(field.first);
        }
    }

    sql << ") values(";

    // Add the column values to the SQL string.
    {
        bool first = true;
        for (auto const& field : map)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                sql << ", ";
            }
            stream_sql_value(conn, sql, field.second);
        }
    }

    sql << ") returning ";
    sql << oid_name;
    sql << ";";

    PGresult* r = PQexec(get_pgconn(conn), sql.str().c_str());
    scoped_result sr(r);
    if (!r || PQresultStatus(r) != PGRES_TUPLES_OK)
    {
        throw query_error(PQerrorMessage(get_pgconn(conn)), sql.str());
    }
    char const* content = PQgetvalue(r, 0, 0);
    return boost::lexical_cast<integer>(content);
}

void
update_row(connection& conn, string const& table, value const& row, string const& where)
{
    if (row.type() != value_type::MAP)
    {
        throw exception("insert_row: row must be a MAP value");
    }
    auto const& map = cast<value_map>(row);

    std::stringstream sql;
    sql << "update " << table << " set ";

    // Add the column names and values to the SQL string.
    {
        bool first = true;
        for (auto const& field : map)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                sql << ", ";
            }
            if (field.first.type() != value_type::STRING)
            {
                throw exception("insert_row: column names must be strings");
            }
            sql << cast<string>(field.first) << " = ";
            stream_sql_value(conn, sql, field.second);
        }
    }

    sql << where << ";";

    PGresult* r = PQexec(get_pgconn(conn), sql.str().c_str());
    scoped_result sr(r);
    if (!r || PQresultStatus(r) != PGRES_COMMAND_OK)
    {
        throw query_error(PQerrorMessage(get_pgconn(conn)), sql.str());
    }
}

}
