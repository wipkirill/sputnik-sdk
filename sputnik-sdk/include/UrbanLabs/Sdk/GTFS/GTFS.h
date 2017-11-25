#pragma once

#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <cassert>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/GraphCore/Point.h>
#include <UrbanLabs/Sdk/Utils/StringUtils.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Utils/Timer.h>

/**
 * A generic way to read gtfs objects
 */
template<class T>
std::vector<T> readObjects(const std::string &fileName) {
    std::ifstream file(fileName, std::ios::in);

    if(file.is_open()) {
        std::string line;
        getline(file, line);
        // carefull, trims any whitespace
        line = StringUtils::trim(line);

        SimpleTokenator columnsTokenator(line, ',', '\"', false);
        std::vector<std::string> columns = columnsTokenator.getTokens();

        std::vector<T> objects;
        while(std::getline(file, line)) {
            SimpleTokenator st(line, ',', '\"', false);
            std::vector<std::string> tokens = st.getTokens();

            if(tokens.size() == columns.size()) {
                T object;
                for(int i = 0; i < columns.size(); i++) {
                    object.setField(columns[i], tokens[i]);
                }
                // TODO: check if condition is needed
                //if(object.isValid())
                objects.push_back(object);
            } else {
                LOGG(Logger::ERROR) << "Bad input line: " << line << " file: " << fileName << Logger::FLUSH;
            }
        }

        file.close();
        return objects;
    } else {
        LOGG(Logger::ERROR) << "Failed to load objects from: " << fileName << Logger::FLUSH;
        return {};
    }
}

/**
 * @brief IdTranslator
 */
template <typename T>
class IdTranslator {
private:
    bool off_;
    T globalId_;
    std::map <std::string, T> stringToId_;
    std::map <T, std::string> idToString_;
public:
    IdTranslator();
    T setId(const std::string  &value);
    T getId(const std::string &str);
    std::string getStringId(const T &t);
    T totalIds();
    void setOff(bool off);
    void addId(const T id, const std::string& val);
};
/**
 * @brief The GTFSObject class
 */
class GTFSObject {
public:
    typedef unsigned int ObjectId;
public:
    const static std::string TYPE;
private:
    bool valid_;
public:
    static const ObjectId NullId;
    GTFSObject();
    bool isValid() const;
    void setValidity(bool valid);
    static void serializeHeader(FILE *file);
    virtual ~GTFSObject() {;}

    // if an id is set to null id, it will be represented as
    // missing gtfs field
    template<typename T>
    std::string idToString(const T &id) const {
        if(id == GTFSObject::NullId)
            return "";
        else
            return lexical_cast(id);
    }
};
/**
 * @brief The Agency class
 */
class Agency : public GTFSObject {
public:
    typedef unsigned int AgencyId;
    typedef AgencyId Id;
public:
    const static std::string AGENCY_ID;
    const static std::string AGENCY_NAME;
    const static std::string AGENCY_URL;
    const static std::string AGENCY_TIMEZONE;
private:
    std::string name_;
    std::string url_;
    std::string timeZone_;
    AgencyId id_;
public:
    static IdTranslator <AgencyId> translator_;
    static std::vector<std::string> fields_;
public:
    Agency();
    Agency(AgencyId id, std::string name = "", std::string url = "", std::string timeZone = "");
    void setField(const std::string &fieldName, const std::string &value);
    size_t serialize(FILE * file) const;
    void serializeRow(FILE *file) const;
    static void serializeHeader(FILE *file);
    AgencyId getId() const;
    void setId(AgencyId id);
};
/**
 * @brief The Stop class
 */
class Stop : public GTFSObject {
public:
    typedef unsigned int StopId;
    typedef StopId Id;
private:
    StopId id_;
    std::string name_;
    Point pt_;
    std::string parentStopId_;
    StopId parentStop_;
public:
    const static std::string STOP_ID;
public:
    static const StopId NullStopId;
    static IdTranslator <StopId> translator_;
public:
    Stop();
    Stop(StopId id, std::string &name, Point pt);
    void setField(const std::string &fieldName, const std::string &value);
    Point getPoint() const;
    StopId getId() const;
};
/**
 * @brief The Route class
 */
class Route : public GTFSObject {
public:
    typedef unsigned int RouteId;
    typedef RouteId Id;
    typedef int32_t RouteType;
    static const RouteType TRAM;
    static const RouteType SUBWAY;
    static const RouteType RAIL;
    static const RouteType BUS;
    static const RouteType FERRY;
    static const RouteType CABLE_CAR;
    static const RouteType GONDOLA;
    static const RouteType FUNICULAR;
public:
    const static std::string ROUTE_ID;
    const static std::string ROUTE_TYPE;
    const static std::string ROUTE_SHORT_NAME;
    const static std::string ROUTE_LONG_NAME;
private:
    RouteId id_;
    RouteType type_;
    std::string shortName_;
    std::string longName_;
    Agency::AgencyId agencyId_;
public:
    static IdTranslator<RouteId> translator_;
    static std::vector<std::string> fields_;
public:
    Route();
    Route(RouteId id, const std::string &sName, const std::string &lName, RouteType type);
    void setField(const std::string &fieldName, const std::string &value);
    size_t serialize(FILE * file) const;
    void serializeRow(FILE *file) const;
    static void serializeHeader(FILE *file);
    RouteId getId() const;
    void setId(RouteId id);
};
/**
 * @brief The Calendar class
 */
class Calendar : public GTFSObject {
public:
    typedef unsigned int ServiceId;
    typedef ServiceId Id;
private:
    ServiceId id_;
    int days_[7];
    DateTime startDate_;
    DateTime endDate_;
public:
    const static std::string CALENDAR_SERVICE_ID;
public:
    static IdTranslator <ServiceId> translator_;
public:
    Calendar();
    Calendar(int (days) [7], std::string startDate, std::string endDate);
    bool contains(DateTime &date) const;
    DateTime nextDate(DateTime date) const;
    void setField(const std::string &fieldName, std::string &value);
};
/**
 * @brief The Shape class
 */
class Shape : public GTFSObject {
public:
    typedef int ShapeSequence;
    typedef unsigned int ShapeId;
    typedef ShapeId Id;
    typedef unsigned int WayId;
    typedef Point::CoordType CoordType;
    typedef Point::PointDistType DistType;
public:
    static IdTranslator <ShapeId> translator_;
    static std::vector<std::string> fields_;
public:
    const static std::string SHAPE_ID;
    const static std::string SHAPE_LAT;
    const static std::string SHAPE_LON;
    const static std::string SHAPE_SEQ;
    const static std::string SHAPE_WAY_ID;
    const static std::string SHAPE_DIST;
private:
    Shape::ShapeId id_;
    WayId wayId_;
    ShapeSequence shapeSeq_;
    CoordType lat_;
    CoordType lon_;
    DistType distanceTraveled_;
public:
    Shape();
    void setField(const std::string &fieldName, const std::string &value);
    size_t serialize(FILE * file) const;
    void serializeRow(FILE *file) const;
    static void serializeHeader(FILE *file);
    ShapeId getId() const;
};
/**
 * @brief The Trip class
 */
class Trip : public GTFSObject {
public:
    typedef unsigned int TripId;
    typedef TripId Id;
private:
    TripId id_;
    std::string shortName_;
    Route::RouteId routeId_;
    Shape::ShapeId shapeId_;
    Calendar::ServiceId serviceId_;
public:
    const static std::string TRIP_ID;
    const static std::string TRIP_SHORT_NAME;
public:
    static IdTranslator <TripId> translator_;
    static std::vector<std::string> fields_;
public:
    static TripId const NullTripId;
    Trip();
    Trip(Route::RouteId routeId, TripId id, Calendar::ServiceId serviceId);
    Calendar::ServiceId getServiceId();
    void setField(const std::string &fieldName, const std::string &value);
    TripId getId() const;
    size_t serialize(FILE * file) const;
    void serializeRow(FILE *file) const;
    static void serializeHeader(FILE *file);
};
/**
 * @brief The StopTime class
 */
class StopTime : public GTFSObject {
public:
    typedef int StopSequence;
    typedef Time::TimeDiff StopTimeDiff;
    typedef unsigned StopTimeId;
public:
    const static std::string TRIP_ID;
    const static std::string STOPTIME_ARRIVAL;
    const static std::string STOPTIME_DEPARTURE;
    const static std::string STOPTIME_ID;
    const static std::string STOPTIME_SEQ;
private:
    StopTimeId id_;
    Trip::TripId tripId_;
    Time arrivalTime_;
    Time departureTime_;
    Stop::StopId stopId_;
    StopSequence stopSeq_;
public:
    static std::vector<std::string> fields_;
public:
    StopTime();
    StopTime(Trip::TripId tripId, std::string arrival, std::string departure,
             Stop::StopId stopId, StopSequence stopSeq);
    Time getArrival() const;
    Time getDeparture() const;
    Trip::TripId getTripId() const;
    StopSequence getStopSeq() const;
    Stop::StopId getStopId() const;
    void setId(StopTimeId id);
    void setField(const std::string &fieldName, const std::string &value);
    bool isValid() const;
    bool operator < (const StopTime &stopTime) const;
    size_t serialize(FILE * file) const;
    void serializeRow(FILE *file) const;
    static void serializeHeader(FILE *file);
    void serializeGroupRelationship(FILE *file) const;
};

template <typename T>
IdTranslator <T>::IdTranslator()
    : off_(false), globalId_(0), stringToId_(), idToString_() {
    ;
}
/**
 * Adds a particular id->value mapping overwriting!! previous values
 */
template <typename T>
void IdTranslator<T>::addId(const T id, const std::string& val) {
    idToString_[id] = val;
    stringToId_[val] = id;
}

/**
 * Toggles the translator switch, when off ids are not translated
 * but have to be integral (which is checked).
 */
template<typename T>
void IdTranslator<T>::setOff(bool off) {
    off_ = off;
}

/**
 * Assumes that the id for a value was not previously set
 */
template <typename T>
T IdTranslator <T>::setId(const std::string &value) {
    if(!off_) {
        stringToId_[value] = globalId_;
        idToString_[globalId_] = value;
        globalId_++;
    } else {
        // in case translation is off, make sure that the value is an integral type
        GTFSObject::ObjectId id = lexical_cast<GTFSObject::ObjectId>(value);
        std::string strId = lexical_cast(id);
        assert(value == strId);

        stringToId_[value] = id;
        idToString_[id] = value;
    }

    return stringToId_[value];
}
/**
 *
 */
template <typename T>
T IdTranslator <T>::getId(const std::string &str) {
    if(stringToId_.find(str) == stringToId_.end()) {
        return GTFSObject::NullId;
    }
    return stringToId_[str];
}
/**
 * Assumes that the provided id is in the map
 */
template <typename T>
std::string IdTranslator <T>::getStringId(const T &t) {
    return idToString_[t];
}
/**
 *
 */
template <typename T>
T IdTranslator <T>::totalIds() {
    return globalId_;
}
