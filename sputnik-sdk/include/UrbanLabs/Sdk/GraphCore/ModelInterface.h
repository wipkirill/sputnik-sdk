#pragma once

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/GraphCore/Point.h>

// interface for storing edges of the graph
template<typename M>
class Model {
public:
    Model<M>() { ; }

    /**
     * @brief parse
     * @param filename
     */
    bool parse(const std::string &filename);

    /**
     * @brief unload
     */
    void unload();

    /**
     * preprocess if needed, pass a reference to a graph of this model
     */
    void preprocess();

    /**
     * get an intermediate vertex
     */
    template<typename V, typename T>
    typename V::VertexId getViaForEdge(V from, V to) const;

    /**
     * find geometry for edge
     */
    template<typename V>
    void findGeometryForEdge(const typename V::VertexId source, const typename V::VertexId dest,
                             Point target, std::vector<Point> &geometry) const;

    /**
     * get point
     */
    template<typename V>
    Point getPoint(const V &v) const;

    /**
     * @brief getNumVertices
     * @return
     */
    size_t getNumVertices() const;

    /**
     * get outgoing edge iterators
     */
    template<typename V, typename T>
    T getOutgoingIterBegin(V v) {
        return static_cast<M*>(this)->getOutgoingIterBegin(v);
    }

    /**
     *
     */
    template<typename V, typename T>
    T getOutgoingIterEnd(V v) {
        return static_cast<M*>(this)->getOutgoingIterEnd(v);
    }

    /**
     * get incoming edge iterators
     */
    template<typename V, typename T>
    T getIncomingIterBegin(V v) {
        return static_cast<M*>(this)->getIncomingIterBegin(v);
    }

    /**
     *
     */
    template<typename V, typename T>
    T getIncomingIterEnd(V v) {
        return static_cast<M*>(this)->getIncomingIterEnd(v);
    }
};
