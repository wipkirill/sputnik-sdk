#include <UrbanLabs/Sdk/Utils/FileSystemUtil.h>
#include <UrbanLabs/Sdk/Utils/StringUtils.h>
#include <UrbanLabs/Sdk/Storage/SqlConsts.h>
#include "MapIconPlugin.h"

using namespace std;

/**
 * @brief MapIconPlugin::MapIconPlugin
 * @param outputFile
 */
MapIconPlugin::MapIconPlugin(const string &outputFile) : mapFileName_(outputFile) {
    pluginId_ = "MAP ICON PLUGIN";
    workDir_ = ".";
    if (!FsUtil::currentWorkDir(workDir_))
        die(pluginId_,"Cannot determine current workdir");
}
/**
 * @brief MapIconPlugin::init
 */
void MapIconPlugin::init() {
    URL url(mapFileName_+".vp.sqlite");
    Properties props = {{"type", "sqlite"},{"table", SqlConsts::VP_TABLE}};
    if(!vpIndex_.open(url, props))
        die(pluginId_, "can't initialize vertexPoint database");
}
/**
 * @brief MapIconPlugin::afterImport
 */
void MapIconPlugin::afterImport() {
    //vector<Point> mb;
    //if(!vpIndex_.getBoundingBox(mb))
       //die(pluginId_, "Failed to get bounding box");
    //string icon;
    //if(!tileServer_.renderIcon(mapFileName_, mb, icon))
        //die(pluginId_, "Failed to get render icon");
//
    //// save to mapinfo
    //URL url(mapFileName_);
    //Properties props = {{"type", "sqlite"},{"create", "1"}};
    //unique_ptr<DbConn> conn = ConnectionsManager::getConnection(url, props);
    //if(!conn || !conn->open(url, props))
        //die(pluginId_,"Failed to open database");
//
    //unique_ptr<PrepStmt> insertStmt;
    //string mInfoInsert = "INSERT INTO mapinfo values(?,?,?)";
    //if(!conn->prepare(mInfoInsert, insertStmt))
        //die(pluginId_,"Failed to prepare insert query");
//
    //string encoded = StringUtils::base64_encode(icon.c_str(), icon.size());
    //if(!insertStmt->bind(0))
        //die(pluginId_, "Couldn't bind id " + mapFileName_);
    //if(!insertStmt->bind("icon"))
        //die(pluginId_, "Couldn't bind icon " + mapFileName_);
    //if(!insertStmt->bind(encoded))
        //die(pluginId_, "Couldn't bind encoded " + mapFileName_);
    //if(!insertStmt->exec())
        //die(pluginId_, "Couldn't insert " + mapFileName_);
    //insertStmt->reset();
}
