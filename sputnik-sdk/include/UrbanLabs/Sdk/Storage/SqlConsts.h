#pragma once

#include <set>
#include <map>
#include <vector>
#include <string>

class SqlConsts {
public:
    //---------------------------SQLITE TABLE NAMES-----------------------------
    const static std::string TAGS_TABLE;
    //here we store vertices
    const static std::string VERTICES_TABLE;
    //store edges
    const static std::string EDGES_TABLE;
    //temp table for Parser, fast vertex-point lookup
    const static std::string VP_TABLE;
    // store object tags (foreign key)
    // osm
    const static std::string OSMID_TO_ID_TABLE;
    const static std::string OSM_TAG_TABLE;
    const static std::string OSM_TAG_FULLTEXT_TABLE;
    const static std::string OSM_TAG_HASH_TABLE;
    const static std::string OSM_TAG_FIXED_TABLE;
    const static std::string OSM_TAG_FULLTEXT_TABLE_OPTIMIZE;
    // relationships
    const static std::string OSM_RELATIONSHIP_TABLE;
    // store object tags (foreign key)
    const static std::string GTFS_TAGS_TABLE;
    const static std::string GTFS_GROUP_TAG_TABLE;
    const static std::string GTFS_GROUPS_RELATIONSHIP_TABLE;
    //store endpoints in kdTree
    const static std::string KDTREE_ENDPT_TABLE;
    // kd tree to store non endpoints
    const static std::string KDTREE_NON_ENDPOINT_TABLE;
    //store data for kdTree
    const static std::string KDTREE_ADDRESS_DECODER_TABLE;
    //areas and bounding boxes
    const static std::string ADDRESS_DECODER_AREAS_TABLE;
    //store data for objects in kdTree
    const static std::string KDTREE_OBJECTS_INDEX_TABLE;
    //store geometry of edges
    const static std::string GEOMETRY_TABLE;
    //turnrestrictions table
    const static std::string TURN_RESTRICTIONS;
    // map info plugin
    const static std::string MAP_INFO_TABLE;
    // GTFS
    const static std::string GTFS_AGENCY;
    const static std::string GTFS_ROUTE;
    const static std::string GTFS_TRIP;
    const static std::string GTFS_STOPTIME;
    const static std::string GTFS_CALENDAR;
    const static std::string GTFS_SHAPE;
    // config
    const static std::string CONFIG_TABLE;
    //---------------------------SQL QUERIES---------------------------------
    // graph core
    const static std::string CREATE_KDTREE_ENDPT;
    const static std::string CREATE_KDTREE_NONENDPT;
    const static std::string CREATE_KDTREE_NONENDPT_DATA;
    const static std::string CREATE_VERTICES;
    const static std::string CREATE_EDGES;
    const static std::string CREATE_TURN_RESTRICTIONS;

    // object lookup
    const static std::string CREATE_OBJECT_KDTREE_INDEX;

    // vertex point index
    const static std::string INSERT_VP;
    const static std::string CREATE_VP_TABLE;

    // addres decoding
    const static std::string CREATE_ADDRESS_DECODER_KDTREE_INDEX;
    const static std::string CREATE_ADDRESS_DECODER_AREA_INDEX;

    // geometry
    const static std::string INSERT_GEOMETRY;
    const static std::string CREATE_GEOMETRY;

    // tags
    const static std::string CREATE_TAGS_TABLE;

    //osm ids
    const static std::string CREATE_OSMID_TO_ID_TABLE;
    // osm tags
    const static std::string CREATE_OSM_TAG;
    const static std::string CREATE_OSM_TAG_HASH;
    const static std::string CREATE_OSM_TAG_FULLTEXT;
    const static std::string CREATE_OSM_TAG_FIXED;

    const static std::string CREATE_OSM_RELATIONSHIPS;

    // tables that have curve and rank index
    const static std::set<std::string> RANKABLE;

    // osm tables used for ranking
    const static std::string CREATE_OSM_TAG_RANK;

    // gtfs tags
    const static std::string CREATE_GTFS_TAGS;
    const static std::string CREATE_GTFS_GROUP_TAG;
    const static std::string CREATE_GTFS_GROUP_RELATIONSHIPS;

    // mapinfo
    const static std::string CREATE_MAP_INFO;

    // normalization
    const static std::string UPDATE_TAGS_LOWER_CASE;
    const static std::string UPDATE_GTFS_LOWER_CASE;
    const static std::string UPDATE_GTFS_GROUP_LOWER_CASE;
    const static std::string UPDATE_ADDR_DECODE_AREAS_LOWER_CASE;
    const static std::string UPDATE_ADDR_DECODE_AREAS_TYPES_LOWER_CASE;

    // transaction
    const static std::string BEGIN;
    const static std::string COMMIT;

    // pragmas
    const static std::vector<std::string> PRAGMAS;
};
