#pragma once

#include <mutex>
#include <vector>
#include <string>

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/ThreadSafe.h>

/**
 * @brief The SqlQuery class
 */
class SqlQuery {
private:
    enum QUERY_TYPE {SELECT, INSERT};
private:
    bool asIs_;
    std::string alias_;
    std::string query_;
    std::vector<std::string> orderAsc_;
    std::vector<std::string> orderDesc_;
    std::string groupBy_;
    std::vector<std::string> having_;
    std::vector<std::string> tables_;
    std::vector<std::string> aliases_;
    std::vector<std::string> cols_;
    std::vector<std::string> joinTables_;
    std::vector<std::vector<std::string> > joinColumns_;
    std::vector<std::string> conds_;
    int limit_, offset_;
public:
    SqlQuery();
    bool isValid() const;
    void setAsIs();
    void setAlias(const std::string &alias);
    void addHaving(const std::string &cond);
    void setGroupBy(const std::string &group);
    void addCondition(const std::string &cond);
    void addTable(const std::string &table, const std::string &alias = "");
    void addTable(SqlQuery &query, const std::string &alias = "");
    void addColumn(const std::string &col = "");

    SqlQuery orderAsc(const std::string &order) const;
    SqlQuery orderDesc(const std::string &order) const;
    SqlQuery groupBy(const std::string &group) const;
    SqlQuery unite(const SqlQuery &query) const;
    SqlQuery intersect(const SqlQuery &query) const;
    SqlQuery join(SqlQuery &query, const std::vector<std::string> &cols) const;
    SqlQuery join(const std::vector<std::string> &tables, const std::vector<std::string> &cols) const;

    SqlQuery from(const SqlQuery &query, const std::string &alias) const;
    SqlQuery from(const std::string &table, const std::string &alias = "") const;
    SqlQuery from(const std::vector<std::string> &tables) const;
    SqlQuery select(const std::vector<std::string> &cols) const;
    SqlQuery where(const std::string &col, const std::string &op, const std::string &val = "") const;
    SqlQuery where(const std::string &cond) const;
    SqlQuery hasPrefix(const std::string &col, const std::string &val) const;
    SqlQuery between(const std::string &col, const std::string &lo, const std::string &hi) const;
    SqlQuery in(const std::string &col, const SqlQuery &query) const;
    SqlQuery in(const std::string &col, const std::vector<std::string> &vals) const;
    SqlQuery in(const std::string &col, int binderSize) const;
    SqlQuery inAny(const std::string &col, const std::vector<SqlQuery> &queries) const;
    SqlQuery match(const std::string &col, int binderSize) const;
    SqlQuery having(const std::string &cond) const;
    SqlQuery offset(int numRecords) const;
    SqlQuery limit(int numRecords) const;
    std::string toString() const;
    std::string getAlias();
    // used for getting aliases for table s.t. all aliases are different
    static std::mutex lock_;
    static int counter_;
    // create table
    static std::string ct(const std::string &table, const std::vector<std::vector<std::string> > &cols);
    // create index
    static std::string ci(const std::string &table, const std::vector<std::string> &col);
    static SqlQuery q();
};
