#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Storage/SqlConsts.h>

using namespace std;

// tags
const string SqlConsts::TAGS_TABLE = "osm_tag";
// graph tables
const string SqlConsts::VERTICES_TABLE = "vertex";
const string SqlConsts::EDGES_TABLE = "edge";
const string SqlConsts::KDTREE_ENDPT_TABLE = "kdtree_endpt";
const string SqlConsts::KDTREE_NON_ENDPOINT_TABLE = "kdtree_nonendpt";
// nearest neighbor for graph
const string SqlConsts::GEOMETRY_TABLE = "geometry";
const string SqlConsts::VP_TABLE = "vertexpoint";
// routing
const string SqlConsts::TURN_RESTRICTIONS = "turn_restrictions";
// address decoding
const string SqlConsts::KDTREE_ADDRESS_DECODER_TABLE = "addrdecoder";
const string SqlConsts::ADDRESS_DECODER_AREAS_TABLE = "addrdecoder_areas";
const string SqlConsts::KDTREE_OBJECTS_INDEX_TABLE = "objectkdtree";
// osm id
const string SqlConsts::OSMID_TO_ID_TABLE = "osm_id";
// search tags
const string SqlConsts::OSM_TAG_TABLE = "osm";
const string SqlConsts::OSM_TAG_HASH_TABLE = SqlConsts::OSM_TAG_TABLE + "_tag_hash";
const string SqlConsts::OSM_TAG_FIXED_TABLE = SqlConsts::OSM_TAG_TABLE + "_tag_fixed";
const string SqlConsts::OSM_TAG_FULLTEXT_TABLE = SqlConsts::OSM_TAG_TABLE + "_fulltext";

const string SqlConsts::OSM_RELATIONSHIP_TABLE = "osm_rel";
// gtfs tags
const string SqlConsts::GTFS_TAGS_TABLE = "gtfs";
const string SqlConsts::GTFS_GROUP_TAG_TABLE = "gtfs_group";
const string SqlConsts::GTFS_GROUPS_RELATIONSHIP_TABLE = "gtfs_rel";
const string SqlConsts::MAP_INFO_TABLE = "map_info";
// gtfs
const string SqlConsts::GTFS_AGENCY = "gtfs_agency";
const string SqlConsts::GTFS_ROUTE = "gtfs_route";
const string SqlConsts::GTFS_TRIP = "gtfs_trip";
const string SqlConsts::GTFS_STOPTIME = "gtfs_stoptime";
const string SqlConsts::GTFS_CALENDAR = "gtfs_calendar";
const string SqlConsts::GTFS_SHAPE = "gtfs_shape";
// config
const string SqlConsts::CONFIG_TABLE = "config";

//-------------------------------SQLITE QUERIES---------------------------------
// RULE OF THUMB:
//  - when there is an integral primary key, always name it as INTEGER PRIMARY KEY (not INT, UNSIGNED INT or BIGINT)
//    this has serious performance considerations
//  - when there is a non-integer primary key always use WITHOUT ROWID (this reduces space)

// tags table
const string SqlConsts::CREATE_TAGS_TABLE = "CREATE TABLE " + SqlConsts::TAGS_TABLE + " (en NVARCHAR(256));";

// address decoder
const string SqlConsts::CREATE_ADDRESS_DECODER_KDTREE_INDEX = "CREATE VIRTUAL TABLE " + SqlConsts::KDTREE_ADDRESS_DECODER_TABLE + " USING rtree(verid INTEGER PRIMARY KEY, minlat INT, maxlat INT, minlon INT, maxlon INT);";
const string SqlConsts::CREATE_ADDRESS_DECODER_AREA_INDEX = "CREATE TABLE " + SqlConsts::ADDRESS_DECODER_AREAS_TABLE + "(id INTEGER PRIMARY KEY , areatype NVARCHAR(256), level INT, name NVARCHAR(256), "
                                                                                                                       " parid INTEGER, hl_lat REAL, hl_lon REAL, lr_lat REAL, lr_lon REAL);";

// graph core
const string SqlConsts::CREATE_KDTREE_ENDPT = "CREATE VIRTUAL TABLE " + SqlConsts::KDTREE_ENDPT_TABLE + " USING rtree(verid INTEGER PRIMARY KEY,"
                                                                                                        "minlat INT, maxlat INT, minlon INT, maxlon INT);";

const string SqlConsts::CREATE_KDTREE_NONENDPT = "CREATE VIRTUAL TABLE " + SqlConsts::KDTREE_NON_ENDPOINT_TABLE + " USING rtree(verid INTEGER PRIMARY KEY,"
                                                                                                                  "minlat INT, maxlat INT, minlon INT, maxlon INT);";
const string SqlConsts::CREATE_KDTREE_NONENDPT_DATA = "CREATE TABLE " + SqlConsts::KDTREE_NON_ENDPOINT_TABLE + "_data" + "(verid INTEGER PRIMARY KEY,"
                                                                                                                         "data TEXT);";

const string SqlConsts::CREATE_VERTICES = "CREATE TABLE " + SqlConsts::VERTICES_TABLE + " (verid INTEGER PRIMARY KEY, lat REAL, lon REAL);";
const string SqlConsts::CREATE_EDGES = "CREATE TABLE " + SqlConsts::EDGES_TABLE + "(startid INTEGER, endid INTEGER,"
                                                                                  "via INTEGER, cost INT, timecost INT, type INT, origid INTEGER);";

const string SqlConsts::CREATE_TURN_RESTRICTIONS = "CREATE TABLE " + SqlConsts::TURN_RESTRICTIONS + "(edgefrom INT, edgeto INT, vervia INT, type NVARCHAR(256));";

// vertex points
const string SqlConsts::CREATE_VP_TABLE = "CREATE TABLE " + SqlConsts::VP_TABLE + "(vertexid INTEGER PRIMARY KEY, lat REAL, lon REAL);";
const string SqlConsts::INSERT_VP = "INSERT INTO " + SqlConsts::VP_TABLE + " VALUES (?, ?, ?);";

// geometry
const string SqlConsts::CREATE_GEOMETRY = "CREATE TABLE " + SqlConsts::GEOMETRY_TABLE + "(ver1 INTEGER, ver2 INTEGER, geom BLOB);";
const string SqlConsts::INSERT_GEOMETRY = "INSERT INTO " + SqlConsts::GEOMETRY_TABLE + " VALUES (?, ?, ?);";

// objects
const string SqlConsts::CREATE_OBJECT_KDTREE_INDEX = "CREATE VIRTUAL TABLE " + SqlConsts::KDTREE_OBJECTS_INDEX_TABLE + " USING rtree(objectid INTEGER PRIMARY KEY,"
                                                                                                                       "minlat INT, maxlat INT, minlon INT, maxlon INT);";
// osm ids
const string SqlConsts::CREATE_OSMID_TO_ID_TABLE = "CREATE TABLE " + SqlConsts::OSMID_TO_ID_TABLE + "(internal_id INTEGER PRIMARY KEY, osm_id NVARCHAR(11) NOT NULL);";
// osm tags
const string SqlConsts::CREATE_OSM_TAG = "CREATE TABLE " + SqlConsts::OSM_TAG_TABLE + "(objectid INTEGER, tag NVARCHAR(256), value NVARCHAR(256));";
const string SqlConsts::CREATE_OSM_TAG_FULLTEXT = "CREATE VIRTUAL TABLE "+SqlConsts::OSM_TAG_FULLTEXT_TABLE+" USING fts4(content=\"\",tokens NVARCHAR(256), rank INT, curve INT, tokenize=unicode61 \"remove_diacritics=1\" \"tokenchars=#&+\");";
const string SqlConsts::CREATE_OSM_TAG_HASH = "CREATE TABLE " + SqlConsts::OSM_TAG_HASH_TABLE + "(tag NVARCHAR(256), hash NVARCHAR(256));";
const string SqlConsts::CREATE_OSM_TAG_FIXED = "CREATE TABLE " + SqlConsts::OSM_TAG_FIXED_TABLE + "(objectid INTEGER, tag NVARCHAR(256));";
const string SqlConsts::OSM_TAG_FULLTEXT_TABLE_OPTIMIZE = "INSERT INTO " + SqlConsts::OSM_TAG_FULLTEXT_TABLE + "(" + SqlConsts::OSM_TAG_FULLTEXT_TABLE + ") VALUES('optimize');";

const string SqlConsts::CREATE_OSM_RELATIONSHIPS = "CREATE TABLE " + SqlConsts::OSM_RELATIONSHIP_TABLE + "(groupid INTEGER NOT NULL, objectid INTEGER NOT NULL, objectorder INTEGER);";

// gtfs tags
const string SqlConsts::CREATE_GTFS_TAGS = "CREATE TABLE " + SqlConsts::GTFS_TAGS_TABLE + "(objectid INTEGER, tag NVARCHAR(30), value NVARCHAR(256));";
const string SqlConsts::CREATE_GTFS_GROUP_TAG = "CREATE TABLE " + SqlConsts::GTFS_GROUP_TAG_TABLE + "(groupid INTEGER, tag NVARCHAR(30), value NVARCHAR(256));";
const string SqlConsts::CREATE_GTFS_GROUP_RELATIONSHIPS = "CREATE TABLE " + SqlConsts::GTFS_GROUPS_RELATIONSHIP_TABLE + "(groupid INTEGER, objectid INTEGER, objectorder INTEGER);";

// normalization
const string SqlConsts::UPDATE_TAGS_LOWER_CASE = "UPDATE " + SqlConsts::OSM_TAG_TABLE + " SET value=lower(value);";
const string SqlConsts::UPDATE_GTFS_LOWER_CASE = "UPDATE " + SqlConsts::GTFS_TAGS_TABLE + " SET value=lower(value);";
const string SqlConsts::UPDATE_GTFS_GROUP_LOWER_CASE = "UPDATE " + SqlConsts::GTFS_GROUP_TAG_TABLE + " SET value=lower(value);";
const string SqlConsts::UPDATE_ADDR_DECODE_AREAS_LOWER_CASE = "UPDATE " + SqlConsts::ADDRESS_DECODER_AREAS_TABLE + " SET name=lower(name);";
const string SqlConsts::UPDATE_ADDR_DECODE_AREAS_TYPES_LOWER_CASE = "UPDATE " + SqlConsts::ADDRESS_DECODER_AREAS_TABLE + " SET name=lower(name);";

// map info
const string SqlConsts::CREATE_MAP_INFO = "CREATE TABLE " + SqlConsts::MAP_INFO_TABLE + "(objectid INTEGER, tag NVARCHAR(256), value NVARCHAR(256));";

// transaction
const string SqlConsts::BEGIN = "BEGIN;";
const string SqlConsts::COMMIT = "COMMIT;";

const set<string> SqlConsts::RANKABLE = { SqlConsts::OSM_TAG_TABLE };

const vector<string> SqlConsts::PRAGMAS = { "PRAGMA synchronous = OFF;",
                                            "PRAGMA journal_mode = MEMORY;",
                                            "PRAGMA cache_size = 8000;",
                                            "PRAGMA temp_store = OFF;",
                                            "PRAGMA count_changes=OFF;",
                                            "PRAGMA foreign_keys = OFF;",
                                            "PRAGMA encoding = \"UTF-8\";" };
