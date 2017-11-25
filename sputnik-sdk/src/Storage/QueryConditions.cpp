#include <UrbanLabs/Sdk/Storage/SqlConsts.h>
#include <UrbanLabs/Sdk/Storage/QueryConditions.h>

using namespace std;

//--------------------------------------------------------------------------------------------------
// Condition
//--------------------------------------------------------------------------------------------------
/**
 * @brief Condition::binder<string>
 * @param t
 * @return
 */
template<>
string Condition::binder<Vertex::VertexId>(const Vertex::VertexId &/*t*/) const {
    return "?";
}
/**
 * @brief Condition::binder<string>
 * @param t
 * @return
 */
template<>
string Condition::binder<string>(const string &/*t*/) const {
    return "?";
}
/**
 * @brief Condition::binder<vector<string> >
 * @param t
 * @return
 */
template<>
string Condition::binder<vector<string> >(const vector<string> &t) const {
    if(t.size() == 0)
        return "";
    else {
        string res = "(";
        for(int i = 0; i < t.size(); i++) {
            res.append("?");
            if(i+1 < t.size())
                res.append(",");
        }
        res.append(")");
        return res;
    }
}
/**
 * @brief Condition::binder<vector<Vertex::VertexId> >
 * @param t
 * @return
 */
template<>
string Condition::binder<vector<Vertex::VertexId> >(const vector<Vertex::VertexId> &t) const {
    if(t.size() == 0)
        return "";
    else {
        string res = "(";
        for(int i = 0; i < t.size(); i++) {
            res.append("?");
            if(i+1 < t.size())
                res.append(",");
        }
        res.append(")");
        return res;
    }
}
/**
 * @brief Condition::prepare
 */
bool Condition::prepare(std::unique_ptr<DbConn> &/*conn*/, const std::string &/*table*/) {
    return true;
}
/**
 * @brief Condition::isIntersectable
 * @return
 */
bool Condition::isIntersectable() const {
    return false;
}
//--------------------------------------------------------------------------------------------------
// ConditionContainer
//--------------------------------------------------------------------------------------------------
/**
 * @brief ConditionContainer::addFieldRelationships
 * @param conds
 */
void ConditionContainer::addFieldRelationships(const std::vector<std::tuple<std::string, std::string, Vertex::VertexId> > &conds) {
    conds_.push_back(unique_ptr<Condition>(new FieldsRelsConditions<Vertex::VertexId>(conds)));
}
/**
 * @brief ConditionContainer::addFieldRelationships
 * @param conds
 */
void ConditionContainer::addFieldRelationships(const std::vector<std::tuple<std::string, std::string,std::string> > &conds) {
    conds_.push_back(unique_ptr<Condition>(new FieldsRelsConditions<std::string>(conds)));
}
/**
 * @brief ConditionContainer::addIdsIn
 * @param ids
 */
void ConditionContainer::addIdsIn(const vector<Vertex::VertexId> &ids) {
    auto cond = new PrimaryKeyConditions<vector<Vertex::VertexId> >({make_tuple("?", "IN", ids)});
    conds_.push_back(unique_ptr<Condition>(cond));
}
/**
 * @brief ConditionContainer::getOther
 * @return
 */
const std::vector<unique_ptr<Condition> > &ConditionContainer::getConditions() const {
    return conds_;
}
//--------------------------------------------------------------------------------------------------
// ConditionContainerFulltext
//--------------------------------------------------------------------------------------------------
const set<string> ConditionContainerFullText::compressed_ = {"tokens"};
/**
 * @brief ConditionContainerFullText::ConditionContainerFullText
 */
ConditionContainerFullText::ConditionContainerFullText() : pointSet_(false) {
    ;
}
/**
 * @brief addTokensExist
 * @param tokens
 */
void ConditionContainerFullText::addTokensExist(const map<std::string, std::vector<std::string> > &tokens) {
    for(auto entry : tokens) {
        auto it = tokens_.find(entry.first);
        if(it == tokens_.end()) {
            tokens_[entry.first] = entry.second;
        } else {
            for(int i = 0; i < entry.second.size(); i++) {
                it->second.push_back(entry.second[i]);
            }
        }
    }
}
/**
 * @brief ConditionContainerFullText::addTokensExist
 * @param tokens
 */
void ConditionContainerFullText::addTokensExist(const std::vector<std::string> &tokens) {
    auto it = tokens_.find("tokens");
    if(it == tokens_.end()) {
        tokens_["tokens"] = tokens;
    } else {
        for(int i = 0; i < tokens.size(); i++) {
            it->second.push_back(tokens[i]);
        }
    }
}
/**
 * @brief addTokensApproxExist
 * @param tokens
 */
void ConditionContainerFullText::addTokensApproxExist(const map<std::string, std::vector<std::string> > &tokens) {
    for(auto entry : tokens) {
        auto it = approxTokens_.find(entry.first);
        if(it == approxTokens_.end()) {
            approxTokens_[entry.first] = vector<string>();
            for(int i = 0; i < entry.second.size(); i++) {
                approxTokens_.find(entry.first)->second.push_back(entry.second[i]+"*");
            }
        } else {
            for(int i = 0; i < entry.second.size(); i++) {
                it->second.push_back(entry.second[i]+"*");
            }
        }
    }
}
/**
 * @brief ConditionContainerFullText::addTokensApproxExist
 * @param tokens
 */
void ConditionContainerFullText::addTokensApproxExist(const std::vector<std::string> &tokens) {
    auto it = approxTokens_.find("tokens");
    if(it == approxTokens_.end()) {
        approxTokens_["tokens"] = vector<string>();
        for(int i = 0; i < tokens.size(); i++) {
            approxTokens_.find("tokens")->second.push_back(tokens[i]+"*");
        }
    } else {
        for(int i = 0; i < tokens.size(); i++) {
            it->second.push_back(tokens[i]+"*");
        }
    }
}
/**
 * @brief ConditionContainerFullText::apply
 * @param base
 * @return
 */
SqlQuery ConditionContainerFullText::apply(const SqlQuery &base) const {
    int size = 0;
    for(auto entry : tokens_) {
        size += entry.second.size();
    }
    for(auto entry : approxTokens_) {
        size += entry.second.size();
    }
    if(size > 0)
        return base.match("tokens", 1);
    else {
        LOGG(Logger::ERROR) << "Zero binder size" << Logger::FLUSH;
        return base;
    }
}
/**
 * @brief getTokens
 * @return
 */
std::map<std::string, std::vector<std::string> > ConditionContainerFullText::getCompressedTokens() const {
    return tokens_;
}
/**
 * @brief ConditionContainerFullText::getAllTokens
 * @return
 */
std::vector<std::string> ConditionContainerFullText::getNonCompressedTokens() const {
    vector<string> tokens;
    for(auto entry : tokens_) {
        if(compressed_.count(entry.first) == 0) {
            for(int i = 0; i < entry.second.size(); i++) {
                tokens.push_back(entry.first+":"+entry.second[i]);
            }
        }
    }
    for(auto entry : approxTokens_) {
        for(int i = 0; i < entry.second.size(); i++) {
            tokens.push_back(entry.first+":"+entry.second[i]);
        }
    }
    return tokens;
}
/**
 * @brief getLocationToken
 * @param level
 * @return
 */
std::string ConditionContainerFullText::getLocationToken(int level) const {
    BalancedPeanoCurve curve;
    int64_t pos = curve.convert(pt_.lat(), pt_.lon());

    string token = lexical_cast(pos);
    int len = max(0, (int)token.size()-level);
    return token.substr(0, len);
}
/**
 * @brief addLocation
 * @param pt
 */
void ConditionContainerFullText::addLocation(const Point &pt) {
    pointSet_ = true;
    pt_ = pt;
}
/**
 * @brief ConditionContainerFullText::hasLocation
 * @return
 */
bool ConditionContainerFullText::hasLocation() const {
    return pointSet_;
}
