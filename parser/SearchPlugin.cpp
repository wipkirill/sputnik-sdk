#include <UrbanLabs/Sdk/Storage/TagConsts.h>
#include <UrbanLabs/Sdk/Storage/SqlConsts.h>
#include "SearchPlugin.h"

using namespace std;

//--------------------------------------------------------------------------------------------------
// TagCompressor
//--------------------------------------------------------------------------------------------------
TagCompressor::TagCompressor() : currId_(0) {
    ;
}
/**
 * @brief TagCompressor::addKeyValue
 * @param key
 * @param value
 * @return
 */
pair<string,string> TagCompressor::addKeyValue(const string &key, const string &value) {
    // compress both tag and value
    pair<string, string> newKeyVal = {addString(key), value};
    if(tagFilter_.allowFixedKeyValue(key, value)) {
        newKeyVal.second = addString(value);
    }
    return newKeyVal;
}
/**
 * @brief TagCompressor::addKey
 * @param key
 * @return
 */
string TagCompressor::addString(const string &str) {
    auto it = strMap_.find(str);
    if(it == strMap_.end()) {
        string id = getCurrId();
        strMap_[str] = "q"+id+"q";
        currId_++;

        return "q"+id+"q";
    } else
        return it->second;
}
/**
 * @brief getCurrId
 * @return
 */
string TagCompressor::getCurrId() {
    return StringUtils::base36_encode(currId_);
}
/**
 * @brief TagCompressor::getTags
 * @return
 */
TagCompressor::StringMap TagCompressor::getTags() {
    return strMap_;
}
//--------------------------------------------------------------------------------------------------
// SearchPlugin
//--------------------------------------------------------------------------------------------------
/**
 * @brief SearchPlugin::SearchPlugin
 */
SearchPlugin::SearchPlugin(const string& outputFile) : Plugin(),
    outputFileName_(),tagsFileName_(), relFileName_(),
    tagsFileDescr_(0), relFileDescr_(0),totalTagsWritten_(0),totalRelTagsWritten_(0),
    totalkdTreeObjectsWritten_(0),totalkdTreeAddressWritten_(0)
{
    pluginId_ = "SEARCH_PLUGIN";
    outputFileName_ = outputFile;
    totalTagsWritten_ = 0;
    totalRelTagsWritten_ = 0;
    totalkdTreeObjectsWritten_ = 0;
    totalkdTreeAddressWritten_ = 0;

    // write objects to kdtree
    if (!kdTreeObjects_.openForBulkLoad(outputFileName_, SqlConsts::KDTREE_OBJECTS_INDEX_TABLE, false)) {
        die(pluginId_, "can't initialize object kdTree for bulk loading");
    }

    // initialize kd tree for multiple areas
    set<string> areas = tagFilter_.getAddressDecodable();
    for(const string & area : areas) {
        shared_ptr<KdTreeSql> tree(new KdTreeSql());
        string table = SqlConsts::KDTREE_ADDRESS_DECODER_TABLE+"_"+area;
        if(tree && !tree->openForBulkLoad(outputFileName_, table, false)) {
            LOGG(Logger::ERROR) << "Couldn't open addr decoder tree "+area << Logger::FLUSH;
            die(pluginId_, "can't initialize address kdTree for bulk loading");
        } else if(tree) {
            addrTree_.insert({area, tree});
        }
    }
}
/**
 * @brief SearchPlugin::~SearchPlugin
 */
SearchPlugin::~SearchPlugin() {
    ;
}
/**
 * @brief init
 * @param outputFile
 * @param edgFilter
 */
void SearchPlugin::init() {
    // initialize vertex ids
    freeInternalId_ = 0;

    idToOsmIdFileName_ = outputFileName_ + StringConsts::PT + SqlConsts::OSMID_TO_ID_TABLE;
    idToOsmIdFileDescr_ = fopen(idToOsmIdFileName_.c_str(), "a");

    // initialize tags file
    tagsFileName_ = outputFileName_ + StringConsts::PT + SqlConsts::OSM_TAG_TABLE;
    tagsFulltextFileName_ = tagsFileName_+"_fulltext";
    tagsHashFileName_ = tagsFileName_+"_tag_hash";
    tagsFixedFileName_ = tagsFileName_+"_tag_fixed";

    tagsFileDescr_ = fopen(tagsFileName_.c_str(), "a");
    tagsHashFileDescr_ = fopen(tagsHashFileName_.c_str(), "a");
    tagsFixedFileDescr_ = fopen(tagsFixedFileName_.c_str(), "a");
    tagsFulltextFileDescr_ = fopen(tagsFulltextFileName_.c_str(), "a");

    // create tables
    fprintf(tagsFileDescr_, "%s\n", SqlConsts::CREATE_OSM_TAG.c_str());
    fprintf(tagsFulltextFileDescr_, "%s\n", SqlConsts::CREATE_OSM_TAG_FULLTEXT.c_str());

    // initialize group relationships
    relFileName_ = outputFileName_ + StringConsts::PT + SqlConsts::OSM_RELATIONSHIP_TABLE;
    relFileDescr_ = fopen(relFileName_.c_str(), "a");

    // table containing vertex points
    vertexPointFileName_ = outputFileName_ + ".vp.sqlite";

    URL url(vertexPointFileName_);
    Properties props = {{"type", "sqlite"},{"table", SqlConsts::VP_TABLE},{"create", "0"}};
    if(!vpIndex_.open(url, props))
        die(pluginId_, "can't initialize vertexPoint database");
}
/**
 *
 */
template<typename T>
int SearchPlugin::rankObject(T *obj) {
    int tagCount = 0;
    for(int i = 0; i < obj->nTags; ++i) {
        string key = StringUtils::escape(obj->tags[i].key);
        string value = StringUtils::escape(obj->tags[i].value);

        if(tagFilter_.writeKeyValue(key, value)) {
            if(tagFilter_.acceptTag(key, value))
                tagCount++;
            if(tagFilter_.userEnteredTag(key))
                tagCount += 2;
            if(tagFilter_.isAddressTag(key))
                tagCount += 2;
            if(tagFilter_.isNameTag(key))
                tagCount += 30;
        }
        tagCount++;
    }
    return tagCount;
}
/**
 * @brief SearchPlugin::searializeTagValue
 * @param file
 * @param id
 * @param tag
 * @param value
 */
template<typename T>
void SearchPlugin::searializeTagValue(FILE *file, const T id, const string &tag, const string &value) {
    // printint for .read
    string stmt = "INSERT INTO "+SqlConsts::OSM_TAG_TABLE+" VALUES ";
    stmt.append("("+lexical_cast<VertexId>(id)+",\'"+StringUtils::escape(tag)+"\',\'"+StringUtils::escape(value)+"\');");
    fprintf(file, "%s\n", stmt.c_str());
    totalTagsWritten_++;
}
/**
 * @brief SearchPlugin::serializeWayMember
 */
void SearchPlugin::serializeWayMember(FILE *file, const Vertex::VertexId way, const Vertex::VertexId member, int order) {
    string sep = StringConsts::SEPARATOR;
    string spec = type_spec<VertexId>()+sep+type_spec<VertexId>()+sep+type_spec<int>()+"\n";
    fprintf(file, spec.c_str(), way, member, order);
}
/**
 * This function serializes all the possible fixed keys
 * and (tag, value) relationship tag=value as a single tag
 *
 * @brief SearchPlugin::serializeFullTextObject
 * @param obj
 */
template<typename T, typename IdType>
void SearchPlugin::serializeFullTextObject(const IdType id, const T *obj, const Point &pt) {
    set<string> tags;
    for(int i = 0; i < obj->nTags; ++i) {
        string key = StringUtils::escape(obj->tags[i].key);
        string value = StringUtils::escape(obj->tags[i].value);

        if(tagFilter_.acceptTag(key, value) && tagFilter_.validValue(key) && tagFilter_.validValue(value)) {
            if(!tagFilter_.isFullTextKeyValue(key, value)) continue;

            // fixed tag values and meta values
            if(tagFilter_.allowFixedKeyValue(key, value)) {
                if(tagFilter_.isValidFixedValue(value)) {
                    vector<string> tokens = tagFilter_.splitMultipleFixedKeyValue(value);
                    for(int j = 0; j < tokens.size(); j++) {
                        pair<string, string> canon = tagFilter_.getCanonicalTag(key, tokens[j]);
                        string nkey = canon.first+"="+canon.second;
                        string nkeyVal = tagCompr_.addString(nkey);

                        // insert compressed tags
                        tags.insert(nkeyVal);

                        // insert meta values
                        if(tagFilter_.isMetaKeyValue(key, value)) {
                            vector<string> tokens = tagFilter_.splitMultipleFixedKeyValue(canon.second);
                            for(int j = 0; j < tokens.size(); j++) {
                                tags.insert(tokens[j]);
                            }
                        }
                    }
                }
                if(tagFilter_.isFullText(key)) {
                    pair<string, string> canon = tagFilter_.getCanonicalTag(key, value);
                    tags.insert(canon.first);
                }
            }
            // user entered values like address, name and others
            else {
                // tokenator in the full text plugin will filter out the spaces and all the others
                if(tagFilter_.userEnteredTag(key) && tagFilter_.isValidUserKeyValue(key, value))
                    tags.insert(value);
            }

            if(tagFilter_.writeKeyValue(key, value)) {
                pair<string, string> canon = tagFilter_.getCanonicalTag(key, value);
                string nkey = tagCompr_.addString(canon.first);
                tags.insert(nkey);
            }
        }
    }

    // create all the required tags as a single stream
    string res = "";
    for(const string &tag : tags) {
        res.append(StringUtils::escape(tag)+" ");
    }
    if(res.size() > 0)
        res.resize(res.size()-1);

    int rank = rankObject(obj);
    uint64_t curve = curve_.convert(pt.lat(), pt.lon());
    string tagPrintSpec = type_spec<string>()+"\n";

    string stmt = "INSERT INTO "+SqlConsts::OSM_TAG_FULLTEXT_TABLE+"(docid, tokens, rank, curve) VALUES ";
    stmt.append("("+lexical_cast<IdType>(id)+",'"+res+"',"+lexical_cast(rank)+","+lexical_cast(curve)+");");
    fprintf(tagsFulltextFileDescr_, tagPrintSpec.c_str(), stmt.c_str());
}
/**
 * @brief SearchPlugin::serializeFixedKeyValue
 * @param file
 * @param type
 * @param keyval
 */
void SearchPlugin::serializeFixedTagValue(FILE *file, Vertex::VertexId id, const string &keyval) {
    string sep = StringConsts::SEPARATOR;
    string spec = type_spec<Vertex::VertexId>()+sep+type_spec<string>()+"\n";
    fprintf(file, spec.c_str(), id, keyval.c_str());
}
/**
 * @brief getOuterId
 */
string SearchPlugin::getOuterId(const string &type, const Vertex::VertexId id) {
    return type.substr(0, 1)+lexical_cast(id);
}
/**
 *
 */
Vertex::VertexId SearchPlugin::extractOuterId(const string &id) {
    return lexical_cast<VertexId>(id.substr(1));
}
/**
 * @brief getNextInternalId
 * @return
 */
Vertex::VertexId SearchPlugin::getInternalId(const string &type, const Vertex::VertexId id) {
    string outer = getOuterId(type, id), sep = StringConsts::SEPARATOR;
    if(intenalIdToOsmId_.count(outer) == 0) {
        string spec = type_spec<Vertex::VertexId>()+sep+type_spec<string>()+"\n";
        fprintf(idToOsmIdFileDescr_, spec.c_str(), freeInternalId_, outer.c_str());
        return intenalIdToOsmId_[outer] = freeInternalId_++;
    } else {
        return intenalIdToOsmId_[outer];
    }
}
/**
 * @brief node processes one osm node
 * @param node
 */
void SearchPlugin::notifyNode(OSMNode* node) {
    // output all tags for the node
    if(tagFilter_.acceptObject<OSMNode>(node)) {
        // store object in mapping table
        Vertex::VertexId internalId = getInternalId("node", node->nID);
        serializeFullTextObject(internalId, node, {node->dfLat, node->dfLon});

        // serialize non searchable tags
        int accepted = 0;
        for(int i = 0; i < node->nTags; ++i) {
            string key = node->tags[i].key;
            string value = node->tags[i].value;
            if(tagFilter_.acceptTag(key, value) && tagFilter_.validValue(key) && tagFilter_.validValue(value)) {
                key = StringUtils::escape(key), value = StringUtils::escape(value);
                pair<string, string> canon = tagFilter_.getCanonicalTag(key, value);

                // compress the key
                bool userEntered = tagFilter_.userEnteredTag(key);
                bool validUserEntered = tagFilter_.isValidUserKeyValue(key, value);
                if(!userEntered || (userEntered && validUserEntered)) {
                    // fixed tag vals get compressed in format tag=value
                    if(tagFilter_.allowFixedKeyValue(key, value)) {
                        string keyval = tagCompr_.addString(canon.first+"="+canon.second);
                        serializeFixedTagValue(tagsFixedFileDescr_, internalId, keyval);
                    } else {
                        // user entered have only key compression
                        canon.first = tagCompr_.addString(canon.first);
                        searializeTagValue(tagsFileDescr_, internalId, canon.first, canon.second);
                        accepted++;
                    }
                }
            }
        }

        // all the tags were compressed, so just mark the presence of the object
        if(accepted == 0) {
            string key = tagCompr_.addString(TagConsts::TAG_META);
            searializeTagValue(tagsFileDescr_, internalId, key, "dummy");
        }

        for(int i = 0; i < node->nTags; ++i) {
            if(tagFilter_.isAddressDecodable(node->tags[i].key)) {
                string area = tagFilter_.extractAdressAreaPrefix(node->tags[i].key);
                if(area != "" && addrTree_.find(area) != addrTree_.end()) {
                    VertexPoint vpt(internalId, node->dfLat, node->dfLon);
                    addrTree_[area]->insertBulk(vpt, StringConsts::SEPARATOR);
                    totalkdTreeAddressWritten_++;
                }
            }
        }

        objects_.insert(internalId);
    }
}
/**
 * @brief way
 * @param way
 */
void SearchPlugin::notifyWay(OSMWay* way) {
    // accept ways tags
    if(tagFilter_.acceptObject<OSMWay>(way)) {
        // store object in mapping table
        Vertex::VertexId internalId = getInternalId("way", way->nID);
        // serialize non searchable tag
        int accepted = 0;
        for(int i = 0; i < way->nTags; ++i) {
            string key = way->tags[i].key;
            string value = way->tags[i].value;
            if(tagFilter_.acceptTag(key, value) && tagFilter_.validValue(key) && tagFilter_.validValue(value)) {
                key = StringUtils::escape(key), value = StringUtils::escape(value);
                pair<string, string> canon = tagFilter_.getCanonicalTag(key, value);

                bool userEntered = tagFilter_.userEnteredTag(key);
                bool validUserEntered = tagFilter_.isValidUserKeyValue(key, value);
                if(!userEntered || (userEntered && validUserEntered)) {
                    if(tagFilter_.allowFixedKeyValue(key, value)) {
                        string keyval = tagCompr_.addString(canon.first+"="+canon.second);
                        serializeFixedTagValue(tagsFixedFileDescr_, internalId, keyval);
                    } else {
                        canon.first = tagCompr_.addString(canon.first);
                        searializeTagValue(tagsFileDescr_, internalId, canon.first, canon.second);
                        accepted++;
                    }
                }
            }
        }

        // all the tags were compressed, so just mark the presence of the object
        if(accepted == 0) {
            string key = tagCompr_.addString(TagConsts::TAG_META);
            searializeTagValue(tagsFileDescr_, internalId, key, "dummy");
        }

        // determine if we have all the points
        vector<VertexId> vid;
        for(int i = 0; i < way->nRefs; i++) {
            vid.push_back(way->nodeRefs[i]);
        }

        vector<Point> pts;
        vector<int8_t> found;
        if(!vpIndex_.getPoints(vid, pts, found)) {
            return;
        }

        // if self loop pick the center of mass and write with the id of the object that
        // was not written among objects (because it had no tags), if no such object is found
        // do not write group coordinates at all.
        Point location;
        Vertex::VertexId dummy = internalId;
        if(way->nodeRefs[0] == way->nodeRefs[way->nRefs-1]) {
            location = centerOfMass(pts);
        }
        // if not the center of mass, pick the median node
        else {
            location = pts[way->nRefs/2];
        }

        // serialize dummy point
        VertexPoint vpt(dummy, location);
        kdTreeObjects_.insertBulk(vpt, StringConsts::SEPARATOR);
        totalkdTreeObjectsWritten_++;

        // serialize address decoder
        for(int i = 0; i < way->nTags; ++i) {
            if(tagFilter_.isAddressDecodable(way->tags[i].key)) {
                string area = tagFilter_.extractAdressAreaPrefix(way->tags[i].key);
                if(area != "" && addrTree_.find(area) != addrTree_.end()) {
                    VertexPoint vpt(internalId, location.lat(), location.lon());
                    addrTree_[area]->insertBulk(vpt, StringConsts::SEPARATOR);
                    totalkdTreeAddressWritten_++;
                }
            }
        }

        // serialize full text object
        serializeFullTextObject(internalId, way, location);

        // for each node in the way indicate that it belongs to the group
        // set<VertexId> localSeen;
        // string sep = StringConsts::SEPARATOR;
        // string relPrintSpec = type_spec<Edge::EdgeId>()+sep+type_spec<VertexId>()+sep+type_spec<int>()+"\n";
        // for(unsigned int i = 0, order = 0; i < way->nRefs; ++i) {
        //     VertexId memInnerId = getInternalId("node", way->nodeRefs[i]);
        //     if(localSeen.find(memInnerId) == localSeen.end()) {
        //         if(objects_.count(memInnerId)) {
        //             serializeWayMember(relFileDescr_, internalId, memInnerId, order);
        //             localSeen.insert(memInnerId);
        //             totalRelTagsWritten_++;
        //             order++;
        //         }
        //     }
        // }
    }
}
/**
 * @brief SearchPlugin::notifyRelation
 * @param rel
 */
void SearchPlugin::notifyRelation(OSMRelation* /*rel*/) {
    ;
}
/**
 * @brief finalize
 */
void SearchPlugin::finalize() {
    // put all the object to the kdtree
    for(auto id : intenalIdToOsmId_) {
        if(id.first[0] == 'n') {
            Point pt;
            if(vpIndex_.getPoint(extractOuterId(id.first), pt)) {
                // store only nodes with some tags, because there are many members
                // of ways without any meaningful data
                if(objects_.count(id.second)) {
                    VertexPoint vpt(id.second, pt);
                    kdTreeObjects_.insertBulk(vpt, StringConsts::SEPARATOR);
                    totalkdTreeObjectsWritten_++;
                }
            } else {
                die(pluginId_, "Couldn't find a point in vpIndex");
            }
        }   
    }

    // serialize all the tags
    string sep = StringConsts::SEPARATOR;
    string tagPrintSpec = type_spec<string>()+sep+type_spec<string>()+"\n";
    for(const pair<string,string> &pr : tagCompr_.getTags()) {
        fprintf(tagsHashFileDescr_, tagPrintSpec.c_str(), pr.first.c_str(), pr.second.c_str());
    }

    kdTreeObjects_.closeBulk();
    for(auto tree : addrTree_) {
        tree.second->closeBulk();
    }
    closeFiles();
}
/**
 * @brief validate called when plugin is done
 */
void SearchPlugin::validate() {
    SqlStream sStream;
    URL url(outputFileName_);
    Properties props = {{"type", "sqlite"}, {"create", "0"}};
    sStream.open(url, props);

    size_t rCount = sStream.getNumRows(SqlConsts::OSM_TAG_TABLE);
    if (rCount != totalTagsWritten_)
        die(pluginId_, "Wrong row count in tags "+lexical_cast(rCount)+", expected "+
                                        lexical_cast(totalTagsWritten_));

    rCount = sStream.getNumRows(SqlConsts::OSM_RELATIONSHIP_TABLE);
    if (rCount != totalRelTagsWritten_)
        die(pluginId_, "Wrong row count in grouprel "+lexical_cast(rCount)+", expected "
                                      +lexical_cast(totalRelTagsWritten_));

    rCount = sStream.getNumRows(SqlConsts::KDTREE_OBJECTS_INDEX_TABLE);
    if (rCount != totalkdTreeObjectsWritten_)
        die(pluginId_, "Wrong row count in kdtreeobjects "+lexical_cast(rCount)+", expected "
                                      +lexical_cast(totalkdTreeObjectsWritten_));

    rCount = 0;
    for(auto tree : addrTree_) {
        rCount += sStream.getNumRows(SqlConsts::KDTREE_ADDRESS_DECODER_TABLE+"_"+tree.first);
    }
    if (rCount != totalkdTreeAddressWritten_)
        die(pluginId_, "Wrong row count in kdtreeaddress "+lexical_cast(rCount)+", expected "
                                      +lexical_cast(totalkdTreeAddressWritten_));
}
/**
 * @brief cleanUp
 */
void SearchPlugin::cleanUp() {
    // base tables
    remove(idToOsmIdFileName_.c_str());
    remove(tagsFileName_.c_str());
    remove(relFileName_.c_str());
    // fixed tags tables
    remove(tagsFixedFileName_.c_str());
    // full text tables
    remove(tagsFulltextFileName_.c_str());
    // tag compression tables
    remove(tagsHashFileName_.c_str());
    // kdtrees
    kdTreeObjects_.cleanUp();
    for(auto tree : addrTree_) {
        tree.second->cleanUp();
    }
}
/**
 * @brief closeFiles
 */
void SearchPlugin::closeFiles() {
    if(idToOsmIdFileDescr_ != 0)
        fclose(idToOsmIdFileDescr_);
    if(tagsFileDescr_ != 0)
        fclose(tagsFileDescr_);
    if(relFileDescr_ != 0)
        fclose(relFileDescr_);
    // full text index tables
    if(tagsFulltextFileDescr_ != 0)
        fclose(tagsFulltextFileDescr_);
    // fixed tags tables
    if(tagsFixedFileDescr_ != 0)
        fclose(tagsFixedFileDescr_);
    // tag compression
    if(tagsHashFileDescr_ != 0)
        fclose(tagsHashFileDescr_);
}
/**
 * @brief SearchPlugin::getTableNamesToImport
 * @return
 */
vector<string> SearchPlugin::getTableNamesToImport() const {
    // create address decoder areas
    vector<string> addrTables;
    for(auto area : addrTree_) {
        addrTables.push_back(SqlConsts::KDTREE_ADDRESS_DECODER_TABLE+"_"+area.first);
    }

    vector<string> defaultTables = {
            //osm id
            SqlConsts::OSMID_TO_ID_TABLE,
            // osm
            SqlConsts::OSM_TAG_HASH_TABLE,
            SqlConsts::OSM_TAG_FIXED_TABLE,
            // relationships
            SqlConsts::OSM_RELATIONSHIP_TABLE,
            // kdtrees
            SqlConsts::KDTREE_OBJECTS_INDEX_TABLE};

    defaultTables.insert(defaultTables.end(), addrTables.begin(), addrTables.end());
    return defaultTables;
}
/**
 * @brief SearchPlugin::getSqlToCreateTables
 * @return
 */
vector<string> SearchPlugin::getSqlToCreateTables() const {
    // construct sql to create address decoder
    vector<string> addrSql;
    for(auto area : addrTree_) {
        string in = SqlConsts::CREATE_ADDRESS_DECODER_KDTREE_INDEX;
        string replace = SqlConsts::KDTREE_ADDRESS_DECODER_TABLE;
        string with = SqlConsts::KDTREE_ADDRESS_DECODER_TABLE+"_"+area.first;
        in = StringUtils::replaceAll(in, replace, with);
        addrSql.push_back(in);
    }

    vector<string> defaultTables = {
            // osm id
            SqlConsts::CREATE_OSMID_TO_ID_TABLE,
            // osm
            SqlConsts::CREATE_OSM_TAG_HASH,
            SqlConsts::CREATE_OSM_TAG_FIXED,
            // relationships
            SqlConsts::CREATE_OSM_RELATIONSHIPS,
            // kdtrees
            SqlConsts::CREATE_OBJECT_KDTREE_INDEX,
            SqlConsts::CREATE_ADDRESS_DECODER_KDTREE_INDEX};

    defaultTables.insert(defaultTables.end(), addrSql.begin(), addrSql.end());
    return defaultTables;
}
/**
 * @brief SearchPlugin::getOtherSqlCommands
 * @return
 */
vector<string> SearchPlugin::getOtherSqlCommands() const {
    return { // update registers
            SqlConsts::UPDATE_TAGS_LOWER_CASE,
            // optimize
            SqlConsts::OSM_TAG_FULLTEXT_TABLE_OPTIMIZE,
            // osm
            SqlQuery::ci(SqlConsts::OSM_TAG_TABLE,{"objectid","tag","value"}),
            SqlQuery::ci(SqlConsts::OSM_TAG_HASH_TABLE, {"tag","hash"}),
            SqlQuery::ci(SqlConsts::OSM_TAG_HASH_TABLE, {"hash","tag"}),
            // relationships
            SqlQuery::ci(SqlConsts::OSM_RELATIONSHIP_TABLE,{"objectid"}),
            SqlQuery::ci(SqlConsts::OSM_RELATIONSHIP_TABLE,{"groupid"})};
}
/**
 * @brief SearchPlugin::getFileNamesToRead
 * @return
 */
vector<string> SearchPlugin::getFileNamesToRead() const {
    // initialize tags file
    string tagsFileName = outputFileName_ + StringConsts::PT + SqlConsts::OSM_TAG_TABLE;
    string tagsFulltextFileName = tagsFileName+"_fulltext";

    return {tagsFileName, tagsFulltextFileName};
}
