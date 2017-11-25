#pragma once

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/GraphCore/Point.h>
#include <UrbanLabs/Sdk/Utils/Types.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Utils/StringUtils.h>

/**
 * @brief The Vertex class
 * class representing graph vertex
 */
class Vertex {
public:
    typedef int64_t VertexId;
protected:
    VertexId id_;
public:
    static const VertexId NullVertexId;
public:
    Vertex();
    Vertex(VertexId id);
    VertexId getId() const;
    void setId(VertexId id);
    bool operator == (const Vertex &v) const;
    bool operator != (const Vertex &v) const;
};

/**
 * @brief The VertexPoint class
 */
class VertexPoint {
public:
    typedef Vertex::VertexId VertexId;
    typedef Point::CoordType CoordType;
private:
    Point pt_;
    Vertex ver_;
public:
    /**
     * @brief VertexPoint
     */
    VertexPoint();
    VertexPoint(const Vertex &ver, const Point &pt);
    VertexPoint(Vertex::VertexId id, CoordType lat, CoordType lon);
    VertexId getId() const;
    Point getPoint() const;
    Vertex getVertex() const;
    std::ostream& serialize(std::ostream& oss) const;
    void serializeToFile(FILE* file) const;
    template <typename T>
    T& deserialize(T& iss) {
        VertexId id;
        iss >> id >> pt_;
        ver_.setId(id);
        return iss;
    }
    friend std::ostream& operator << (std::ostream& os, const VertexPoint& v);
    bool operator == (const Vertex &v1) const;
};

namespace std {
    // specialize hashing function for vertices
    template<>
    class hash<Vertex> {
    public:
        size_t operator()(const Vertex &v) const {
            return hash<Vertex::VertexId>()(v.getId());
        }
    };

    template<>
    class hash<VertexPoint> {
    public:
        size_t operator()(const VertexPoint &v) const {
            return hash<Vertex::VertexId>()(v.getId());
        }
    };
}
