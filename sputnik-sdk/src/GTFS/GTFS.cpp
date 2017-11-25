
// GTFS.cpp
//
#include <cstring>
#include <UrbanLabs/Sdk/GTFS/GTFS.h>
#include <UrbanLabs/Sdk/GraphCore/VerticesGTFS.h>
#include <UrbanLabs/Sdk/GraphCore/EdgesGTFS.h>
#include <UrbanLabs/Sdk/Utils/StringUtils.h>
#include <UrbanLabs/Sdk/Storage/SqlConsts.h>

using namespace std;

const GTFSObject::ObjectId GTFSObject::NullId = -1;
const Stop::StopId Stop::NullStopId = GTFSObject::NullId;
const Trip::TripId Trip::NullTripId = GTFSObject::NullId;
const VertexGTFS::VertexId VertexGTFS::NullVertexId =  GTFSObject::NullId;

IdTranslator <Agency::AgencyId> Agency::translator_ = IdTranslator<Agency::AgencyId>();
IdTranslator <Stop::StopId> Stop::translator_ = IdTranslator<Stop::StopId>();
IdTranslator <Route::RouteId> Route::translator_ = IdTranslator<Route::RouteId>();
IdTranslator <Calendar::ServiceId> Calendar::translator_ = IdTranslator<Calendar::ServiceId>();
IdTranslator <Trip::TripId> Trip::translator_ = IdTranslator<Trip::TripId>();
IdTranslator <Shape::ShapeId> Shape::translator_ = IdTranslator<Shape::ShapeId>();

const string GTFSObject::TYPE        = "type";
const string Agency::AGENCY_ID       = "agency_id";
const string Agency::AGENCY_NAME     = "agency_name";
const string Agency::AGENCY_URL      = "agency_url";
const string Agency::AGENCY_TIMEZONE = "agency_timezone";

const Route::RouteType Route::TRAM = 0;
const Route::RouteType Route::SUBWAY = 1;
const Route::RouteType Route::RAIL = 2;
const Route::RouteType Route::BUS = 3;
const Route::RouteType Route::FERRY = 4;
const Route::RouteType Route::CABLE_CAR = 5;
const Route::RouteType Route::GONDOLA = 6;
const Route::RouteType Route::FUNICULAR = 7;

const string Route::ROUTE_ID         = "route_id";
const string Route::ROUTE_TYPE       = "route_type";
const string Route::ROUTE_SHORT_NAME = "route_short_name";
const string Route::ROUTE_LONG_NAME  = "route_long_name";

const string Shape::SHAPE_ID         = "shape_id";
const string Shape::SHAPE_LAT        = "shape_pt_lat";
const string Shape::SHAPE_LON        = "shape_pt_lon";
const string Shape::SHAPE_SEQ        = "shape_pt_sequence";
const string Shape::SHAPE_WAY_ID     = "shape_way_id";
const string Shape::SHAPE_DIST       = "shape_dist_traveled";

const string Trip::TRIP_ID           = "trip_id";
const string Trip::TRIP_SHORT_NAME   = "trip_short_name";

const string Calendar::CALENDAR_SERVICE_ID = "service_id";

const string StopTime::TRIP_ID            = "trip_id";
const string StopTime::STOPTIME_ARRIVAL   = "arrival_time";
const string StopTime::STOPTIME_DEPARTURE = "departure_time";
const string StopTime::STOPTIME_SEQ       = "stop_sequence";
const string StopTime::STOPTIME_ID        = "stop_time_id";

const string Stop::STOP_ID = "stop_id";

// fields used when serializing objects in gtfs rowwise format
// these headers are used to serialize only gtfs parsed from osm
vector<string> Agency::fields_ = {"agency_id", "agency_name", "agency_url", "agency_timezone"};
vector<string> Route::fields_ = {"route_id", "route_short_name", "route_long_name", "agency_id", "route_type"};
vector<string> Trip::fields_ = {"trip_id", "route_id", "service_id", "trip_short_name", "shape_id"};
vector<string> StopTime::fields_ = {"stop_time_id", "trip_id", "arrival_time",
                                    "departure_time", "stop_id", "stop_sequence"};
vector<string> Shape::fields_ = {"shape_id", "way_id", "shape_pt_sequence",
                                 "shape_pt_lat", "shape_pt_lon", "shape_dist_traveled"};

/**
 * @brief writeGtfsHeaderFields
 * @param file
 * @param fields
 */
void writeGtfsHeaderFields(FILE *file, const vector<string> &fields) {
    string sep = ",";
    string header = "";
    for(int i = 0; i < fields.size(); i++) {
        header.append(fields[i]);
        if(i+1 < fields.size()) {
            header.append(sep);
        }
    }
    fprintf(file, "%s\n", header.c_str());
}

/**
 * @brief GTFSObject::GTFSObject
 */
GTFSObject::GTFSObject()
    : valid_(true) {
    ;
}
/**
 * @brief GTFSObject::isValid
 * @return
 */
bool GTFSObject::isValid() const {
    return valid_;
}
/**
 * @brief GTFSObject::setValidity
 * @param valid
 */
void GTFSObject::setValidity(bool valid) {
    valid_ = valid;
}
/**
 * @brief Agency::Agency
 */
Agency::Agency() : name_(), url_(), timeZone_(), id_(GTFSObject::NullId) {
    ;
}
/**
 * @brief Agency::Agency
 * @param id
 * @param name
 * @param url
 * @param timeZone
 */
Agency::Agency(AgencyId id, string name, string url, string timeZone)
    :  name_(name), url_(url), timeZone_(timeZone), id_(id) {
    ;
}
/**
 * @brief Agency::setField
 * @param fieldName
 * @param value
 */
void Agency::setField(const string &fieldName, const string &value) {
    if(fieldName == AGENCY_NAME) {
        name_ = value;
    } else if(fieldName == AGENCY_URL) {
        url_ = value;
    } else if(fieldName == AGENCY_TIMEZONE) {
        timeZone_ = value;
    } else if(fieldName == AGENCY_ID) {
        id_ = translator_.setId(value);
    }
}
/**
 * @brief Agency::getId
 * @return
 */
Agency::AgencyId Agency::getId() const {
    return id_;
}
/**
 * @brief Agency::serialize
 * @param file
 */
size_t Agency::serialize(FILE * file) const {
    size_t recAdded = 2;
    string sep = StringConsts::SEPARATOR;
    string printSpec = type_spec<Agency::AgencyId>()+sep+type_spec<string>()+sep+type_spec<string>()+"\n";
    fprintf(file, printSpec.c_str(), id_, AGENCY_ID.c_str(), lexical_cast(id_).c_str());
    fprintf(file, printSpec.c_str(), id_, GTFSObject::TYPE.c_str(), SqlConsts::GTFS_AGENCY.c_str());
    if (!name_.empty()) {
        recAdded++;
        fprintf(file, printSpec.c_str(), id_, AGENCY_NAME.c_str(), name_.c_str());
    }
    if (!url_.empty()) {
        recAdded++;
        fprintf(file, printSpec.c_str(), id_, AGENCY_URL.c_str(), url_.c_str());
    }
    if (!timeZone_.empty()) {
        recAdded++;
        fprintf(file, printSpec.c_str(), id_, AGENCY_TIMEZONE.c_str(), timeZone_.c_str());
    }
    return recAdded;
}
/**
 * @brief Agency::serializeHeader
 * @param file
 */
void Agency::serializeHeader(FILE *file) {
    writeGtfsHeaderFields(file, Agency::fields_);
}

/**
 * @brief serializeRow
 * @param file
 */
void Agency::serializeRow(FILE *file) const {
    string sep = ",";
    string printSpec = type_spec<string>()+sep+type_spec<string>()+
            sep+type_spec<string>()+sep+type_spec<string>()+"\n";
    fprintf(file, printSpec.c_str(), GTFSObject::idToString(id_).c_str(), name_.c_str(), url_.c_str(), timeZone_.c_str());
}
/**
 * @brief Agency::setId
 * @param id
 */
void Agency::setId(AgencyId id) {
    id_ = id;
}
/**
 * @brief Stop::Stop
 */
Stop::Stop() : id_(GTFSObject::NullId), name_(), pt_(), parentStopId_(), parentStop_(GTFSObject::NullId) {
    ;
}
/**
 * @brief Stop::Stop
 * @param id
 * @param name
 * @param pt
 */
Stop::Stop(StopId id, string &name, Point pt)
    : id_(id), name_(name), pt_(pt), parentStopId_(), parentStop_(GTFSObject::NullId) {
    ;
}
/**
 * @brief Stop::setField
 * @param fieldName
 * @param value
 */
void Stop::setField(const string &fieldName, const string &value) {
    if(fieldName == "stop_id") {
        id_ = translator_.setId(value);
    } else if(fieldName == "stop_name") {
        name_ = value;
    } else if(fieldName == "stop_lat") {
        pt_.setLat(lexical_cast<Point::CoordType>(value));
    } else if(fieldName == "stop_lon") {
        pt_.setLon(lexical_cast<Point::CoordType>(value));
    } else if(fieldName == "parent_station") {
        parentStopId_ = value;
    }
}
/**
 * @brief Stop::getPoint
 * @return
 */
Point Stop::getPoint() const {
    return pt_;
}
/**
 * @brief Stop::getId
 * @return
 */
Stop::StopId Stop::getId() const {
    return id_;
}
/**
 * @brief Route::Route
 */
Route::Route() : id_(GTFSObject::NullId), shortName_(), longName_(), agencyId_(GTFSObject::NullId) {
    ;
}
/**
 * @brief Route::Route
 * @param id
 * @param sName
 * @param lName
 * @param type
 */
Route::Route(RouteId id, const string &sName, const string &lName, RouteType type)
    : id_(id), type_(type), shortName_(sName), longName_(lName), agencyId_(GTFSObject::NullId) {
    ;
}
/**
 * @brief Route::setField
 * @param fieldName
 * @param value
 */
void Route::setField(const string &fieldName, const string &value) {
    if(fieldName == ROUTE_ID) {
        id_ = translator_.setId(value);
    } else if(fieldName == ROUTE_SHORT_NAME) {
        shortName_ = value;
    } else if(fieldName == ROUTE_LONG_NAME) {
        longName_ = value;
    } else if(fieldName == Agency::AGENCY_ID) {
        agencyId_ = Agency::translator_.getId(value);
    } else if(fieldName == ROUTE_TYPE) {
        // hack, because there is no lexical cast translation from string to RouteType
        type_ = lexical_cast<uint32_t>(value);
    }
}
/**
 * @brief Route::serialize
 * @param file
 */
size_t Route::serialize(FILE * file) const {
    size_t recAdded = 2;
    string sep = StringConsts::SEPARATOR;
    string printSpec = type_spec<Route::RouteId>()+sep+type_spec<string>()+sep+type_spec<string>()+"\n";
    fprintf(file, printSpec.c_str(), id_, ROUTE_ID.c_str(), lexical_cast(id_).c_str());
    fprintf(file, printSpec.c_str(), id_, GTFSObject::TYPE.c_str(), SqlConsts::GTFS_ROUTE.c_str());
    if (!shortName_.empty()) {
        fprintf(file, printSpec.c_str(), id_, ROUTE_SHORT_NAME.c_str(), shortName_.c_str());
        vector<string> tokens = StringUtils::filterWords(StringUtils::split(shortName_));
        if (tokens.size() > 1) {
            int i = 0;
            for (string & token : tokens) {
                string newKey = string(ROUTE_SHORT_NAME+lexical_cast(i));
                fprintf(file, printSpec.c_str(), id_, newKey.c_str(), token.c_str());
                recAdded++;
                i++;
            }
        }
        recAdded++;
    }
    if (!longName_.empty()) {
        fprintf(file, printSpec.c_str(), id_, ROUTE_LONG_NAME.c_str(), longName_.c_str());
        vector<string> tokens = StringUtils::filterWords(StringUtils::split(longName_));
        if (tokens.size() > 1) {
            int i = 0;
            for (string & token : tokens) {
                string newKey = string(ROUTE_LONG_NAME+lexical_cast(i));
                fprintf(file, printSpec.c_str(), id_, newKey.c_str(), token.c_str());
                recAdded++;
                i++;
            }
        }
        recAdded++;
    }
    fprintf(file, printSpec.c_str(), id_, Agency::AGENCY_ID.c_str(), lexical_cast(agencyId_).c_str());
    string rType = lexical_cast(static_cast<int>(type_));
    fprintf(file, printSpec.c_str(), id_, ROUTE_TYPE.c_str(), lexical_cast(static_cast<int>(type_)).c_str());
    recAdded += 2;
    return recAdded;
}
/**
 * @brief Route::serializeRow
 * @param file
 */
void Route::serializeHeader(FILE *file) {
    writeGtfsHeaderFields(file, Route::fields_);
}
/**
 * @brief Route::serializeRow
 * @param file
 */
void Route::serializeRow(FILE *file) const {
    string sep = ",";
    string printSpec = type_spec<string>()+sep+type_spec<string>()+sep+type_spec<string>()+
            sep+type_spec<string>()+sep+type_spec<RouteType>()+"\n";

    fprintf(file, printSpec.c_str(), GTFSObject::idToString(id_).c_str(), shortName_.c_str(), longName_.c_str(),
            GTFSObject::idToString(agencyId_).c_str(), type_);
}
/**
 * @brief Route::setId
 * @param id
 */
void Route::setId(RouteId id) {
    id_ = id;
}

/**
 * @brief getId
 * @return
 */
Route::RouteId Route::getId() const {
    return id_;
}

/**
 * @brief Calendar::Calendar
 */
Calendar::Calendar() : id_(GTFSObject::NullId), startDate_(), endDate_() {
    ;
}
/**
 * @brief Calendar::Calendar
 * @param startDate
 * @param endDate
 */
Calendar::Calendar(int days[7], string startDate, string endDate)
    : startDate_(startDate), endDate_(endDate) {
    memcpy(days_, days, sizeof(int)*7);
}
/**
 * @brief Calendar::contains
 * @param date
 * @return
 */
bool Calendar::contains(DateTime &date) const {
    int day = date.getWeekDay();
    if(days_[day] && date <= endDate_ && startDate_ <= date) {
        return true;
    }
    return false;
}
/**
 * @brief Calendar::nextDate
 * @param date
 * @return
 */
DateTime Calendar::nextDate(DateTime date) const {
    if(startDate_ <= date) {
        int day = date.getWeekDay();
        for(int i = 0; i < 7; i++, day++, day %= 7) {
            if(days_[day]) {
                if(i == 0) {
                    return date;
                } else {
                    date.incrementBy(DateTime::SECONDS_IN_DAY*i);
                    date.setTime(Time(0, 0, 0));
                    if(date <= endDate_)
                        return date;
                    else
                        return DateTime::invalid_;
                }
            }
        }
    }
    return startDate_;
}
/**
 * @brief Calendar::setField
 * @param fieldName
 * @param value
 */
void Calendar::setField(const string &fieldName, string &value) {
    map<string, int> dayMap = {{"monday", 0}, {"tuesday", 1}, {"wednesday", 2}, {"thursday", 3},
                               {"friday", 4}, {"saturday", 5}, {"sunday", 6}
    };
    if(fieldName == "service_id") {
        id_ = translator_.setId(value);
    } else if(fieldName == "route_short_name") {
        endDate_ = DateTime(value);
    } else if(fieldName == "start_date") {
        startDate_ = DateTime(value);
    } else if(fieldName == "end_date") {
        endDate_ = DateTime(value, false);
    } else if(dayMap.find(fieldName) != dayMap.end()) {
        days_[dayMap[fieldName]] = lexical_cast<int>(value);
    }
}
/**
 * @brief Trip::Trip
 */
Trip::Trip() : id_(GTFSObject::NullId), shortName_(), shapeId_(GTFSObject::NullId), serviceId_(GTFSObject::NullId) {
    serviceId_ = GTFSObject::NullId;
    shapeId_ = GTFSObject::NullId;
}
/**
 * @brief Trip::Trip
 * @param routeId
 * @param id
 * @param serviceId
 */
Trip::Trip(Route::RouteId routeId, TripId id, Calendar::ServiceId serviceId)
    : id_(id), shortName_(), routeId_(routeId), shapeId_(GTFSObject::NullId), serviceId_(serviceId) {
    ;
}
/**
 * @brief Trip::serialize
 * @param file
 */
size_t Trip::serialize(FILE * file) const {
    size_t recAdded = 3;
    string sep = StringConsts::SEPARATOR;
    string printSpec = type_spec<ObjectId>()+sep+type_spec<string>()+sep+type_spec<string>()+"\n";
    fprintf(file, printSpec.c_str(), id_, TRIP_ID.c_str(), lexical_cast(id_).c_str());
    fprintf(file, printSpec.c_str(), id_, Route::ROUTE_ID.c_str(), lexical_cast(routeId_).c_str());
    fprintf(file, printSpec.c_str(), id_, GTFSObject::TYPE.c_str(), SqlConsts::GTFS_TRIP.c_str());
    if (serviceId_ != GTFSObject::NullId) {
        fprintf(file, printSpec.c_str(), id_, Calendar::CALENDAR_SERVICE_ID.c_str(), lexical_cast(serviceId_).c_str());
        recAdded++;
    }
    if (!shortName_.empty()) {
        fprintf(file, printSpec.c_str(), id_, TRIP_SHORT_NAME.c_str(), shortName_.c_str());
        recAdded++;
    }
    if (shapeId_ != GTFSObject::NullId) {
        fprintf(file, printSpec.c_str(), id_, Shape::SHAPE_ID.c_str(), lexical_cast(shapeId_).c_str());
        recAdded++;
    }
    return recAdded;
}
/**
 * @brief Trip::serializeHeader
 * @param file
 */
void Trip::serializeHeader(FILE *file) {
    writeGtfsHeaderFields(file, Trip::fields_);
}
/**
 * @brief Route::serializeRow
 * @param file
 */
void Trip::serializeRow(FILE *file) const {
    string sep = ",";
    string printSpec = type_spec<string>()+sep+type_spec<string>()+
            sep+type_spec<string>()+sep+type_spec<string>()+
            sep+type_spec<string>()+"\n";

    fprintf(file, printSpec.c_str(), GTFSObject::idToString(id_).c_str(),
            GTFSObject::idToString(routeId_).c_str(), GTFSObject::idToString(serviceId_).c_str(),
            shortName_.c_str(), GTFSObject::idToString(shapeId_).c_str());
}
/**
 * @brief Trip::getServiceId
 * @return
 */
Calendar::ServiceId Trip::getServiceId() {
    return serviceId_;
}
/**
 * @brief getId
 * @return
 */
Trip::TripId Trip::getId() const {
    return id_;
}

/**
 * @brief Trip::setField
 * @param fieldName
 * @param value
 */
void Trip::setField(const string &fieldName, const string &value) {
    if(fieldName == TRIP_ID) {
        id_ = translator_.setId(value);
    } else if(fieldName == Route::ROUTE_ID) {
        routeId_ = Route::translator_.getId(value);
    } else if(fieldName == Shape::SHAPE_ID) {
        shapeId_ = Shape::translator_.getId(value);
    } else if(fieldName == TRIP_SHORT_NAME){
        shortName_ = value;
    } else if(fieldName == Calendar::CALENDAR_SERVICE_ID) {
        serviceId_ = Calendar::translator_.getId(value);
    }
}
/**
 * @brief StopTime::StopTime
 */
StopTime::StopTime() {
    ;
}
/**
 * @brief StopTime::StopTime
 * @param tripId
 * @param arrival
 * @param departure
 * @param stopId
 * @param stopSeq
 */
StopTime::StopTime(Trip::TripId tripId, string arrival, string departure, Stop::StopId stopId, StopSequence stopSeq)
    : tripId_(tripId), arrivalTime_(arrival), departureTime_(departure), stopId_(stopId), stopSeq_(stopSeq) {
    ;
}
/**
 * @brief StopTime::getArrival
 * @return
 */
Time StopTime::getArrival() const {
    return arrivalTime_;
}
/**
 * @brief StopTime::getDeparture
 * @return
 */
Time StopTime::getDeparture() const {
    return departureTime_;
}
/**
 * @brief StopTime::getTripId
 * @return
 */
Trip::TripId StopTime::getTripId() const {
    return tripId_;
}
/**
 * @brief StopTime::getStopSeq
 * @return
 */
StopTime::StopSequence StopTime::getStopSeq() const {
    return stopSeq_;
}
/**
 * @brief StopTime::serialize
 * @param file
 */
size_t StopTime::serialize(FILE * file) const {
    size_t recAdded = 7;
    string sep = StringConsts::SEPARATOR;
    string printSpec = type_spec<StopTime::StopTimeId>()+sep+type_spec<string>()+
                        sep+type_spec<string>()+"\n";

    fprintf(file, printSpec.c_str(), id_, GTFSObject::TYPE.c_str(), SqlConsts::GTFS_STOPTIME.c_str());
    fprintf(file, printSpec.c_str(), id_, TRIP_ID.c_str(), lexical_cast(tripId_).c_str());
    fprintf(file, printSpec.c_str(), id_, STOPTIME_ARRIVAL.c_str(),arrivalTime_.toString().c_str());
    fprintf(file, printSpec.c_str(), id_, STOPTIME_DEPARTURE.c_str(), departureTime_.toString().c_str());
    fprintf(file, printSpec.c_str(), id_, Stop::STOP_ID.c_str(), lexical_cast(stopId_).c_str());
    fprintf(file, printSpec.c_str(), id_, STOPTIME_SEQ.c_str(),lexical_cast(stopSeq_).c_str());
    fprintf(file, printSpec.c_str(), id_, STOPTIME_ID.c_str(),lexical_cast(id_).c_str());
    return recAdded;
}
/**
 * @brief StopTime::serializeHeader
 * @param file
 */
void StopTime::serializeHeader(FILE *file) {
    writeGtfsHeaderFields(file, StopTime::fields_);
}
/**
 * @brief StopTime::serializeRow
 * @param file
 */
void StopTime::serializeRow(FILE *file) const {
    string sep = ",";
    string printSpec = type_spec<string>()+sep+type_spec<string>()+
                        sep+type_spec<string>()+sep+type_spec<string>()+sep+type_spec<string>()+
                        sep+type_spec<StopTime::StopSequence>()+"\n";

    fprintf(file, printSpec.c_str(), GTFSObject::idToString(id_).c_str(), GTFSObject::idToString(tripId_).c_str(),
            arrivalTime_.toString().c_str(), departureTime_.toString().c_str(),
            GTFSObject::idToString(stopId_).c_str(), stopSeq_);
}

/**
 * @brief StopTime::serializeGroupRelationship
 * @param file
 */
void StopTime::serializeGroupRelationship(FILE *file) const {
    string sep = StringConsts::SEPARATOR;
    string printSpec = type_spec<Trip::TripId>()+sep+type_spec<StopTime::StopTimeId>()+
                        sep+type_spec<StopTime::StopSequence>()+"\n";

    fprintf(file, printSpec.c_str(), tripId_, id_, stopSeq_);
}
/**
 * @brief StopTime::getStopId
 * @return
 */
Stop::StopId StopTime::getStopId() const {
    return stopId_;
}
/**
 * @brief setId
 * @param id
 */
void StopTime::setId(StopTime::StopTimeId id) {
    id_ = id;
}
/**
 * @brief StopTime::setField
 * @param fieldName
 * @param value
 */
void StopTime::setField(const string &fieldName, const string &value) {
    if(fieldName == Trip::TRIP_ID) {
        tripId_ = Trip::translator_.getId(value);
    } else if(fieldName == Stop::STOP_ID) {
        // this is done so that translator doesn't destroy ids
        // of osm stops. checks if the value is an integral type
        // and in case it is, do not overwrtie
        Stop::StopId id = lexical_cast<Stop::StopId>(value);
        string strId = lexical_cast(id);
        if (strId == value) {
            stopId_ = id;
            Stop::translator_.addId(id, value);
        } else
            stopId_ = Stop::translator_.getId(value);
    } else if(fieldName == STOPTIME_SEQ) {
        stopSeq_ = lexical_cast<int>(value);
    } else if(fieldName == STOPTIME_ARRIVAL) {
        arrivalTime_ = Time(value);
    } else if(fieldName == STOPTIME_DEPARTURE) {
        departureTime_ = Time(value);
    } else if (fieldName == STOPTIME_ID) {
        id_ = lexical_cast<StopTimeId>(value);
    }
}
/**
 * @brief StopTime::isValid
 * @return
 */
bool StopTime::isValid() const {
    return arrivalTime_.isValid() && departureTime_.isValid();
}
/**
 * @brief StopTime::operator <
 * @param stopTime
 * @return
 */
bool StopTime::operator < (const StopTime &stopTime) const {
    if(tripId_ != stopTime.tripId_)
        return tripId_ < stopTime.getTripId();
    return stopSeq_ < stopTime.getStopSeq();
}
/**
 * @brief Shape::Shape
 */
Shape::Shape() {
    wayId_  = GTFSObject::NullId;
    lat_ = Point::NullCoord;
    lon_ = Point::NullCoord;
    distanceTraveled_ = Point::NullDist;
}
/**
 * @brief Shape::setField
 * @param fieldName
 * @param value
 */
void Shape::setField(const string &fieldName, const string &value) {
    if(fieldName == SHAPE_SEQ) {
        shapeSeq_ = lexical_cast<ShapeSequence>(value);
    } else if(fieldName == SHAPE_ID) {
        id_ = translator_.setId(value);
    } else if(fieldName == SHAPE_WAY_ID) {
        wayId_ = lexical_cast<WayId>(value);
    } else if(fieldName == SHAPE_LAT) {
        lat_ = lexical_cast<CoordType>(value);
    } else if(fieldName == SHAPE_LON) {
        lon_ = lexical_cast<CoordType>(value);
    } else if(fieldName == SHAPE_DIST) {
        distanceTraveled_ = lexical_cast<DistType>(value);
    }
}
/**
 * @brief Shape::getId
 * @return
 */
Shape::ShapeId Shape::getId() const {
    return id_;
}
/**
 * @brief Shape::serialize
 * @param file
 */
size_t Shape::serialize(FILE * file) const {
    size_t recAdded = 2;
    string sep = StringConsts::SEPARATOR;
    string printSpec = type_spec<ObjectId>()+sep+type_spec<string>()+sep+type_spec<string>()+"\n";
    fprintf(file, printSpec.c_str(), id_, SHAPE_ID.c_str(), lexical_cast(id_).c_str());
    fprintf(file, printSpec.c_str(), id_, GTFSObject::TYPE.c_str(), SqlConsts::GTFS_SHAPE.c_str());
    if (wayId_ != GTFSObject::NullId) {
        fprintf(file, printSpec.c_str(), id_, SHAPE_WAY_ID.c_str(), lexical_cast(wayId_).c_str());
        recAdded++;
    }
    recAdded++;
    fprintf(file, printSpec.c_str(), id_, SHAPE_SEQ.c_str(), lexical_cast(shapeSeq_).c_str());
    if (lat_ != Point::NullCoord) {
        recAdded++;
        fprintf(file, printSpec.c_str(), id_, SHAPE_LAT.c_str(), lexical_cast(lat_).c_str());
    }
    if (lon_ != Point::NullCoord) {
        recAdded++;
        fprintf(file, printSpec.c_str(), id_, SHAPE_LON.c_str(), lexical_cast(lon_).c_str());
    }
    if (distanceTraveled_ != Point::NullDist) {
        recAdded++;
        fprintf(file, printSpec.c_str(), id_, SHAPE_DIST.c_str(), lexical_cast(distanceTraveled_).c_str());
    }
    return recAdded;
}
/**
 * @brief Shape::serializeHeader
 * @param file
 */
void Shape::serializeHeader(FILE *file) {
    writeGtfsHeaderFields(file, Shape::fields_);
}
/**
 * @brief Shape::serializeRow
 * @param file
 */
void Shape::serializeRow(FILE * file) const {
    string sep = ",";
    string printSpec = type_spec<string>()+sep+type_spec<string>()+sep+
            type_spec<Shape::ShapeSequence>()+sep+type_spec<CoordType>()+sep+
            type_spec<CoordType>()+sep+type_spec<DistType>()+"\n";

    fprintf(file, printSpec.c_str(), GTFSObject::idToString(id_).c_str(),
            GTFSObject::idToString(wayId_).c_str(), shapeSeq_,
            lat_, lon_, distanceTraveled_);
}
