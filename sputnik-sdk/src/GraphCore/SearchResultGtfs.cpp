// SearchResultGtfs.cpp
//
#include <UrbanLabs/Sdk/GraphCore/SearchResultGtfs.h>

using namespace std;
using namespace GtfsGraphCore;

/**
 * @brief SearchResultGtfs::SearchResultGtfs
 * @param src
 * @param dest
 */
SearchResultGtfs::SearchResultGtfs(const NearestPointResult &src, const NearestPointResult &dest)
    : length_(numeric_limits<DistType>::max()), src_(src), dst_(dest) {
    ;
}
/**
 * @brief SearchResultGtfs::SearchResultGtfs
 */
SearchResultGtfs::SearchResultGtfs() : length_(numeric_limits<DistType>::max()) {
    ;
}
/**
 * @brief SearchResultGtfs::getSrc
 * @return
 */
OsmGraphCore::NearestPointResult SearchResultGtfs::getSrc() const {
    return src_;
}
/**
 * @brief SearchResultGtfs::getDest
 * @return
 */
OsmGraphCore::NearestPointResult SearchResultGtfs::getDst() const {
    return dst_;
}
/**
 * @brief SearchResultGtfs::isValid
 * @return
 */
bool SearchResultGtfs::isValid() const {
    return path_.size() > 0;
}
/**
 * @brief SearchResultGtfs::getPath
 * @return
 */
const vector<VertexGTFS::VertexId> &SearchResultGtfs::getPath() const {
    return path_;
}
/**
 * @brief SearchResultGtfs::getOrigWayIds
 * @return
 */
const vector<EdgeGTFS::EdgeId> &SearchResultGtfs::getOrigWayIds() const {
    return origIds_;
}
/**
 * @brief SearchResultGtfs::setPath
 * @param path
 */
void SearchResultGtfs::setPath(const vector <VertexGTFS::VertexId> &path) {
    path_ = path;
}
/**
 * @brief SearchResultGtfs::setOrigIds
 * @param origIds
 */
void SearchResultGtfs::setOrigIds(const vector<EdgeGTFS::EdgeId> &origIds) {
   origIds_ = origIds;
}
/**
 * @brief SearchResultGtfs::getLength
 * @return
 */
SearchResultGtfs::DistType SearchResultGtfs::getLength() const {
    return length_;
}
/**
 * @brief SearchResultGtfs::setLength
 * @param len
 */
void SearchResultGtfs::setLength(DistType len) {
    length_ = len;
}
