#include <vector>
#include <UrbanLabs/Sdk/OSM/TagFilter.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/Storage/AddressDecoder.h>
#include <UrbanLabs/Sdk/Storage/QueryConditions.h>

using std::set;
using std::map;
using std::string;
using std::vector;
using std::unique_ptr;

/**
 * @brief AddressDecoder::Initializer::Initializer
 */
AddressDecoder::Initializer::Initializer(const URL &url, const Properties &props)
    : url_(url), props_(props) {
    ;
}
/**
 * @brief AddressDecoder::Initializer::init
 * @param storage
 */
bool AddressDecoder::Initializer::init(AddressDecoder &storage) const {
    return storage.open(url_, props_);
}
/**
 * @brief AddressDecoder::Destructor::release
 * @param storage
 */
bool AddressDecoder::Destructor::release(AddressDecoder &storage) {
    storage.close();
    return true;
}
/**
 * @brief AddressDecoder::AddressDecoder 
 */
AddressDecoder::AddressDecoder() {
    // distance from neighbor type that can be accepted as
    // a good approximate to the given one
    dist_ = {{"country", 5e4},       // 50km
             {"state", 1e4},         // 10km
             {"city", 1e3},          // 1km
             {"suburb", 5e2},        // 500m
             {"district", 5e2},      // 500m
             {"postcode", 2e2},      // 200m
             {"zip", 2e2},           // 200m
             {"street", 5e1},        // 50m
             {"housenumber", 2e1}};  // 20m
}
/**
 * @brief AddressDecoder::open
 * @param db
 */
bool AddressDecoder::open(const URL &url, const Properties &props) {  
    bool ok = false;
    string addrTable = props.get("table");
    for(auto area : dist_) {
        Properties treeProps = props;
        treeProps.set("table", addrTable+"_"+area.first);
        
        shared_ptr<KdTreeSql> tree(new KdTreeSql());
        if(tree && !tree->open(url, treeProps, false))
            LOGG(Logger::WARNING) << "Couldn't open addr decoder tree "+area.first << Logger::FLUSH; 
        else if(tree) {
            addrTree_.insert({area.first, tree});
            ok = true;
        }
    }

    tagTable_ = props.get("tagtable");
    Properties tagProps = {{"type", props.get("type")}, {"table", props.get("tagtable")}};
    if(!tags_.open(url, tagProps)) {
        return false;
    }

    return ok;
}
/**
 * @brief AddressDecoder::close
 */
bool AddressDecoder::close() {
    tagTable_ = "";
    addrTree_.clear();
    if(!tags_.close())
        return false;
    return true;
}
/**
 * @brief GeoRouting::filterTagList
 */
template<typename Filter>
TagList AddressDecoder::filterTagList(TagList & tagList, Filter filter) {
    TagList newTags;
    newTags.setId(tagList.getId());
    newTags.setPoint(tagList.getPoint());
    vector<KeyValuePair> tags = tagList.getTags();
    for(KeyValuePair &kv : tags) {
        if(filter(kv.getKey()))
             newTags.add(kv);
    }
    return newTags;
}
/**
 * @brief AddressDecoder::resolve
 */
bool AddressDecoder::resolve(const Point &pt, TagList &tags) {
    bool ok = false;
    for(auto &tree : addrTree_) {
        string tag = tree.first;

        string data;
        OsmGraphCore::NearestPointResult res;
        if(tree.second) {
            if(!tree.second->findNearestVertex(pt, res, data)) {
                LOGG(Logger::WARNING) << "Couldn't find "+tag << " for " << pt << Logger::FLUSH; 
            } else {
                if(goodNeighbor(pt, res.getTarget().getPoint(), tree.first)) {
                    ConditionContainer cont;
                    cont.addIdsIn({res.getTarget().getId()});

                    vector<TagList> areaTags;
                    if(tags_.simpleSearch(tagTable_, cont, 0, 1, areaTags)) {
                        areaTags[0] = filterTagList(areaTags[0], [=] (const string &t) -> bool {
                                        return t.find("addr:"+tag) != string::npos;});
                        vector<KeyValuePair> keVals = areaTags[0].getTags();
                        for(KeyValuePair &p : keVals) {
                            tags.add(p);
                        }
                        ok = true;
                    }
                }
            }
        }
    }
    return ok;
}
/**
 * @brief AddressDecoder::goodNeighbor
 */
bool AddressDecoder::goodNeighbor(const Point &pt, const Point &neigh, const string &type) {
    return pointDistance(pt, neigh) <= dist_[type];
}