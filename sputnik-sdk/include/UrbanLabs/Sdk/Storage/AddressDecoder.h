#pragma once

#include <string>
#include <memory>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/SqlModels/Tag.h>
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>
#include <UrbanLabs/Sdk/GraphCore/BoundingBox.h>
#include <UrbanLabs/Sdk/Storage/KdTreeSql.h>
#include <UrbanLabs/Sdk/Storage/ConnectionsManager.h>

typedef struct AddressRecord AddressRecord;

/**
 * @brief The AddressDecoder class
 */
class AddressDecoder {
public:
    class Initializer {
    private:
        URL url_;
        Properties props_;
    public:
        Initializer(const URL &url, const Properties &props);
        bool init(AddressDecoder &storage) const;
    };
    class Destructor {
    public:
        static bool release(AddressDecoder &storage);
    };
private:
    TagStorage tags_;
    std::string tagTable_;
    std::map<std::string, Point::PointDistType> dist_;
    std::map<std::string, std::shared_ptr<KdTreeSql> > addrTree_;
public:
    AddressDecoder();
    bool open(const URL &url, const Properties &props);
    bool close();
    bool resolve(const Point &pt, TagList &list);
private:
    template<typename Filter>
    TagList filterTagList(TagList & tagList, Filter filter);
    bool goodNeighbor(const Point &pt, const Point &neigh, const std::string &type);
};
