#pragma once

#include <set>
#include <mutex>
#include <memory>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/URL.h>
#include <UrbanLabs/Sdk/Utils/Properties.h>
#include <UrbanLabs/Sdk/Storage/DatabaseConnection.h>

// do not change the order here, ANDROID should come before __linux
#ifdef _WIN64
//define something for Windows (64-bit)
#elif _WIN32
//define something for Windows (32-bit)
#elif __APPLE__
#include "TargetConditionals.h"
#include <sqlite3.h>
#ifdef TARGET_OS_IPHONE
// iOS
#elif TARGET_IPHONE_SIMULATOR
// iOS Simulator
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
// Unsupported platform
#endif
#elif ANDROID
#include <sqlite3.h>
#elif __linux
#include <sqlite3.h>
#elif __unix // all unices not caught above
// Unix
#elif __posix
// POSIX
#endif
class SqliteConn;

class SqliteStmt : public PrepStmt {
private:
    int offset_;
    bool finalized_;
    std::string query_;
    SqliteConn* conn_;
    sqlite3_stmt* stmt_;
public:
    SqliteStmt(const std::string &query, sqlite3_stmt* raw, SqliteConn* conn_);
    virtual ~SqliteStmt();

    // sequential binding api
    virtual bool bind(int32_t param, int times = 1);
    virtual bool bind(uint32_t param, int times = 1);
    virtual bool bind(int64_t param, int times = 1);
    virtual bool bind(uint64_t param, int times = 1);
    virtual bool bind(const std::vector<int32_t> &param, int times = 1);
    virtual bool bind(const std::vector<int64_t> &param, int times = 1);
    virtual bool bind(const std::string &param, int times = 1);
    virtual bool bind(const std::vector<std::string> &param, int times = 1);
    virtual bool bind(const uint8_t* param, size_t len, int times = 1);
    virtual bool bind(double param, int times = 1);

    virtual int32_t column_int(int col);
    virtual int64_t column_int64(int col);
    virtual std::string column_text(int col);
    virtual std::string column_blob(int col);
    virtual double column_double(int col);
    virtual int column_count();

    virtual bool reset();
    virtual std::string explain();
    virtual bool finalize();
    virtual bool step();
    virtual bool exec();
    virtual std::string version();
public:
    friend class SqliteConn;
    friend class DbConn;
protected:
    // random access binding api
    // DO NOT USE this api, it is implementation dependent!
    virtual bool bind_int(int col, int32_t param, int times = 1);
    virtual bool bind_int64(int col, int64_t param, int times = 1);
    virtual bool bind_text(int col, const std::string &param, int times = 1);
    virtual bool bind_blob(int col, const uint8_t* param, size_t len, int times = 1);
    virtual bool bind_double(int col, double param, int times = 1);
};

// TODO
struct SqliteStmtDestructor {
    void operator() (SqliteStmt* stmt) {
        if(stmt) {
            ;
        }
    }
};

class SqliteConn : public DbConn {
private:
    sqlite3* db_;
    std::string dbName_;
    std::mutex lock_;
    // state of the connection
    bool create_;
    bool inmemory_;
    bool readwrite_;
    bool activeTransaction_;
    // garbage collection of the prepared statements
    std::set<SqliteStmt*> stmts_;
private:
    // these are supposed to be used by friends only
    bool notifyDestruction(SqliteStmt *stmt);
    sqlite3 *getDB();
public:
    SqliteConn();
    virtual ~SqliteConn();
    virtual bool open(const URL &url, const Properties &props);
    virtual bool close();
    virtual bool prepare(const std::string &query, std::unique_ptr<PrepStmt> &stmt);
    virtual bool exec(const std::string &str);
    virtual bool exec(const std::vector<std::string> &str);
    virtual bool existsTable(const std::string &table);
    virtual bool primaryKey(const std::string &table, std::string &pk);
    virtual bool beginTransaction();
    virtual bool commitTransaction();
public:
    friend class SqliteStmt;
    friend class PrepStmt;
};
