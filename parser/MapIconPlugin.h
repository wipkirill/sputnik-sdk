#pragma once

#include "ParserPlugin.h"
#include <UrbanLabs/Sdk/Utils/FileSystemUtil.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/Storage/ConnectionsManager.h>
/**
 * @brief The MapIconPlugin class
 */
class MapIconPlugin : public Plugin {
private:
    std::string mapFileName_;
    std::string workDir_;
    VertexToPointIndexSql vpIndex_;
public:
    MapIconPlugin(const std::string& outputFile);
    virtual ~MapIconPlugin(){}
    void init();
    virtual void afterImport();
};
