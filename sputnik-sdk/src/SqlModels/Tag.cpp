// Tag.cpp
//

#include <UrbanLabs/Sdk/SqlModels/Tag.h>

using namespace std;

/**
 * @brief KeyValuePair::KeyValuePair
 * @param key
 * @param value
 */
KeyValuePair::KeyValuePair(const string &key, const string &value) : key_(key), value_(value) {
    ;
}

/**
 * @brief KeyValuePair::toJSON
 * @return
 */
string KeyValuePair::toJSON() const {
    JSONFormatterNode tag(key_, value_);
    return tag.toString();
}
/**
 * @brief setKey
 * @param key
 */
void KeyValuePair::setKey(const std::string &key) {
   key_ = key;
}
/**
 * @brief KeyValuePair::getKey
 * @return
 */
string KeyValuePair::getKey() const {
    return key_;
}
/**
 * @brief KeyValuePair::getValue
 * @return
 */
string KeyValuePair::getValue() const {
    return value_;
}
/**
 * @brief compareByTag
 * @param kv1
 * @param kv2
 * @return
 */
bool compareByTag(const KeyValuePair &kv1, const KeyValuePair &kv2) {
    return kv1.getKey() < kv2.getKey();
}

/**
 * @brief compareByValue
 * @param kv1
 * @param kv2
 * @return
 */
bool compareByValue(const KeyValuePair &kv1, const KeyValuePair &kv2) {
    return kv1.getValue() < kv2.getValue();
}

/**
 * @brief Tag::Tag
 */
Tag::Tag() : objId_(-1), pt_(), key_(), value_() {
    ;
}
/**
 * @brief Tag::Tag
 * @param id
 * @param key
 * @param value
 */
Tag::Tag(const Vertex::VertexId id, const string &key, const string &value) : objId_(id), pt_(), key_(key), value_(value) {
    ;
}
/**
 * @brief Tag::getKey
 * @return
 */
string Tag::getKey() const {
    return key_;
}
/**
 * @brief Tag::getValue
 * @return
 */
string Tag::getValue() const {
    return value_;
}
/**
 * @brief Tag::toJSON
 * @return
 */
string Tag::toJSON() const {
    Node node;
    Nodes nodes = {Node("id", objId_), Node("lat", pt_.lat()), Node("lon", pt_.lon()), Node("key", key_), Node("value", value_)};
    node.add(nodes);
    return node.toString();
}

/**
 * @brief TagList
 */
TagList::TagList() : objId_(-1), tags_(), pt_(){
    ;
}

/**
 * @brief TagList
 * @param id
 */
TagList::TagList(const Vertex::VertexId id)
    : pointSet_(false), objId_(id), tags_(), pt_() {
    ;
}

/**
 * @brief add
 * @param kvp
 */
void TagList::add(const KeyValuePair &kvp) {
    tags_.push_back(kvp);
}
/**
 * @brief GroupTagList::setObjects
 * @param ids
 */
void TagList::setObjects(const vector<Vertex::VertexId> &ids) {
    objects_ = ids;
}
/**
 * @brief GroupTagList::getObjects
 * @param ids
 */
vector<Vertex::VertexId> TagList::getObjects() const {
    return objects_;
}
/**
 * @brief setPoint
 * @param pt
 */
void TagList::setPoint(const Point &pt) {
    pt_ = pt;
    pointSet_ = true;
}
/**
 * @brief TagList::hasKey
 * @param key
 * @return
 */
bool TagList::hasKey(const string &key) const {
    for (const KeyValuePair &kv : tags_) {
        if (kv.getKey() == key)
            return true;
    }
    return false;
}
/**
 * @brief TagList::getId
 * @return
 */
TagList::VertexId TagList::getId() const {
    return objId_;
}
/**
 * @brief setId
 */
void TagList::setId(const TagList::VertexId &id) {
    objId_ = id;
}
/**
 * @brief TagList::getValue
 * @param key
 * @return
 */
string TagList::getValue(const string &key) const {
    for (const KeyValuePair &kv : tags_) {
        if (kv.getKey() == key)
            return kv.getValue();
    }
    return "";
}
/**
 * @brief getTags
 * @return
 */
vector<KeyValuePair> TagList::getTags() const {
    return tags_;
}
/**
 * @brief toJSON
 * @return
 */
string TagList::toJSON() {
    Node node = toJSONFormatterNode();
    return node.toString();
}
/**
 * @brief TagList::clear
 */
void TagList::clearTags() {
    tags_.clear();
}
/**
 * @brief TagList::toJSON
 * @return
 */
JSONFormatterNode TagList::toJSONFormatterNode() {
    Nodes tmp;
    if(tags_.size() > 0) {
        sort(tags_.begin(), tags_.end(), compareByTag);

        vector<string> values;
        string prevKey = tags_[0].getKey();
        for(int i = 0; i < tags_.size(); i++) {
            if(tags_[i].getKey() != prevKey) {
                if(values.size() == 1)
                    tmp.push_back(Node(prevKey, values[0]));
                else
                    tmp.push_back(Node(prevKey, values));
                prevKey = tags_[i].getKey();
                values = {tags_[i].getValue()};
            } else {
                values.push_back(tags_[i].getValue());
            }

            if(i+1 == tags_.size()) {
                if(values.size() == 1)
                    tmp.push_back(Node(tags_[i].getKey(), values[0]));
                else
                    tmp.push_back(Node(tags_[i].getKey(), values));
            }
        }
    }

    Node tagList("tags");
    tagList.add(tmp);

    Node node;
    if(pointSet_) {
        Nodes nodes = {Node("id", objId_), Node("lat", pt_.lat()), Node("lon", pt_.lon()), tagList};
        node.add(nodes);
    } else {
        Nodes nodes = {Node("id", objId_), tagList};
        node.add(nodes);
    }

    if(objects_.size() > 0)
        node.add({Node("objects", objects_)});

    return node;
}
/**
 * @brief TagList::getPoint
 * @return
 */
Point TagList::getPoint() const {
    return pt_;
}
/**
 * @brief TagList::hasPoint
 * @return
 */
bool TagList::hasPoint() const {
    return pointSet_;
}
/**
 * @brief RouteTagList::RouteTagList
*/
GtfsTagList::GtfsTagList() :TagList() {
    ;
}
/**
 * @brief GtfsTagList::GtfsTagList
 * @param id
 */
GtfsTagList::GtfsTagList(const Vertex::VertexId id) : TagList(id)  {
    ;
}

/**
 * @brief setId
 */
GtfsTagList::VertexId GtfsTagList::getId() const {
    return objId_;
}

/**
 * @brief setId
 */
void GtfsTagList::setId(GtfsTagList::VertexId &id) {
    objId_ = id;
}

/**
 * @brief RouteTagList::toJSON
 * @return
 */
string GtfsTagList::toJSON() const {
    Node node = toJSONFormatterNode();
    return node.toString();
}

/**
 * @brief GtfsTagList::getPoint
 * @return
 */
Point GtfsTagList::getPoint() const {
    return pt_;
}
/**
 * @brief RouteTagList::toJSONFormatterNode
 * @return
 */
JSONFormatterNode GtfsTagList::toJSONFormatterNode() const {
    Nodes tmp;
    for(int i = 0; i < tags_.size(); i++) {
        tmp.push_back(Node(tags_[i].getKey(), tags_[i].getValue()));
    }
    Node tagList("tags");
    tagList.add(tmp);

    Node node;
    Nodes nodes = {Node("id", objId_), tagList};
    node.add(nodes);
    return node;
}
/**
 * @brief getTags
 * @return
 */
vector<KeyValuePair> GtfsTagList::getTags() const {
    return tags_;
}
