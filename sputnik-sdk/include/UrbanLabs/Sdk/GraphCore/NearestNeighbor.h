#pragma once

namespace OsmGraphCore {
/**
 * @brief The NearestPointResult class
 */
class NearestPointResult {
private:
    VertexPoint src_, dst_, target_;
public:
    /**
     * @brief NearestPointResult
     */
    NearestPointResult() : src_(), dst_(), target_() {;}
    /**
     * @brief NearestPointResult
     */
    NearestPointResult(const VertexPoint &src, const VertexPoint &dst, const VertexPoint &target)
        : src_(src), dst_(dst), target_(target) {;}
    /**
     * @brief isEndPoint
     * @return
     */
    bool isEndPoint() const {
        return src_.getId() == dst_.getId();
    }
    /**
     * @brief getStartVertex
     * @return
     */
    VertexPoint getSrc() const {
        return src_;
    }
    /**
     * @brief getEndVertex
     * @return
     */
    VertexPoint getDst() const {
        return dst_;
    }
    /**
     * @brief getTargetVertexPoint
     * @return
     */
    VertexPoint getTarget() const {
        return target_;
    }
    /**
     * @brief setStartVertexId
     * @param start
     */
    void setStartId(Vertex::VertexId start) {
        src_ = VertexPoint(start, -1, -1);
    }
    /**
     * @brief setEndVertexId
     * @param end
     */
    void setEndId(Vertex::VertexId end) {
        dst_ = VertexPoint(end, -1, -1);
    }
    /**
     * @brief setTargetVertex
     * @param vp
     */
    void setTarget(const VertexPoint &vp) {
        target_ = vp;
    }
    /**
     * @brief reverse
     * @return
     */
    NearestPointResult reverse() const {
        return NearestPointResult(dst_, src_, target_);
    }
};
} 
