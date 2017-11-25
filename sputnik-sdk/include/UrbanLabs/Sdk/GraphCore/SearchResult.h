#pragma once

#include <vector>
#include <unordered_map>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/GraphCore/Edges.h>
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>
#include <UrbanLabs/Sdk/GraphCore/NearestNeighbor.h>

namespace OsmGraphCore {
/**
 * @brief The SearchResultBasic class
 */
class SearchResultBasic {
protected:
    typedef Vertex::VertexId VertexId;
    typedef Edge::EdgeDist DistType;
    typedef OsmGraphCore::NearestPointResult NearestPointResult;
protected:
    DistType length_;
    NearestPointResult source_, dest_;
    std::vector<Edge::EdgeId> origIds_;
    std::vector<Vertex::VertexId> path_;
public:
    SearchResultBasic();
    SearchResultBasic(const NearestPointResult &src, const NearestPointResult &dest);
    void setPath(const std::vector <Vertex::VertexId> &path);
    void setOrigIds(const std::vector<Edge::EdgeId> &origIds);
    void setLength(DistType len);
    bool isValid() const;
    DistType getLength() const;
    NearestPointResult getSrc() const;
    NearestPointResult getDst() const;
    const std::vector<Edge::EdgeId> &getOrigWayIds() const;
    const std::vector<Vertex::VertexId> &getPath() const;
};

/**
 * @brief The SearchResultLocal class
 */
class SearchResultLocal : public SearchResultBasic {
public:
    typedef Vertex::VertexId VertexId;
    typedef Edge::EdgeDist DistType;
private:
    size_t searchSpace_;
    std::unordered_map<VertexId, DistType> distToTargets_;
public:
    SearchResultLocal();
    SearchResultLocal(const NearestPointResult &src, const NearestPointResult &dest);
    void setDistToTargets(const std::unordered_map<VertexId, DistType> &dists);
    void setSearchSpace(size_t searchSpace);
    size_t getSearchSpace() const;
    DistType getTargetDistFor(const VertexId id) const;
};
} // OsmGraphCore

