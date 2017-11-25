#pragma once 

#include <set>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include "ParserPlugin.h"
#include <UrbanLabs/Sdk/GTFS/GTFS.h>

using std::string;
using std::set;
using std::unordered_map;
using std::vector;

/**
 * @brief The PublicTransport class
 */
class PublicTransPlugin : public Plugin {
private:
    typedef unordered_map<string, string> StringMap;
    typedef unordered_map<string, set<string> > KVSet;
private:
    KVSet requiredKV_;
    std::string outputFileName_;
    // store gtfs objects
    std::string gtfsFileName_;
    FILE * gtfsFileDescriptor_;
    // store gtfs groups
    std::string gtfsGroupFileName_;
    FILE * gtfsGroupFileDescriptor_;
    // store gtfs relationship
    std::string gtfsRelFileName_;
    FILE * gtfsRelFileDescriptor_;
    // gtfs structures
    std::vector<Agency> agencies_;
    std::vector<Route> routes_;
    std::vector<Trip> trips_;
    std::vector<StopTime> stopTimes_;
    std::vector<Shape> shapes_;
    std::unordered_map<std::string, Route::RouteType> routeTypes_;
    // statistics
    size_t totalTagsWritten_;
    size_t totalGroupsWritten_;
private:
    /**
     *
     */
    template <typename T>
    StringMap getObjectTags(const T &obj) {
        StringMap objTags;
        for(unsigned int i=0; i < obj->nTags; ++i) {
            string key = obj->tags[i].key;
            string value = obj->tags[i].value;
            if (objTags.find(key) == objTags.end())
                objTags[key] = value;
        }
        return objTags;
    }
    /**
     * @brief findKey
     * @param tags
     * @param key
     * @return
     */
    bool findKey(const string &key, const StringMap &tags) const;
    /**
     * @brief findKeys
     * @param keys
     * @param tags
     * @return
     */
    bool findKeys(const vector<string> &keys, const StringMap &tags) const;
    /**
     * @brief findKeysValues
     * @param kvs
     * @param tags
     * @return
     */
    bool findKeysValues(const KVSet &kvs, StringMap &tags) const;
public:
    PublicTransPlugin(const std::string &outputFile);
public:
    virtual void init();
    virtual void notifyNode(OSMNode* /*node*/){}
    virtual void notifyWay(OSMWay* /*way*/){}
    virtual void notifyRelation(OSMRelation* rel);
    virtual void finalize();
    void writeData();
    virtual void validate();
    virtual void cleanUp();
    virtual std::vector<std::string> getTableNamesToImport() const;
    virtual std::vector<std::string> getSqlToCreateTables() const;
    virtual std::vector<std::string> getOtherSqlCommands() const;
    /**
     *
     */
    template <typename T>
    bool acceptObject(const T &rel) {
        StringMap objTags = getObjectTags(rel);
        return findKeysValues(requiredKV_, objTags);
    }
    /**
     * Extracts route/trip/agency/stop times/stops information from relation
     */
    template <typename T>
    void storeInfo(const T &rel) {
        // extract agency
        Agency agency;
        bool isNewAgency = false;
        if(!readAgency(rel, agency, isNewAgency))
            return;

        // extract route
        Route route;
        if(!readRoute(rel, agency, route))
            return;

        // extract shape
        Shape shape;
        vector<Shape> ways;
        readShapes(rel, shape, ways);

        // extract tips
        Trip trip;
        if(!readTrip(rel, route, shape, trip))
            return;

        // extract stop times
        vector<StopTime> tmpStopTimes;
        if(!storeStopTimes(rel, trip, tmpStopTimes))
            return;

        // seen a new agency
        if(isNewAgency)
            agencies_.push_back(agency);

        // always a new trip and a new route
        trips_.push_back(trip);
        routes_.push_back(route);
        stopTimes_.insert(stopTimes_.end(), tmpStopTimes.begin(), tmpStopTimes.end());
        shapes_.insert(shapes_.end(), ways.begin(), ways.end());
    }
    /**
     *
     */
    template <typename T>
    bool readAgency(const T &rel, Agency& agency, bool& isNew) {
        StringMap tagMap = getObjectTags(rel);
        string network;
        if(findKey("network", tagMap))
            network = StringUtils::escape(tagMap["network"]);
        string oper;
        if(findKey("operator", tagMap))
            oper = StringUtils::escape(tagMap["operator"]);

        if(oper.empty() && network.empty()) {
            agency.setId(GTFSObject::NullId);
            return false;
        }

        string agencyName;
        if(!oper.empty() && !network.empty())
            agencyName += oper + "/" +network;
        else if (!oper.empty())
            agencyName+=oper;
        else
            agencyName+=network;

        if(Agency::translator_.getId(agencyName) == GTFSObject::NullId) {
            agency.setField(Agency::AGENCY_NAME, agencyName);
            agency.setField(Agency::AGENCY_ID, agencyName);
            agency.setField(Agency::AGENCY_URL, "");
            agency.setField(Agency::AGENCY_TIMEZONE, "");
            isNew = true;
        } else {
            agency.setId(Agency::translator_.getId(agencyName));
        }
        return true;
    }
    /**
     *
     */
    template <typename T>
    bool readRoute(const T &rel, const Agency& agency, Route& route) {
        StringMap tagMap = getObjectTags(rel);
        if(!findKeys({"ref"}, tagMap))
            return false;

        string routeName;
        if(findKeys({"from","to"}, tagMap))
            routeName = StringUtils::escape(tagMap["from"]) +" / "+StringUtils::escape(tagMap["to"]);
        else
            routeName = StringUtils::escape(tagMap["ref"]);

        if(Route::translator_.getId(tagMap["ref"]) == GTFSObject::NullId) {
            if(routeTypes_.find(tagMap["route"]) != routeTypes_.end()) {
                string ref = StringUtils::escape(tagMap["ref"]);
                route.setField(Route::ROUTE_SHORT_NAME, ref);
                route.setField(Route::ROUTE_ID, ref);
                route.setField(Agency::AGENCY_ID, Agency::translator_.getStringId(agency.getId()));
                route.setField(Route::ROUTE_LONG_NAME, routeName);
                string routeType = lexical_cast(routeTypes_[tagMap["route"]]);
                route.setField(Route::ROUTE_TYPE, routeType);
                return true;
            }
        }
        return false;
    }
    /**
     *
     */
    template <typename T>
    bool readTrip(const T &rel, const Route& route, const Shape& shape, Trip& trip) {
        StringMap tagMap = getObjectTags(rel);
        if (!findKeys({"ref"}, tagMap))
            return false;
        if (Trip::translator_.getId(tagMap["ref"]) == GTFSObject::NullId) {
            string ref = StringUtils::escape(tagMap["ref"]);
            trip.setField("trip_short_name", ref);
            trip.setField("shape_id", Shape::translator_.getStringId(shape.getId()));
            trip.setField("trip_id", ref);
            trip.setField("route_id", Route::translator_.getStringId(route.getId()));
        } else
            return false;
        return true;
    }
    /**
     *
     */
    template <typename T>
    bool storeStopTimes(const T &rel, const Trip& trip, vector<StopTime> &stopTimes) {
        int sequence = 0;
        bool nodeFound = false;
        for (unsigned int i=0; i < rel->nMembers; ++i) {
            if (rel->members[i].eType == MEMBER_NODE) {
                Stop stop;
                stop.setField("stop_id", lexical_cast(rel->members[i].nID));
                StopTime stopTime;
                stopTime.setField("stop_id", Stop::translator_.getStringId(stop.getId()));
                stopTime.setField("stop_sequence", lexical_cast(sequence++));
                stopTime.setField("trip_id", Trip::translator_.getStringId(trip.getId()));
                stopTime.setField("departure_time","00:00:00");
                stopTime.setField("arrival_time","00:00:00");
                stopTimes.push_back(stopTime);
                nodeFound = true;
            }
        }
        return nodeFound;
    }
    /**
     *
     */
    template <typename T>
    bool readShapes(const T &rel, Shape& shape, vector<Shape>& ways) {
        int sequence = 0;
        bool nodeFound = false;
        shape.setField(Shape::SHAPE_ID, lexical_cast(rel->nID));
        for (unsigned int i=0; i < rel->nMembers; ++i) {
            if (rel->members[i].eType == MEMBER_WAY) {
                nodeFound = true;
                shape.setField(Shape::SHAPE_WAY_ID, lexical_cast(rel->members[i].nID));
                shape.setField(Shape::SHAPE_SEQ, lexical_cast(sequence++));
                ways.push_back(shape);
            }
        }
        return nodeFound;
    }
    /**
     * Serialize gtfs objects in gtfs file format
     */
    template<typename T>
    void prewriteData(const vector<T> &objects, string fileName) {
        FILE *file = fopen(fileName.c_str(), "w");
        T::serializeHeader(file);
        for(const T &obj : objects) {
            obj.serializeRow(file);
        }
        fclose(file);
    }
};
