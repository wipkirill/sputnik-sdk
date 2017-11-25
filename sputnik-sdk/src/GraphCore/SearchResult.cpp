// SearchResult.cpp
//
#include <UrbanLabs/Sdk/GraphCore/SearchResult.h>

using namespace std;
using namespace OsmGraphCore;

//------------------------------------------------------------------------------
// Default search result
//------------------------------------------------------------------------------
/**
 * @brief SearchResultBasic::SearchResult
 * @param src
 * @param dest
 */
SearchResultBasic::SearchResultBasic(const NearestPointResult &src, const NearestPointResult &dest)
    : length_(numeric_limits<DistType>::max()), source_(src), dest_(dest) {;}
/**
 * @brief SearchResultBasic::SearchResult
 */
SearchResultBasic::SearchResultBasic() : length_(numeric_limits<DistType>::max()) {;}
/**
 * @brief SearchResultBasic::getSrc
 * @return
 */
NearestPointResult SearchResultBasic::getSrc() const {
    return source_;
}
/**
 * @brief SearchResultBasic::getDest
 * @return
 */
NearestPointResult SearchResultBasic::getDst() const {
    return dest_;
}
/**
 * @brief SearchResultBasic::isValid
 * @return
 */
bool SearchResultBasic::isValid() const {
    return path_.size() > 0;
}
/**
 * @brief SearchResultBasic::getPath
 * @return
 */
const vector<Vertex::VertexId> &SearchResultBasic::getPath() const {
    return path_;
}
/**
 * @brief SearchResultBasic::getOrigWayIds
 * @return
 */
const vector<Edge::EdgeId> &SearchResultBasic::getOrigWayIds() const {
    return origIds_;
}
/**
 * @brief SearchResultBasic::setPath
 * @param path
 */
void SearchResultBasic::setPath(const vector <Vertex::VertexId> &path) {
    path_ = path;
}
/**
 * @brief SearchResultBasic::setOrigIds
 * @param origIds
 */
void SearchResultBasic::setOrigIds(const vector<Edge::EdgeId> &origIds) {
   origIds_ = origIds;
}
/**
 * @brief SearchResultBasic::getLength
 * @return
 */
SearchResultBasic::DistType SearchResultBasic::getLength() const {
    return length_;
}
/**
 * @brief SearchResultBasic::setLength
 * @param len
 */
void SearchResultBasic::setLength(DistType len) {
    length_ = len;
}
//------------------------------------------------------------------------------
// Local search search result
//------------------------------------------------------------------------------
/**
 * @brief SearchResultLocal::SearchResultLocal
 */
SearchResultLocal::SearchResultLocal() : SearchResultBasic(), searchSpace_(0), distToTargets_() {
    ;
}
/**
 * @brief SearchResultLocal::SearchResultLocal
 * @param src
 * @param dest
 */
SearchResultLocal::SearchResultLocal(const NearestPointResult &src, const NearestPointResult &dest)
    : SearchResultBasic(src, dest), searchSpace_(0), distToTargets_() {
    ;
}
/**
 * @brief SearchResultLocal::setSearchSpace
 * @param searchSpace
 */
void SearchResultLocal::setSearchSpace(size_t searchSpace) {
    searchSpace_ = searchSpace;
}
/**
 * @brief SearchResultLocal::getSearchSpace
 * @return
 */
size_t SearchResultLocal::getSearchSpace() const {
    return searchSpace_;
}
/**
 * @brief SearchResultLocal::getTargetDistFor
 * @param id
 * @return
 */
SearchResultLocal::DistType SearchResultLocal::getTargetDistFor(const VertexId id) const{
    return distToTargets_.find(id)->second;
}
/**
 * @brief SearchResultLocal::setDistToTargets
 * @param dists
 */
void SearchResultLocal::setDistToTargets(const unordered_map<VertexId, DistType> &dists) {
    distToTargets_ = dists;
}
