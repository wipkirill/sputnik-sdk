#pragma once

#include <map>
#include <set>
#include <string>
#include <utility>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include "OsmTypes.h"

class OsmTagUtil {
public:
    template <typename T>
    static std::string getValueByKey(const std::string& key, T* osmObject) {
        for (unsigned int i = 0; i < osmObject->nTags; ++i) {
            if (osmObject->tags[i].key == key)
                return osmObject->tags[i].value;
        }
        return "";
    }
};
/**
 * the default edge filter accepting every edge
 * @brief The EdgeFilterDefault class
 */
class EdgeFilter {
protected:
    std::map<std::string, std::set<std::string> > access_;
    std::map<std::string, std::set<std::string> > accessPresent_;
    std::map<std::string, std::set<std::string> > accessRestriction_;
public:
    constexpr static double KMH_TO_MS = 0.277778;
public:
    EdgeFilter():access_(), accessPresent_(), accessRestriction_() {
        ;
    }
    virtual ~EdgeFilter() {
        ;
    }
    virtual int speedLimit(OSMWay* /*way*/) {
        return 0;
    }

    virtual bool reverseEdge(OSMWay* /*way*/) {
        return false;
    }

    virtual bool onewayEdge(OSMWay* /*way*/) {
        return false;
    }
    virtual bool isNoRestriction(const std::string &/*tag*/) {
        return false;
    }
    virtual bool isOnlyRestriction(const std::string &/*tag*/) {
        return false;
    }
    /**
     * @brief acceptEdge
     * @param way
     * @return
     */
    virtual bool acceptEdge(OSMWay* way) {
        // check that we have access
        for(const std::pair<std::string, std::set<std::string> > &allowedTag : access_) {
            std::string val = OsmTagUtil::getValueByKey<OSMWay>(allowedTag.first, way);
            if(val == "")
                return false;

            if(allowedTag.second.find(val) == allowedTag.second.end())
                return false;
        }

        // check if a key is present, then its value should be among the allowed values
        for(const std::pair<std::string, std::set<std::string> > &allowedTag : accessPresent_) {
            std::string val = OsmTagUtil::getValueByKey<OSMWay>(allowedTag.first,way);
            if(val == "")
                continue;

            if(allowedTag.second.find(val) == allowedTag.second.end())
                return false;
        }

        // check that we do not have access restriction
        for(const std::pair<std::string, std::set<std::string> > &notAllowedTag : accessRestriction_) {
            std::string val = OsmTagUtil::getValueByKey<OSMWay>(notAllowedTag.first, way);
            if(val == "")
                continue;

            if(notAllowedTag.second.find(val) != notAllowedTag.second.end())
                return false;
        }

        return true;
    }
};
/**
 * the vehicle edge filter
 * @brief The HighWayFilter class
 */
class HighWayFilter : public EdgeFilter {
private:
    std::set<std::string> oneWay_;
    std::set<std::string> oneWayDesignated_;
    std::set<std::string> notOneWay_;
    std::set<std::string> reverse_;
    std::map<std::string, int> speedLimits_;
    std::set<std::string> turnRestrictionType_;
    std::set<std::string> noTurnType_;
    std::set<std::string> onlyTurnType_;
public:
    HighWayFilter() : oneWay_(),oneWayDesignated_(), notOneWay_(),reverse_(),speedLimits_() {
        oneWay_ = {"true", "1", "yes", "-1", "reverse"};
        oneWayDesignated_ = {"motorway", "motorway_link", "trunk", "roundabout"};
        notOneWay_ = {"false", "0", "no"};
        reverse_ = {"-1", "reverse"};

        // should be present
        access_ = {{"highway", {"motorway",
                               "motorway_link",
                               "trunk",
                               "trunk_link",
                               "primary",
                               "primary_link",
                               "secondary",
                               "secondary_link",
                               "tertiary",
                               "tertiary_link",
                               "residential",
                               "unclassified",
                               "living_street",
                               "road"}}};

        // if present then should have one of the specified tags
        accessPresent_ = {{"access", {"yes", "designated", "permissive",
                                      "unknown", "destination", "customer",
                                      "official"}},
                          {"vehicle", {"yes", "designated", "permissive",
                                       "unknown", "destination", "customer",
                                       "official"}},
                          {"motor_vehicle", {"yes", "designated", "permissive",
                                             "unknown", "destination", "customer",
                                             "official"}},
                          {"motorcar", {"yes", "designated", "permissive",
                                        "unknown", "destination", "customer",
                                        "official"}}};

        // cannot be present with the specified value
        accessRestriction_ = {{"construction", {"yes"}},
                              {"access", {"no"}},
                              {"vehicle", {"no"}},
                              {"motorcar", {"no"}},
                              {"motorcycle", {"no"}},
                              {"highway", {"area", "construction"}}};

        // some implicit speed limits
        speedLimits_ = {{"motorway",      140}, // main intercity
                        {"motorway_link", 80},
                        {"trunk",         100}, // somewhat highspeed
                        {"trunk_link",    80},
                        {"primary",       120}, // primary intercity road
                        {"primary_link",  100},
                        {"secondary",     90},
                        {"secondary_link",60},
                        {"tertiary",      80},
                        {"tertiary_link", 60},
                        {"residential",   60},
                        {"unclassified",  20},
                        {"living_street", 20},
                        {"road",          80}}; // unknown road

        // turn restrictions types
        turnRestrictionType_ = {"only_right_turn", "only_left_turn", "only_straight_on",
                                "no_right_turn", "no_left_turn", "no_straight_on",
                                "no_entry", "no_exit", "no_u_turn"};

        // not allowed
        noTurnType_ = {"no_right_turn", "no_left_turn", "no_straight_on",
                       "no_entry", "no_exit", "no_u_turn"};

        // allowed only this
        onlyTurnType_ = {"only_right_turn", "only_left_turn", "only_straight_on"};
    }

    /**
     * @brief speedLimit
     * @param way
     * @return
     */
    virtual int speedLimit(OSMWay* way) {
        std::string classify = OsmTagUtil::getValueByKey<OSMWay>("highway", way);
        if(classify == "")
            assert(false);

        return (speedLimits_[classify]*0.277778); // convert to m/s
    }

    /**
     * @brief reverseEdge
     * @param way
     * @return
     */
    virtual bool reverseEdge(OSMWay* way) {
        std::string rever = OsmTagUtil::getValueByKey<OSMWay>("oneway", way);
        if(rever == "") {
            return false;
        } else {
            if(reverse_.find(rever) != reverse_.end()) {
                return true;
            } else {
                return false;
            }
        }
    }

    /**
     * @brief onewayEdge
     * @param way
     * @return
     */
    virtual bool onewayEdge(OSMWay* way) {
        std::string oneway = OsmTagUtil::getValueByKey<OSMWay>("oneway", way);
        if(oneway != "") {
            // standard check
            if(oneWay_.find(oneway) != oneWay_.end())
                return true;

            // false positive check
            if(notOneWay_.find(oneway) != notOneWay_.end())
                return false;

            // reverse check
            if(reverse_.find(oneway) != reverse_.end())
                return false;
        }

        // highway check
        std::string highway = OsmTagUtil::getValueByKey<OSMWay>("highway", way);
        if(highway != "") {
            if(oneWayDesignated_.find(highway) != oneWayDesignated_.end()) {
                return true;
            }
        }

        // roundabout check
        std::string junction = OsmTagUtil::getValueByKey<OSMWay>("junction", way);
        if(junction != "") {
            if(oneWayDesignated_.find(junction) != oneWayDesignated_.end()) {
                return true;
            }
        }

        return false;
    }

    /**
     * Restrictive turn type, not allowed to follow turn
     * @brief isNoRestriction
     * @return
     */
    virtual bool isNoRestriction(const std::string &tag) {
        return noTurnType_.count(tag);
    }

    /**
     * Permissive turn type, allowed to follow ONLY this turn
     * @brief isOnlyRestriction
     * @return
     */
    virtual bool isOnlyRestriction(const std::string &tag) {
         return onlyTurnType_.count(tag);
    }
};

/**
 * the pedestrian edge filter
 * @brief The HighWayFilter class
 */
class PedestrianFilter : public EdgeFilter {
public:
    PedestrianFilter() {
        access_ = {{"highway", {"trunk",
                                 "pedestrian",
                                 "living_street",
                                 "footway",
                                 "steps",
                                 "track",
                                 "path",
                                 "primary",
                                 "secondary",
                                 "tertiary",
                                 "unclassified",
                                 "residential",
                                 "living_street",
                                 "road",
                                 "service"}}};
    }

    /**
     * @brief speedLimit
     * @param way
     * @return
     */
    virtual int speedLimit(OSMWay* /*way*/) {
        return 10;
    }
};
