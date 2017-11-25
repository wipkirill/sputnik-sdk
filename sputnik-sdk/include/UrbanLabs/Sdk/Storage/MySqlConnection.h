#pragma once

#include <UrbanLabs/Sdk/Utils/URL.h>
#include <UrbanLabs/Sdk/Utils/Properties.h>
#include <UrbanLabs/Sdk/Storage/DatabaseConnection.h>

class MySqlStmt : public PrepStmt {
public:
};

class MySqlConn : public DbConn {
private:
    int port;
    sqlite3 *db_;
    std::string dbName_;
    // state of the connection
    bool create_;
    bool activeTransaction_;
    // garbage collection of the prepared statements
    std::set<MySqlStmt*> stmts_;
private:
    bool notifyDestruction(MySqlStmt *stmt);
public:
    MySqlConn(const URL &url, const Properties &props);
    ~MySqlConn();
    virtual bool open();
    virtual bool close();
    virtual bool prepare(const std::string &query, std::unique_ptr<PrepStmt> &stmt);
    virtual bool exec(const std::string &str);
    virtual bool exec(const std::vector<std::string> &queries);
    virtual bool beginTransaction();
    virtual bool commitTransaction();
};
