#include <UrbanLabs/Sdk/Storage/SqlQuery.h>
#include <UrbanLabs/Sdk/Utils/Types.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>

using namespace std;

/**
 * @brief SqlQuery::counter_
 */
int SqlQuery::counter_ = 0;
std::mutex SqlQuery::lock_;
/**
 * @brief SqlQuery::SqlQuery
 */
SqlQuery::SqlQuery() :  asIs_(false), alias_(""), query_(""),
    groupBy_(""), having_(), tables_(), aliases_(), cols_(),
    joinTables_(), joinColumns_(), conds_(), limit_(0), offset_(-1)
{
    getAlias();
}

/**
 * @brief SqlQuery::setGroupBy
 * @param group
 */
void SqlQuery::setGroupBy(const string &group) {
    groupBy_ = group;
}

/**
 * @brief setHaving
 * @param cond
 */
void SqlQuery::addHaving(const string &cond) {
    having_.push_back(cond);
}

/**
 * @brief setAsIs
 */
void SqlQuery::setAsIs() {
    asIs_ = true;
}

/**
 * @brief SqlQuery::setAlias
 */
void SqlQuery::setAlias(const string &alias) {
    alias_ = alias;
}
/**
 * @brief orderBy
 * @param order
 * @return
 */
SqlQuery SqlQuery::orderAsc(const string &order) const {
    SqlQuery res = *this;
    res.orderAsc_.push_back(order);
    return res;
}
/**
 * @brief orderBy
 * @param order
 * @return
 */
SqlQuery SqlQuery::orderDesc(const string &order) const {
    SqlQuery res = *this;
    res.orderDesc_.push_back(order);
    return res;
}
/**
 * @brief groupBy
 * @param group
 * @return
 */
SqlQuery SqlQuery::groupBy(const string &group) const {
    SqlQuery res = *this;
    res.setGroupBy(group);
    return res;
}

/**
 * @brief SqlQuery::addCondition
 * @param cond
 */
void SqlQuery::addCondition(const string &cond) {
    conds_.push_back(cond);
}

/**
 * @brief addTable
 * @param table
 */
void SqlQuery::addTable(SqlQuery &query, const string &alias) {
    tables_.push_back("("+query.toString()+")");
    if(alias != "")
        aliases_.push_back(alias);
    else
        aliases_.push_back(query.getAlias());
}

/**
 * @brief addTable
 * @param query
 */
void SqlQuery::addTable(const string &table, const string &alias) {
    tables_.push_back(table);
    aliases_.push_back(alias);
}
/**
 * @brief SqlQuery::addColumn
 * @param query
 * @param col
 */
void SqlQuery::addColumn(const string &col) {
    cols_.push_back(col);}

/**
 * checks if at least one column is chosen from at least one table
 * or if a table code should be pasted verbatim
 * @brief isValid
 * @return
 */
bool SqlQuery::isValid() const {
    return (tables_.size() > 0 && cols_.size() > 0 && conds_.size() > 0) || asIs_;
}

/**
 * @brief SqlQuery::query
 * @return
 */
SqlQuery SqlQuery::q() {
    SqlQuery query;
    return query;
}
/**
 * @brief SqlQuery::ci
 * @return
 */
string SqlQuery::ci(const string &table, const vector<string> &col) {
    string name = table;
    for(int i = 0; i < col.size(); i++)
        name.append("_"+col[i]);

    string line = "";
    for(int i = 0; i < col.size(); i++) {
        line.append(col[i]);
        if(i+1 < col.size())
            line.append(",");
    }
    string query = "CREATE INDEX IF NOT EXISTS "+name+"_index ON "+table+"("+line+")";
    query.append(";");
    return query;
}
/**
 * @brief SqlQuery::ci
 * @param table
 * @param col
 * @return
 */
string SqlQuery::ct(const string &table, const vector<vector<string> > &cols) {
    string query = "CREATE TABLE "+table+" (";
    if(cols.size() == 0 || table.size())
        return "";
    for(int i = 0; i < cols.size(); i++) {
        if(cols[i].size() != 2) {
            return "";
        }

        query.append(cols[i][0]);
        query.append(" ");
        query.append(cols[i][1]);

        if(i+1 < cols.size()) {
            query.append(",");
        }
    }
    query.append(");");
    return query;
}
/**
 * @brief SqlQuery::from
 * @param table
 * @return
 */
SqlQuery SqlQuery::from(const string &table, const string &alias) const {
    SqlQuery res = *this;
    res.addTable(table, alias);
    return res;
}
/**
 * @brief SqlQuery::from
 * @param table
 * @param alias
 * @return
 */
SqlQuery SqlQuery::from(const SqlQuery &query, const string &alias) const {
    SqlQuery res = *this;
    res.addTable("("+query.toString()+")", alias);
    return res;
}
/**
 * @brief SqlQuery::from
 * @param tables
 * @return
 */
SqlQuery SqlQuery::from(const vector<string> &tables) const {
    SqlQuery res = *this;
    for(size_t i = 0; i < tables.size(); i++)
        res.addTable(tables[i]);
    return res;
}
/**
 * @brief SqlQuery::select
 * @param cols
 * @return
 */
SqlQuery SqlQuery::select(const vector<string> &cols) const {
    SqlQuery res = *this;
    for(size_t i = 0; i < cols.size(); i++)
        res.addColumn(cols[i]);
    return res;
}

/**
 * @brief SqlQuery::where
 * @param col
 * @param op
 * @param val
 * @return
 */
SqlQuery SqlQuery::where(const string &col, const string &op, const string &val) const {
    SqlQuery res = *this;
    string condition = col+" "+op+" "+val;
    res.addCondition(condition);
    return res;
}

/**
 * @brief SqlQuery::where
 * @param cond
 * @return
 */
SqlQuery SqlQuery::where(const string &cond) const {
    SqlQuery res = *this;
    res.addCondition(cond);
    return res;
}

/**
 * @brief SqlQuery::hasPrefix
 * @param col
 * @param val
 * @return
 */
SqlQuery SqlQuery::hasPrefix(const string &col, const string &val) const {
    SqlQuery res = *this;
    return res.between(col, "'' || "+val+" || ''", "'' || "+val+" || '😎😎😎😎'");
}
/**
 * @brief SqlQuery::between
 * @param col
 * @param lo
 * @param hi
 * @return
 */
SqlQuery SqlQuery::between(const string &col, const string &lo, const string &hi) const {
    SqlQuery res = *this;
    string condition = col+" BETWEEN "+lo+" AND "+hi;
    res.addCondition(condition);
    return res;
}
/**
 * @brief SqlQuery::limit
 * @param numRecords
 * @return
 */
SqlQuery SqlQuery::limit(int numRecords) const {
    if(numRecords < 0) {
        LOGG(Logger::WARNING) << "Negative limit" << Logger::FLUSH;
        return *this;
    }

    SqlQuery res = *this;
    if(asIs_) {
        if(res.tables_.size() == 1)
            res.tables_[0].append(" LIMIT "+lexical_cast(numRecords));
        else
            LOGG(Logger::WARNING) << "Cannot limit the output of the query, too many as is tables" << Logger::FLUSH;
    }
    if(!asIs_)
        res.limit_ = numRecords;
    return res;
}
/**
 * @brief SqlQuery::limit
 * @param numRecords
 * @return
 */
SqlQuery SqlQuery::offset(int numRecords) const {
    if(numRecords < 0) {
        LOGG(Logger::WARNING) << "Negative offset" << Logger::FLUSH;
        return *this;
    }

    SqlQuery res = *this;
    if(asIs_) {
        if(res.tables_.size() == 1)
            res.tables_[0].append(" OFFSET "+lexical_cast(numRecords));
        else
            LOGG(Logger::WARNING) << "Cannot offset the output of the query, too many as is tables" << Logger::FLUSH;
    }
    if(!asIs_)
        res.offset_ = numRecords;
    return res;
}
/**
 * @brief SqlQuery::in
 * @param col
 * @param query
 * @return
 */
SqlQuery SqlQuery::in(const string &col, const SqlQuery &query) const {
    SqlQuery res = *this;
    string condition = col+" IN ("+query.toString()+ ")";
    if(query.isValid())
        res.addCondition(condition);
    else
        LOGG(Logger::WARNING) << "Invalid query: " << query.toString() << Logger::FLUSH;
    return res;
}

/**
 * @brief SqlQuery::in
 * @param col
 * @param vals
 * @return
 */
SqlQuery SqlQuery::in(const string &col, const vector<string> &vals) const {
    LOGG(Logger::ERROR) << "Unsafe sql operation, use bind instead" << Logger::FLUSH;

    SqlQuery res = *this;
    string arr = "(";
    for(size_t i = 0; i < vals.size(); i++) {
        arr.append("\'"+vals[i]+"\'");
        if(i+1 != vals.size()) {
            arr.append(",");
        }
    }
    arr.append(")");
    string condition = col+" IN "+arr;
    if(vals.size() > 0)
        res.addCondition(condition);
    else
        LOGG(Logger::WARNING) << "Empty binder size" << Logger::FLUSH;
    return res;
}

/**
 * @brief SqlQuery::in
 * @param col
 * @param vals
 * @return
 */
SqlQuery SqlQuery::in(const string &col, int binderSize) const {
    SqlQuery res = *this;
    string arr = "(";
    for(int i = 0; i < binderSize; i++) {
        arr.append("?");
        if(i+1 != binderSize) {
            arr.append(",");
        }
    }
    arr.append(")");
    string condition = col+" IN "+arr;
    if(binderSize > 0)
        res.addCondition(condition);
    else {
        LOGG(Logger::WARNING) << "Zero binder size" << Logger::FLUSH;
        return *this;
    }
    return res;
}
/**
 * @brief in
 * @param col
 * @param queries
 * @return
 */
SqlQuery SqlQuery::inAny(const string &col, const vector<SqlQuery> &queries) const {
    string cond = "";
    for(int i = 0; i < queries.size(); i++) {
        cond.append(col+" IN ("+queries[i].toString()+")");
        if(i+1 < queries.size())
            cond.append(" OR ");
    }

    SqlQuery res = *this;
    res.addCondition("("+cond+")");
    return res;
}
/**
 * @brief match
 * @param col
 * @param binderSize
 * @return
 */
SqlQuery SqlQuery::match(const string &col, int binderSize) const {
    SqlQuery res = *this;
    if(binderSize > 0) {
        string arr = "?";
        for(int i = 1; i < binderSize; i++) {
            arr.append(" || \" \" || ?");
        }
        string condition = col+" MATCH "+arr;
        res.addCondition(condition);
    } else
        LOGG(Logger::WARNING) << "Zero binder size" << Logger::FLUSH;
    return res;
}
/**
 * @brief SqlQuery::toString
 * @return
 */
string SqlQuery::toString() const {
    if(asIs_) {
        if(tables_.size() == 0) {
            LOGG(Logger::WARNING) << "no table present" << Logger::FLUSH;
            return "";
        } else if(tables_.size() > 1) {
            LOGG(Logger::WARNING) << "too many tables specified" << Logger::FLUSH;
            return "";
        }
        return tables_[0];
    }

    string res = "SELECT ";
    if(cols_.size() == 0) {
        LOGG(Logger::WARNING) << "no columns specified" << Logger::FLUSH;
    }

    for(size_t i = 0; i < cols_.size(); i++) {
        res.append(cols_[i]);
        if(i+1 != cols_.size()) {
            res.append(",");
        }
    }

    if(tables_.size() == 0) {
        LOGG(Logger::WARNING) << "no tables specified" << Logger::FLUSH;
        return res;
    }

    res.append(" FROM ");
    if(tables_.size() != aliases_.size()) {
        LOGG(Logger::WARNING) << "not enough aliases" << Logger::FLUSH;
    }
    for(size_t i = 0; i < tables_.size(); i++) {
        res.append(tables_[i]);
        if(aliases_[i] != "") {
            res.append(" AS "+aliases_[i]);
        }
        if(i+1 != tables_.size()) {
            res.append(",");
        }
    }

    if(conds_.size() == 0) {
        LOGG(Logger::WARNING) << "no conditions specified" << Logger::FLUSH;
    }

    if(conds_.size() != 0) {
        res.append(" WHERE ");
        for(int i = 0; i < conds_.size(); i++) {
            res.append(conds_[i]);
            if(i+1 != conds_.size()) {
                res.append(" AND ");
            }
        }
    }

    if(groupBy_ != "") {
        res.append(" GROUP BY "+groupBy_);
    }

    if(having_.size() != 0) {
        res.append(" HAVING ");
        for(size_t i = 0; i < having_.size(); i++) {
            res.append(having_[i]);
            if(i+1 != having_.size()) {
                res.append(" AND ");
            }
        }
    }

    if(orderAsc_.size() != 0 || orderDesc_.size() != 0) {
        res.append(" ORDER BY ");
        for(int i = 0; i < orderAsc_.size(); i++) {
            res.append(orderAsc_[i]+" ASC");
            if(i+1 < orderAsc_.size() || orderDesc_.size() > 0)
                res.append(",");
        }
        for(int i = 0; i < orderDesc_.size(); i++) {
            res.append(orderDesc_[i]+" DESC");
            if(i+1 < orderDesc_.size())
                res.append(",");
        }
        res.append(" ");
    }

    if (limit_ > 0) {
        res.append(" LIMIT " + lexical_cast(limit_)+" ");
    }

    if(offset_ != -1) {
        res.append(" OFFSET " + lexical_cast(offset_)+" ");
    }

    return res;
}

/**
 * @brief SqlQuery::unite
 * @param query
 * @return
 */
SqlQuery SqlQuery::unite(const SqlQuery &query) const {
    // TODO set alias for the table and set the query field
    SqlQuery res;
    if(isValid() && query.isValid()) {
        res.setAsIs();
        res.addTable(toString()+ " UNION "+query.toString());
    } else if(query.isValid()) {
        res = query;
    } else {
        res = *this;
    }
    return res;
}

/**
 * @brief SqlQuery::intersect
 * @param query
 * @return
 */
SqlQuery SqlQuery::intersect(const SqlQuery &query) const {
    // TODO set alias for the table and set the query field
    SqlQuery res;
    if(isValid() && query.isValid()) {
        res.setAsIs();
        res.addTable(toString()+" INTERSECT "+query.toString());
    } else if(query.isValid()) {
        res = query;
    } else {
        res = *this;
    }
    return res;
}

/**
 * @brief SqlQuery::join
 * @param query
 * @param cols
 * @return
 */
SqlQuery SqlQuery::join(SqlQuery &query, const vector<string> &cols) const {
    SqlQuery res = *this;
    if(query.isValid()) {
        string alias = query.getAlias();
        for(size_t i = 0; i < cols.size(); i++) {
            res = res.where(cols[i], "=", alias+"."+cols[i]);
        }
    }
    return res;
}
/**
 * @brief SqlQuery::join
 * @param table
 * @param cols
 * @return
 */
SqlQuery SqlQuery::join(const vector<string> &tables, const vector<string> &cols) const {
    SqlQuery res = *this;
    for(size_t i = 0; i < cols.size(); i++) {
        for(size_t j = 0; j < tables.size(); j++)
            if(j > 0)
                res = res.where(tables[j-1]+"."+cols[i], "=", tables[j]+"."+cols[i]);
    }
    return res;
}
/**
 * @brief having
 * @param cond
 * @return
 */
SqlQuery SqlQuery::having(const string &cond) const {
    SqlQuery res = *this;
    res.addHaving(cond);
    return res;
}
/**
 * @brief SqlQuery::getAlias
 * @return
 */
string SqlQuery::getAlias() {
    if(alias_ == "") {
        std::lock_guard<std::mutex> guard(lock_);
        int cnt = counter_;
        alias_ = "";
        while(cnt > 0) {
            char next = (char)('A'+(cnt % 26));
            alias_ = alias_.append(1, next);
            cnt /= 26;
        }
        counter_++;
        return alias_;
    } else {
        return alias_;
    }
}