#include "PublicTransPlugin.h"
#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/Storage/SqlConsts.h>

using namespace std;
/**
 * @brief PublicTransPlugin::PublicTransPlugin
 */
PublicTransPlugin::PublicTransPlugin(const string &outputFile) : Plugin(),
    requiredKV_(),outputFileName_(),gtfsFileName_(),
    gtfsFileDescriptor_(0), gtfsGroupFileName_(), gtfsGroupFileDescriptor_(0),
    gtfsRelFileName_(),gtfsRelFileDescriptor_(0),
     agencies_(),routes_(),trips_(),stopTimes_(),shapes_(),
    routeTypes_(),totalTagsWritten_(0),totalGroupsWritten_(0)
{
    pluginId_ = "PUBLIC_TRANSPORT";
    outputFileName_ = outputFile;
}
/**
 * @brief PublicTransPlugin::relation
 * @param rel
 */
void PublicTransPlugin::notifyRelation(OSMRelation* rel) {
    if(acceptObject(rel)) {
        storeInfo(rel);
    }
}
void PublicTransPlugin::finalize() {
    writeData();
    if(gtfsFileDescriptor_ != 0)
        fclose(gtfsFileDescriptor_);
    if(gtfsGroupFileDescriptor_ != 0)
        fclose(gtfsGroupFileDescriptor_);
    if(gtfsRelFileDescriptor_ != 0)
        fclose(gtfsRelFileDescriptor_);
}
/**
 * @brief findKey
 * @param tags
 * @param key
 * @return
 */
bool PublicTransPlugin::findKey(const string &key, const StringMap &tags) const {
    if(tags.find(key) != tags.end())
        return true;
    return false;
}
/**
 * @brief findKeys
 * @param keys
 * @param tags
 * @return
 */
bool PublicTransPlugin::findKeys(const vector<string> &keys, const StringMap &tags) const {
    for (const string& key : keys)
        if(tags.find(key) == tags.end())
            return false;
    return true;
}
/**
 * @brief findKeysValues
 * @param kvs
 * @param tags
 * @return
 */
bool PublicTransPlugin::findKeysValues(const KVSet &kvs, StringMap &tags) const {
    for (const pair<string, set<string> > &kv : kvs) {
        if ( !findKey(kv.first, tags))
            return false;
        bool found = false;
        for(const string& val: kv.second)
            if (tags[kv.first] == val)
                found = true;
        if ( !found)
            return false;
    }
    return true;
}
/**
 * @brief PublicTransport::PublicTransport
 * @param outputFile
 */
void PublicTransPlugin::init() {
    requiredKV_ = {{"route",{"bus"}},
                   {"type",{"route"}}};
    routeTypes_ = {{"bus", Route::BUS},
                   {"ferry", Route::FERRY},
                   {"train", Route::RAIL},
                   {"tram", Route::TRAM}};

    totalTagsWritten_ = 0;
    totalGroupsWritten_ = 0;

    // gtfs objects
    gtfsFileName_ = outputFileName_+"."+SqlConsts::GTFS_TAGS_TABLE;
    gtfsGroupFileName_ = outputFileName_ +"."+ SqlConsts::GTFS_GROUP_TAG_TABLE;
    gtfsRelFileName_ = outputFileName_+"."+SqlConsts::GTFS_GROUPS_RELATIONSHIP_TABLE;

    // gtfs groups
    gtfsFileDescriptor_ = fopen(gtfsFileName_.c_str(), "w+");
    gtfsGroupFileDescriptor_ = fopen(gtfsGroupFileName_.c_str(), "w+");
    gtfsRelFileDescriptor_ = fopen(gtfsRelFileName_.c_str(), "w+");
}
/**
 * @brief
 */
void PublicTransPlugin::writeData() {
    // assign ids to stop times
    for(int i = 0; i < stopTimes_.size(); i++) {
        stopTimes_[i].setId(i);
    }

    // prewrite data to gtfs format
    prewriteData(agencies_, "agency.txt");
    prewriteData(routes_, "routes.txt");
    prewriteData(trips_, "trips.txt");
    prewriteData(stopTimes_, "stop_times.txt");
    prewriteData(shapes_, "shapes.txt");

    // assign new ids to all entities (call python script)
    if(system("python3 ../tools/gtfs_assign_ids.py") != EXIT_SUCCESS) {
        LOGG(Logger::ERROR) << "Failure calling gtfs_assign_ids.py script" << Logger::FLUSH;
        // remove all gtfs files on error
        vector<string> files = {"agency.txt", "routes.txt", "trips.txt", "stop_times.txt",
                                "shapes.txt"};
        for(const string &file : files)
            std::remove(file.c_str());
        return;
    }

    // turn off id translation
    Agency::translator_.setOff(true);
    Route::translator_.setOff(true);
    Trip::translator_.setOff(true);
    Shape::translator_.setOff(true);

    // unserialize entities
    string folder = "";
    agencies_ = readObjects<Agency>(folder+"agency.txt");
    routes_ = readObjects<Route>(folder+"routes.txt");
    trips_ = readObjects<Trip>(folder+"trips.txt");
    stopTimes_ = readObjects<StopTime>(folder+"stop_times.txt");
    shapes_ = readObjects<Shape>(folder+"shapes.txt");

    // serialize to searchable format
    // TODO: not write arrival/departure times?
    for (const Agency& agency : agencies_)
        totalTagsWritten_ += agency.serialize(gtfsFileDescriptor_);

    for (const Route& route : routes_)
        totalGroupsWritten_ += route.serialize(gtfsGroupFileDescriptor_);

    for (const Trip& trip : trips_)
        totalGroupsWritten_ += trip.serialize(gtfsGroupFileDescriptor_);

    for(const StopTime &stopTime : stopTimes_)
        totalTagsWritten_ += stopTime.serialize(gtfsFileDescriptor_);

    for (const Shape& shape : shapes_)
        totalGroupsWritten_ += shape.serialize(gtfsGroupFileDescriptor_);

    for(const StopTime &stopTime : stopTimes_) {
        stopTime.serializeGroupRelationship(gtfsRelFileDescriptor_);
    }

    // turn on id translation
    Agency::translator_.setOff(false);
    Route::translator_.setOff(false);
    Trip::translator_.setOff(false);
    Shape::translator_.setOff(false);
}
/**
 * @brief validate
 * @param outFileName
 * @return
 */
void PublicTransPlugin::validate() {
    SqlStream sStream;
    URL url(outputFileName_);
    Properties props = {{"type", "sqlite"}, {"create", "0"}};
    sStream.open(url, props);

    size_t rCount = sStream.getNumRows(SqlConsts::GTFS_TAGS_TABLE);
    if (rCount != totalTagsWritten_)
        die(pluginId_, "Wrong row count in gtfs tags "+lexical_cast(rCount)+", expected "
                                      +lexical_cast(totalTagsWritten_));

    rCount = sStream.getNumRows(SqlConsts::GTFS_GROUP_TAG_TABLE);
    if (rCount != totalGroupsWritten_)
        die(pluginId_, "Wrong row count in gtfs tags "+lexical_cast(rCount)+", expected "
                                      +lexical_cast(totalGroupsWritten_));
    // TODO groups rel check
}
/**
 * @brief cleanUp
 */
void PublicTransPlugin::cleanUp() {
    std::remove(gtfsFileName_.c_str());
    std::remove(gtfsGroupFileName_.c_str());
    std::remove(gtfsRelFileName_.c_str());
}
/**
 * @brief PublicTransPlugin::getTableNamesToImport
 * @return
 */
vector<string> PublicTransPlugin::getTableNamesToImport() const {
    return {SqlConsts::GTFS_TAGS_TABLE,
            SqlConsts::GTFS_GROUP_TAG_TABLE,
            SqlConsts::GTFS_GROUPS_RELATIONSHIP_TABLE};
}
/**
 * @brief PublicTransPlugin::getSqlToCreateTables
 * @return
 */
vector<string> PublicTransPlugin::getSqlToCreateTables() const {
    return {SqlConsts::CREATE_GTFS_TAGS,
            SqlConsts::CREATE_GTFS_GROUP_TAG,
            SqlConsts::CREATE_GTFS_GROUP_RELATIONSHIPS};
}
/**
 * @brief PublicTransPlugin::getOtherSqlCommands
 * @return
 */
vector<string> PublicTransPlugin::getOtherSqlCommands() const {
    return {SqlConsts::UPDATE_GTFS_LOWER_CASE,
            SqlConsts::UPDATE_GTFS_GROUP_LOWER_CASE,
            SqlQuery::ci(SqlConsts::GTFS_TAGS_TABLE,{"objectid"}),
            SqlQuery::ci(SqlConsts::GTFS_TAGS_TABLE,{"value"}),
            SqlQuery::ci(SqlConsts::GTFS_TAGS_TABLE,{"tag"}),
            SqlQuery::ci(SqlConsts::GTFS_GROUP_TAG_TABLE,{"groupid"}),
            SqlQuery::ci(SqlConsts::GTFS_GROUP_TAG_TABLE,{"value"}),
            SqlQuery::ci(SqlConsts::GTFS_GROUP_TAG_TABLE,{"tag"}),
            SqlQuery::ci(SqlConsts::GTFS_GROUPS_RELATIONSHIP_TABLE,{"objectid"}),
            SqlQuery::ci(SqlConsts::GTFS_GROUPS_RELATIONSHIP_TABLE,{"groupid"})};
}

