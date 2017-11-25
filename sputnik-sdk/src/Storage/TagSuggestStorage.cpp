#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/Storage/TagSuggestStorage.h>

using std::string;

/**
 * @brief TagSuggestStorage::open
 * @param url
 * @param props
 * @return
 */
bool TagSuggestStorage::open(const URL &url, const Properties &props) {
    conn_ = ConnectionsManager::getConnection(url, props);
    if(!conn_ || !conn_->open(url, props))
        return false;

    // TODO: sql injection on locale
    string locale = props.get("locale");
    if(locale == "")
        locale = "en";
    string table = props.get("table");
    SqlQuery tagLike = SqlQuery::q().select({"en"}).from(table).hasPrefix(locale, "?");
    if(!conn_->prepare(tagLike.toString(), stmt_))
        return false;
    return true;
}

/**
 * @brief TagSuggestStorage::close
 * @return
 */
bool TagSuggestStorage::close() {
    stmt_.reset(0);
    conn_.reset(0);
    return true;
}
/**
 * @brief TagSuggestStorage::matchTag
 * @param tag
 * @param tags
 * @return
 */
bool TagSuggestStorage::matchTag(const std::string &tag, std::vector<std::string> &tags) {
    if(stmt_) {
        stmt_->reset();
        if(!stmt_->bind(tag) || !stmt_->bind(tag))
            return false;
        while(stmt_->step()) {
            string res = stmt_->column_text(0);
            tags.push_back(res);
        }
        return true;
    }
    return false;
}
