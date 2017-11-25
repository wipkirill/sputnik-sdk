#pragma once

#include <map>
#include <algorithm>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>
#include <UrbanLabs/Sdk/Output/JsonRouteFormatter.h>

/**
 * @brief The KeyValuePair class
 */
class KeyValuePair {
private:
    std::string key_;
    std::string value_;
public:
    std::string getKey() const;
    std::string getValue() const;
    KeyValuePair(const std::string &key, const std::string &value);
    std::string toJSON() const;
    void setKey(const std::string &key);
};

/**
 * @brief The Tag class
 */
class Tag {
public:
    typedef Vertex::VertexId VertexId;
private:
    typedef JSONFormatterNode Node;
    typedef JSONFormatterNode::Nodes Nodes;
private:
    VertexId objId_;
    Point pt_;
    std::string key_;
    std::string value_;
public:
    Tag();
    Tag(const Vertex::VertexId id, const std::string &key, const std::string &value);
    std::string getKey() const;
    std::string getValue() const;
    template <typename T>
    T &deserialize(T &t);
    std::string toJSON() const;
};

/**
 * @brief The TagList class
 */
class TagList {
public:
    typedef Vertex::VertexId VertexId;
protected:
    typedef JSONFormatterNode Node;
    typedef JSONFormatterNode::Nodes Nodes;
protected:
    bool pointSet_;
    VertexId objId_;
    std::vector<KeyValuePair> tags_;
    std::vector<Vertex::VertexId> objects_;
    Point pt_;
public:
    TagList();
    TagList(const Vertex::VertexId id);
    VertexId getId() const;

    bool hasKey(const std::string &key) const;
    std::string getValue(const std::string &key) const;
    std::vector<KeyValuePair> getTags() const;
    Point getPoint() const;
    bool hasPoint() const;

    JSONFormatterNode toJSONFormatterNode();
    void setObjects(const std::vector<Vertex::VertexId> &ids);
    std::vector<Vertex::VertexId> getObjects() const;
    void add(const KeyValuePair &kvp);
    void setId(const VertexId &id);
    void setPoint(const Point &pt);
    std::string toJSON();
    void clearTags();
};
/**
 * @brief The RouteTagList class
 * TODO: put VertexGtfs here instead
 */
class GtfsTagList : public TagList {
public:
    typedef Vertex::VertexId VertexId;
public:
    GtfsTagList();
    GtfsTagList(const Vertex::VertexId id);
    std::string toJSON() const;
    JSONFormatterNode toJSONFormatterNode() const;
    std::vector<KeyValuePair> getTags() const;
    VertexId getId() const;
    void setId(GtfsTagList::VertexId &id);
    Point getPoint() const;
};
/**
 *
 */
template<typename T>
class TagListComparator {
public:
    std::map<Vertex::VertexId, int> order_;
    TagListComparator(const std::map<Vertex::VertexId, int> &order) : order_(order) {;}
    bool operator () (const T &t1, const T &t2) const {
        return order_.find(t1.getId())->second < order_.find(t2.getId())->second;
    }
};

template <typename T>
T &Tag::deserialize(T &t) {
    t >> objId_ >> pt_ >>  key_ >> value_;
}
