#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/Timer.h>
#include <UrbanLabs/Sdk/Utils/Types.h>
#include <UrbanLabs/Sdk/Utils/URL.h>
#include <UrbanLabs/Sdk/Utils/Properties.h>
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>
#include <UrbanLabs/Sdk/GraphCore/Edges.h>
#include <UrbanLabs/Sdk/SqlModels/Tag.h>
#include <UrbanLabs/Sdk/Storage/SqlQuery.h>
#include <UrbanLabs/Sdk/Storage/QueryConditions.h>
#include <UrbanLabs/Sdk/Storage/DatabaseConnection.h>

/**
 * @brief The VertexToPointIndexSqlite class
 */
class ObjectIdStorage {
public:
    class Initializer {
    private:
        URL url_;
        Properties props_;
    public:
        Initializer(const URL &url, const Properties &props);
        bool init(ObjectIdStorage &storage) const;
    };
    class Destructor {
    public:
        static bool release(ObjectIdStorage &storage);
    };
private:
    std::unique_ptr<DbConn> conn_;
    std::string table_, inner_, outer_;
public:
    ~ObjectIdStorage();
    bool open(const URL &url, const Properties &props);
    bool close();
    bool fromInternal(const std::vector<Vertex::VertexId> &from, std::vector<std::string> &to);
    bool toInternal(const std::vector<std::string> &from, std::vector<Vertex::VertexId> &to);
};
