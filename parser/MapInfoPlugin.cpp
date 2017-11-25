#include "ParserOptions.h"
#include <UrbanLabs/Sdk/Utils/FileSystemUtil.h>
#include <UrbanLabs/Sdk/Storage/SqlConsts.h>
#include <UrbanLabs/Sdk/Storage/ConnectionsManager.h>
#include "MapInfoPlugin.h"

using namespace std;

/**
 * @brief MapInfoPlugin::MapInfoPlugin
 * @param outputFile
 * @param mapFeatures
 */
MapInfoPlugin::MapInfoPlugin(const string& outputFile, const std::set<string> &mapFeatures):
    Plugin(),mapInfoFileName_(),outputFileName_(),mapInfoFileDescriptor_(0),
    vertexPointFileName_(), mapFeatures_(), features_()
{
    pluginId_ = "MAP_INFO_PLUGIN";
    features_ = {{ParserOption::TAGS, "tags"},
                 {ParserOption::ROUTING, "routing"},
                 {ParserOption::SEARCH,"search"},
                 {ParserOption::GTFS,"gtfs"},
                 {ParserOption::TILES, "tiles"},
                 {ParserOption::TURN_RESTRICTIONS, "turnrestrictions"},
                 {ParserOption::ADDRESS_DECODER, "addressdecoder"}};
    outputFileName_ = outputFile;
    mapInfoFileName_ = outputFileName_ + StringConsts::PT+SqlConsts::MAP_INFO_TABLE;
    mapInfoFileDescriptor_ = fopen(mapInfoFileName_.c_str(), "a");
    vertexPointFileName_ = outputFileName_ + ".vp.sqlite";
    mapFeatures_ = mapFeatures;
}
/**
 * @brief MapInfoPlugin::init
 * @param outputFile
 */
void MapInfoPlugin::init() {
    URL url(vertexPointFileName_);
    Properties props = {{"type", "sqlite"},{"table", SqlConsts::VP_TABLE}};
    if(!vpIndex_.open(url, props))
        die(pluginId_, "can't initialize vertexPoint database");

    vector<Point> bbox;
    if (!vpIndex_.getBoundingBox(bbox)) {
        die(pluginId_, "Can't find bounding box");
    }
    string sep = StringConsts::SEPARATOR;
    string tagPrintSpec = type_spec<VertexId>()+sep+type_spec<string>()+sep+type_spec<string>()+"\n";
    string bboxString = lexical_cast(bbox[0].lat())+" "+lexical_cast(bbox[0].lon())
            +" "+lexical_cast(bbox[1].lat())+" "+lexical_cast(bbox[1].lon());
    fprintf(mapInfoFileDescriptor_, tagPrintSpec.c_str(), 0, "bbox", bboxString.c_str());

    // store map features
    for (const string& ft : mapFeatures_) {
        if (features_.find(ft) != features_.end()) {
            fprintf(mapInfoFileDescriptor_, tagPrintSpec.c_str(), 0, "map_feature", features_[ft].c_str());
        }
    }
    if(areaName_ != "") {
        fprintf(mapInfoFileDescriptor_, tagPrintSpec.c_str(), 0, "area_name", areaName_.c_str());
        LOGG(Logger::DEBUG) << pluginId_ << "Area name set" << areaName_ << Logger::FLUSH;
    } else {
        LOGG(Logger::WARNING) << pluginId_ << "Area name not set" << Logger::FLUSH;
    }
    if(countryCodes_.size() > 0) {
        for(const string &cc:countryCodes_)
            fprintf(mapInfoFileDescriptor_, tagPrintSpec.c_str(), 0, "country_codes", cc.c_str());
        LOGG(Logger::DEBUG) << pluginId_ << "Set" << countryCodes_.size() << "country code(s)" << Logger::FLUSH;
    } else {
        LOGG(Logger::WARNING) << pluginId_ << "Country codes not set" << Logger::FLUSH;
    }
    if(comment_ != "") {
        fprintf(mapInfoFileDescriptor_, tagPrintSpec.c_str(), 0, "comment", comment_.c_str());
        LOGG(Logger::DEBUG) << pluginId_ << "Comment:" << comment_ << Logger::FLUSH;
    } else {
        LOGG(Logger::WARNING) << pluginId_ << "Comment not set" << Logger::FLUSH;
    }

    fclose(mapInfoFileDescriptor_);
}
/**
 * @brief MapInfoPlugin::afterImport
 */
void MapInfoPlugin::setAreaName(const std::string &areaName) {
    areaName_ = StringUtils::escape(areaName);
}
/**
 * @brief MapInfoPlugin::afterImport
 */
void MapInfoPlugin::setCountryCodes(const std::string &countryCodes) {
    if(!countryCodes.empty()) {
        SimpleTokenator tokenator(StringUtils::escape(countryCodes), ',', '\"', true);
        countryCodes_ = tokenator.getTokens();
    }
}
/**
 * @brief MapInfoPlugin::afterImport
 */
void MapInfoPlugin::setComment(const std::string &comment) {
    comment_ = StringUtils::escape(comment);
}
/**
 * @brief MapInfoPlugin::afterImport
 */
void MapInfoPlugin::afterImport() {
    // store country and city names
    if(mapFeatures_.count(ParserOption::ADDRESS_DECODER) > 0) {
        URL url(outputFileName_);
        Properties props = {{"type", "sqlite"},{"create", "1"}};
        unique_ptr<DbConn> conn = ConnectionsManager::getConnection(url, props);
        if(!conn || !conn->open(url, props)) {
            LOGG(Logger::ERROR) << pluginId_ << " Failed to open database" << Logger::FLUSH;
            return;
        }
        if(!conn->exec(SqlConsts::PRAGMAS))
            return;

        SqlQuery areaQuery;
        areaQuery = areaQuery.select({"name","areatype", "abs(hl_lat-lr_lat)*abs(hl_lon-lr_lon) as area"})
                .from(SqlConsts::ADDRESS_DECODER_AREAS_TABLE)
                .where("areatype in ('country','city')").orderDesc("area");
        unique_ptr<PrepStmt> stmt;
        if(!conn->prepare(areaQuery.toString(), stmt)) {
            LOGG(Logger::ERROR) << pluginId_ << " Failed to prepare area query:"<< areaQuery.toString()<< Logger::FLUSH;
            return;
        }
        string mInfoInsert = "INSERT INTO "+SqlConsts::MAP_INFO_TABLE+" VALUES(?,?,?)";
        unique_ptr<PrepStmt> insertStmt;
        if(!conn->prepare(mInfoInsert, insertStmt)) {
            LOGG(Logger::ERROR) << pluginId_ << " Failed to prepare insert query:"<< Logger::FLUSH;
            return;
        }

        KeyValuePair fName("mapfile",outputFileName_);
        insertKeyValue(0, fName, insertStmt);
        while(stmt->step()) {
            string name = stmt->column_text(0);
            string areatype = stmt->column_text(1);
            KeyValuePair k(areatype, name);
            insertKeyValue(0, k, insertStmt);
        }
    } else {
        LOGG(Logger::DEBUG) << pluginId_ << " Address decoding unavailable" << Logger::FLUSH;
    }
}
/**
 * @brief MapInfoPlugin::insertKeyValue
 * @param id
 * @param kv
 * @param stmt
 * @return
 */
bool MapInfoPlugin::insertKeyValue(int id, const KeyValuePair &kv, unique_ptr<PrepStmt> &insertStmt){
    if(!insertStmt->bind(id) || !insertStmt->bind(kv.getKey()) || !insertStmt->bind(kv.getValue())) {
        die(pluginId_, "Couldn't bind parameter");
    }

    if(!insertStmt->exec())
        die(pluginId_, "Couldn't insert");
    insertStmt->reset();
    return true;
}
/**
 * @brief MapInfoPlugin::cleanUp
 */
void MapInfoPlugin::cleanUp() {
    std::remove(mapInfoFileName_.c_str());
}
/**
 * @brief MapInfoPlugin::getTableNamesToImport
 * @return
 */
vector<string> MapInfoPlugin::getTableNamesToImport() const {
    return {SqlConsts::MAP_INFO_TABLE};
}
/**
 * @brief MapInfoPlugin::getSqlToCreateTables
 * @return
 */
vector<string> MapInfoPlugin::getSqlToCreateTables() const {
    return {SqlConsts::CREATE_MAP_INFO};
}
