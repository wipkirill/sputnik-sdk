#pragma once

#include <string>
#include <cstdint>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/URL.h>
#include <UrbanLabs/Sdk/Utils/Properties.h>

class PrepStmt {
public:
    virtual ~PrepStmt() {;}

    // sequential binding api
    virtual bool bind(int32_t param, int times = 1) = 0;
    virtual bool bind(uint32_t param, int times = 1) = 0;
    virtual bool bind(int64_t param, int times = 1) = 0;
    virtual bool bind(uint64_t param, int times = 1) = 0;
    virtual bool bind(const std::vector<int32_t> &param, int times = 1) = 0;
    virtual bool bind(const std::vector<int64_t> &param, int times = 1) = 0;
    virtual bool bind(const std::string &param, int times = 1) = 0;
    virtual bool bind(const std::vector<std::string> &param, int times = 1) = 0;
    virtual bool bind(const uint8_t* param, size_t len, int times = 1) = 0;
    virtual bool bind(double param, int times = 1) = 0;

    virtual int32_t column_int(int col) = 0;
    virtual int64_t column_int64(int col) = 0;
    virtual std::string column_text(int col) = 0;
    virtual std::string column_blob(int col) = 0;
    virtual double column_double(int col) = 0;
    virtual int column_count() = 0;

    virtual bool step() = 0;
    virtual std::string explain() = 0;
    virtual bool reset() = 0;
    virtual bool finalize() = 0;
    virtual bool exec() = 0;
    virtual std::string version() = 0;
protected:
    // random access binding api
    // DO NOT USE this api, it is implementation dependent!
    virtual bool bind_int(int col, int32_t param, int times = 1) = 0;
    virtual bool bind_int64(int col, int64_t param, int times = 1) = 0;
    virtual bool bind_text(int col, const std::string &param, int times = 1) = 0;
    virtual bool bind_blob(int col, const uint8_t* param, size_t len, int times = 1) = 0;
    virtual bool bind_double(int col, double param, int times = 1) = 0;
};

class DbConn {
public:
    virtual ~DbConn() {;}
    virtual bool open(const URL &url, const Properties &props) = 0;
    virtual bool close() = 0;
    virtual bool prepare(const std::string &query, std::unique_ptr<PrepStmt> &stmt) = 0;
    virtual bool primaryKey(const std::string &table, std::string &pk) = 0;
    virtual bool existsTable(const std::string &table) = 0;
    virtual bool exec(const std::string &query) = 0;
    virtual bool exec(const std::vector<std::string> &queries) = 0;
    virtual bool beginTransaction() = 0;
    virtual bool commitTransaction() = 0;
};