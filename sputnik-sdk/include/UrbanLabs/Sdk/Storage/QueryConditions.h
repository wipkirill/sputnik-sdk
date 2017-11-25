#pragma once

#include <map>
#include <tuple>
#include <string>
#include <vector>
#include <memory>
#include <UrbanLabs/Sdk/Utils/MathUtils.h>
#include <UrbanLabs/Sdk/Storage/SqlQuery.h>
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>
#include <UrbanLabs/Sdk/Storage/DatabaseConnection.h>

using namespace std;

//--------------------------------------------------------------------------------------------------
// Condition
//--------------------------------------------------------------------------------------------------
/**
 * @brief The Condition class
 */
class Condition {
public:
    // applying the condition
    virtual SqlQuery apply(const SqlQuery &query) = 0;
    virtual bool bind(std::unique_ptr<PrepStmt> &stmt) = 0;
    virtual bool isIntersectable() const;
    virtual bool prepare(std::unique_ptr<DbConn> &conn, const std::string &table);
protected:
    template<typename T>
    std::string binder(const T &t) const;
};

template<>
std::string Condition::binder<Vertex::VertexId>(const Vertex::VertexId &t) const;

template<>
std::string Condition::binder<std::string>(const string &t) const;

template<>
std::string Condition::binder<std::vector<std::string> >(const std::vector<std::string> &t) const;

template<>
std::string Condition::binder<std::vector<Vertex::VertexId> >(const std::vector<Vertex::VertexId> &t) const;

//--------------------------------------------------------------------------------------------------
// FieldsRelsConditions
//--------------------------------------------------------------------------------------------------
/**
 * @brief The FieldsRelsConditions class
 */
template<typename V>
class FieldsRelsConditions : public Condition {
private:
    std::set<std::string> validOps_;
    std::vector<std::tuple<std::string, std::string, V> > conds_;
public:
    FieldsRelsConditions(const std::vector<std::tuple<std::string, std::string, V> > &conds);
    virtual SqlQuery apply(const SqlQuery &query);
    virtual bool bind(std::unique_ptr<PrepStmt> &stmt);
private:
    int paramBind(const std::tuple<string, string, V> &cond) const;
};
/**
 * @brief FieldsRelsConditions::FieldsRelsConditions
 * @param conds
 */
template<typename V>
FieldsRelsConditions<V>::FieldsRelsConditions(const vector<tuple<string, string, V> > &conds) : conds_(conds) {
    validOps_ = {"^","<",">","=",">=","<=","IN"};
}
/**
 * @brief FieldsRelsConditions::apply
 * @param query
 * @return
 */
template<typename V>
SqlQuery FieldsRelsConditions<V>::apply(const SqlQuery &query) {
    SqlQuery res = query;
    for(int i = 0; i < conds_.size(); i++) {
        if(validOps_.count(get<1>(conds_[i]))) {
            if(get<1>(conds_[i]) == "^") {
                if(binder<V>(get<2>(conds_[i])) == "?") {
                    res = res.hasPrefix(get<0>(conds_[i]), "?");
                } else {
                    LOGG(Logger::ERROR) << "Can't bind vector with operation ^" << Logger::FLUSH;
                }
            } else {
                res = res.where(get<0>(conds_[i])+" "+get<1>(conds_[i])+" "+binder<V>(get<2>(conds_[i])));
            }
        }
    }
    return res;
}
/**
 * @brief FieldsRelsConditions::bind
 * @param stmt
 * @return
 */
template<typename V>
bool FieldsRelsConditions<V>::bind(unique_ptr<PrepStmt> &stmt) {
    for(int i = 0; i < conds_.size(); i++) {
        int numBind = paramBind(conds_[i]);
        if(numBind > 0) {
            if(!stmt->bind(get<2>(conds_[i]), numBind))
                return false;
        } else {
            LOGG(Logger::ERROR) << "Invalid condition: " << i << Logger::FLUSH;
            return false;
        }
    }
    return true;
}
/**
 * @brief paramBind
 * @param cond
 * @return
 */
template<typename V>
int FieldsRelsConditions<V>::paramBind(const std::tuple<std::string, std::string, V> &cond) const {
    if(validOps_.count(get<1>(cond))) {
        if(get<1>(cond) == "^") {
            if(binder<V>(get<2>(cond)) == "?") {
                return 2;
            } else {
                // invalid query
                return 0;
            }
        }
        else {
            return 1;
        }
    }
    // invalid operator
    return 0;
}
//--------------------------------------------------------------------------------------------------
// PrimaryKeyConditions
//--------------------------------------------------------------------------------------------------
/**
 * @brief The PrimaryKeyConditions class
 */
template<typename V>
class PrimaryKeyConditions : public Condition {
    private:
    std::set<std::string> validOps_;
    std::vector<std::tuple<std::string,std::string,V> > conds_;
public:
    PrimaryKeyConditions(const std::vector<std::tuple<std::string,std::string,V> > &conds);
    virtual SqlQuery apply(const SqlQuery &query);
    virtual bool bind(std::unique_ptr<PrepStmt> &stmt);
    virtual bool prepare(std::unique_ptr<DbConn> &conn, const std::string &table);
private:
    int paramBind(const std::tuple<string, string, V> &cond) const;
};
/**
 * @brief PrimaryKeyConditions::PrimaryKeyConditions
 * @param conds
 */
template<typename V>
PrimaryKeyConditions<V>::PrimaryKeyConditions(const vector<tuple<string,string,V> > &conds) : conds_(conds) {
    validOps_ = {"<",">","=",">=","<=","IN"};
}
/**
 * @brief PrimaryKeyConditions::apply
 * @param query
 * @return
 */
template<typename V>
SqlQuery PrimaryKeyConditions<V>::apply(const SqlQuery &query) {
    SqlQuery res = query;
    for(int i = 0; i < conds_.size(); i++) {
        if(validOps_.count(get<1>(conds_[i]))) {
            res = res.where(get<0>(conds_[i])+" "+get<1>(conds_[i])+" "+binder<V>(get<2>(conds_[i])));
        }
    }
    return res;
}
/**
 * @brief PrimaryKeyConditions::bind
 * @param stmt
 * @return
 */
template<typename V>
bool PrimaryKeyConditions<V>::bind(unique_ptr<PrepStmt> &stmt) {
    for(int i = 0; i < conds_.size(); i++) {
        int numBind = paramBind(conds_[i]);
        if(!stmt->bind(get<2>(conds_[i]), numBind))
            return false;
    }
    return true;
}
/**
 * @brief paramBind
 * @param cond
 * @return
 */
template<typename V>
int PrimaryKeyConditions<V>::paramBind(const std::tuple<std::string, std::string, V> &/*cond*/) const {
    return 1;
}
/**
 * @brief PrimaryKeyConditions::prepare
 */
template<typename V>
bool PrimaryKeyConditions<V>::prepare(unique_ptr<DbConn> &conn, const string &table) {
    string key;
    if(conn->primaryKey(table, key)) {
        auto prepared = conds_;
        prepared.clear();

        for(auto cond : conds_) {
            prepared.push_back(std::make_tuple(key, get<1>(cond), get<2>(cond)));
        }
        conds_ = prepared;
        return true;
    }
    return false;
}
//--------------------------------------------------------------------------------------------------
// Condition contaniners
//--------------------------------------------------------------------------------------------------
/**
 * @brief The ConditionContiner class
 */
class ConditionContainer {
private:
    std::vector<unique_ptr<Condition> > conds_;
public:
    void addFieldRelationships(const std::vector<std::tuple<std::string, std::string, Vertex::VertexId> > &conds);
    void addFieldRelationships(const std::vector<std::tuple<std::string, std::string, std::string> > &conds);
    void addIdsIn(const std::vector<Vertex::VertexId> &ids);
    const std::vector<unique_ptr<Condition> > &getConditions() const;
};
/**
 * @brief The ConditionContiner class
 */
class ConditionContainerFullText {
private:
    Point pt_;
    bool pointSet_;
    const static set<string> compressed_;
    map<std::string, std::vector<std::string> > tokens_;
    map<std::string, std::vector<std::string> > approxTokens_;
public:
    ConditionContainerFullText();
    // adding tokens
    void addTokensExist(const map<std::string, std::vector<std::string> > &tokens);
    void addTokensExist(const std::vector<std::string> &tokens);
    void addTokensApproxExist(const map<std::string, std::vector<std::string> > &tokens);
    void addTokensApproxExist(const std::vector<std::string> &tokens);
    // getting and applying the conditions
    std::map<string, std::vector<string> > getCompressedTokens() const;
    std::vector<std::string> getNonCompressedTokens() const;
    SqlQuery apply(const SqlQuery &base) const;
    // location queries
    std::string getLocationToken(int level) const;
    void addLocation(const Point &pt);
    bool hasLocation() const;
};
