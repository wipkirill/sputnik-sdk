#pragma once

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/GraphCore/EdgesGTFS.h>
#include <UrbanLabs/Sdk/GraphCore/VerticesGTFS.h>
#include <UrbanLabs/Sdk/GraphCore/NearestNeighbor.h>

namespace GtfsGraphCore {
/**
 * @brief The SearchResult class
 */
class SearchResultGtfs {
public:
    typedef VertexGTFS::VertexId VertexId;
    typedef EdgeGTFS::EdgeDist DistType;
    typedef OsmGraphCore::NearestPointResult NearestPointResult;
private:
    DistType length_;
    NearestPointResult src_, dst_;
    std::vector<EdgeGTFS::EdgeId> origIds_;
    std::vector<VertexGTFS::VertexId> path_;
public:
    SearchResultGtfs();
    SearchResultGtfs(const NearestPointResult &src, const NearestPointResult &dest);
    void setPath(const std::vector <VertexGTFS::VertexId> &path);
    void setOrigIds(const std::vector<EdgeGTFS::EdgeId> &origIds);
    void setLength(DistType len);
    bool isValid() const;
    VertexPoint getStart() const;
    VertexPoint getEnd() const;
    DistType getLength() const;
    NearestPointResult getSrc() const;
    NearestPointResult getDst() const;
    const std::vector<EdgeGTFS::EdgeId> &getOrigWayIds() const;
    const std::vector<VertexGTFS::VertexId> &getPath() const;
};
} 
