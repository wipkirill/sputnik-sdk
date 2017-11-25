#pragma once

#include <string>
#include <memory>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/OSM/TagFilter.h>
#include <UrbanLabs/Sdk/SqlModels/Tag.h>
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>
#include <UrbanLabs/Sdk/GraphCore/BoundingBox.h>
#include <UrbanLabs/Sdk/Storage/ConnectionsManager.h>

class TagSuggestStorage {
private:
    std::string table_;
    std::string dbName_;
    TagFilter tagFilter_;
    std::unique_ptr<DbConn> conn_;
    std::unique_ptr<PrepStmt> stmt_;
public:
    bool open(const URL &url, const Properties &props);
    bool close();
    bool matchTag(const std::string &tag, std::vector<std::string> &tags);
};
