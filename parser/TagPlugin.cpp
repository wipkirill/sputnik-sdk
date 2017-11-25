#include "TagPlugin.h"
#include <UrbanLabs/Sdk/Storage/SqlConsts.h>

using std::string;

/**
 * @brief TagPlugin::TagPlugin
 * @param inputFile
 */
TagPlugin::TagPlugin(const string &outFile) : tagsWritten_(0) {
    tagFileName_ = outFile+"."+SqlConsts::TAGS_TABLE;
    outFileName_ = outFile;
}
/**
 * @brief TagPlugin::init
 */
void TagPlugin::init() {
    tagFile_ = fopen(tagFileName_.c_str(), "w");
    locales_ = {"en"};
    tagsWritten_ = 0;
}
/**
 * @brief TagPlugin::notifyNode
 * @param node
 */
void TagPlugin::notifyNode(OSMNode* node) {
    if(tagFilter_.acceptObject(node)) {
        for(int i = 0; i < node->nTags; ++i) {
            string key = node->tags[i].key;
            string value = node->tags[i].value;
            if(tagFilter_.acceptTag(key, value) && tagFilter_.validValue(key) && tagFilter_.validValue(value)) {
                // get the canonical tags
                pair<string, string> canon = tagFilter_.getCanonicalTag(key, value);
                key = canon.first, value = canon.second;

                tags_.insert(StringUtils::escape(key));
                if(value != "" && tagFilter_.isAggregateTag(key))
                    tags_.insert(StringUtils::escape(value));
            }
        }
    }
}
/**
 * @brief TagPlugin::notifyWay
 * @param way
 */
void TagPlugin::notifyWay(OSMWay* way) {
    if(tagFilter_.acceptObject(way)) {
       for(int i = 0; i < way->nTags; ++i) {
            string key = way->tags[i].key;
            string value = way->tags[i].value;
            if(tagFilter_.acceptTag(key, value) && tagFilter_.validValue(key) && tagFilter_.validValue(value)) {
                // get the canonical tags
                pair<string, string> canon = tagFilter_.getCanonicalTag(key, value);
                key = canon.first, value = canon.second;

                tags_.insert(StringUtils::escape(key));
                if(value != "" && tagFilter_.isAggregateTag(key))
                    tags_.insert(StringUtils::escape(value));
            }
        }
    }
}
/**
 * @brief TagPlugin::finalize
 */
void TagPlugin::finalize() {
    string sep = StringConsts::SEPARATOR;
    string printSpec = type_spec<string>();
    for(int i = 0; i+1 < locales_.size(); i++)
        printSpec.append(sep+type_spec<string>());
    printSpec.append("\n");

    for(const string &tag : tags_) {
        for(int i = 0; i < locales_.size(); i++) {
            fprintf(tagFile_, printSpec.c_str(), tag.c_str());
            tagsWritten_++;
        }
    }
    fclose(tagFile_);
}
/**
 * @brief TagPlugin::cleanUp
 */
void TagPlugin::cleanUp() {
    std::remove(tagFileName_.c_str());
}
/**
 * @brief TagPlugin::validate
 */
void TagPlugin::validate() {
    SqlStream sStream;
    URL url(outFileName_);
    Properties props = {{"type", "sqlite"}, {"create", "0"}};
    sStream.open(url, props);

    size_t rCount = sStream.getNumRows(SqlConsts::TAGS_TABLE);
    if(rCount != tagsWritten_) {
        die(pluginId_, "Wrong number of tags written");
    }
}
/**
 * @brief TagPlugin::getTableNamesToImport
 * @return
 */
vector<string> TagPlugin::getTableNamesToImport() const {
    vector<string> tables = {SqlConsts::TAGS_TABLE};
    return tables;
}
/**
 * @brief TagPlugin::getSqlToCreateTables
 * @return
 */
vector<string> TagPlugin::getSqlToCreateTables() const {
    vector<string> tables = {SqlConsts::CREATE_TAGS_TABLE};
    return tables;
}
/**
 * @brief TagPlugin::getOtherSqlCommands
 * @return
 */
vector<string> TagPlugin::getOtherSqlCommands() const {
    vector<string> other;
    for(int i = 0; i < locales_.size(); i++)
        other.push_back(SqlQuery::ci(SqlConsts::TAGS_TABLE,{locales_[i]}));
    return other;
}
