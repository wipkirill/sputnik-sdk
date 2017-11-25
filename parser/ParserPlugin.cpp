#include "ParserPlugin.h"

#include <vector>

using namespace std;

string Plugin::WORK_DIRECTORY = "";

/**
 * @brief Plugin::setWorkDir
 * @param dirName
 */
void Plugin::setWorkDir(const string &dirName) {
    Plugin::WORK_DIRECTORY = dirName;
}
/**
 * @brief Plugin::getWorkDir
 * @return
 */
string Plugin::getWorkDir() const {
    return Plugin::WORK_DIRECTORY;
}

/**
 * @brief isValidWay
 * @param way
 * @return
 */
bool Plugin::isValidWay(OSMWay* w){
    if (osmValidator_ != 0) {
        return osmValidator_->isValidWay(w);
    }
    return true;
}

void Plugin::node(osmium::Node& osmiumNode) {
    OSMNode node;
    node.readOsmium(osmiumNode);
    notifyNode(&node);
}

void Plugin::way(osmium::Way& osmiumWay) {
    OSMWay way;
    way.readOsmium(osmiumWay);
    notifyWay(&way);
}

void Plugin::relation(osmium::Relation& osmiumRel) {
    OSMRelation rel;
    rel.readOsmium(osmiumRel);
    notifyRelation(&rel);
}

/**
 * Each plugin can subscribe for N parser passes
 * Default is 1 pass, when all of the methods
 * notifyNode(), notifyWay, notifyRelation
 * are called. Can be a situation when it should be done
 * N times.
 * @brief getPassNumber
 * @return
 */
int Plugin::getPassNumber() {
    return 1;
}
/**
 * This method is called with id of current parser pass only if
 * a Plugin requires more than one round
 * @brief notifyPassNumber
 * @param currPassId
 */
void Plugin::notifyPassNumber(const int /*currPassId*/) {
    ;
}
/**
 * @brief setOsmValidator
 * @param v
 */
void Plugin::setOsmValidator(OsmValidator *v) {
    osmValidator_ = v;
}
/**
 * @brief getTableNamesToImport
 * @return
 */
vector<string> Plugin::getTableNamesToImport() const {
    return vector<string>();
}
/**
 * @brief getSqlToCreateTables
 * @return
 */
vector<string> Plugin::getSqlToCreateTables() const {
    return vector<string>();
}
/**
 * @brief getOtherSqlCommands
 * @return
 */
vector<string> Plugin::getOtherSqlCommands() const {
    return vector<string>();
}
/**
 * @brief getFileNamesToRead is used to .read files from shell
 * this was done because sometimes we need to import BLOB
 * data which cannot be .imported from a CSV file
 * @return
 */
vector<string> Plugin::getFileNamesToRead() const {
    return vector<string>();
}
