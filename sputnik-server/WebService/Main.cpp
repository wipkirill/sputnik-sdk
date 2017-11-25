#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <WebService/Main.h>
#include <UrbanLabs/Sdk/SqlModels/Tag.h>
#include <UrbanLabs/Sdk/Output/JsonRouteFormatter.h>
#include <UrbanLabs/Sdk/Network/HttpClient.h>
#include <UrbanLabs/Sdk/OSM/TagFilter.h>
#include <UrbanLabs/Sdk/Utils/MathUtils.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Utils/Timer.h>
#include <UrbanLabs/Sdk/Utils/FileSystemUtil.h>
#include <UrbanLabs/Sdk/Utils/Compression.h>
#include <UrbanLabs/Sdk/Storage/TagConsts.h>

#include <boost/lexical_cast.hpp>

using namespace std;
using namespace CoreToolkit;
using namespace WebToolkit;
bool GeoRouting::serverStarted_ = false;

std::map<std::string, std::string> GeoRouting::ERRORS = {
    // invalid parameters
    {"NO_WAY_PTS","Parameter 'waypoints' was not specified"},
    {"NOT_ENOUGH_WPTS", "Not enough waypoints were specified (<2)"},
    {"MISS_LONLAT","Missing latitude/longitude"},
    {"TOO_MUCH_LONLAT", "Too many latitude/longitude parameters"},
    {"INVALID_COORD", "Invalid coordinate value: "},
    {"INVALID_LAT", "Invalid latitude value, possible range is [-90, 90]: "},
    {"INVALID_LON", "Invalid longitude value, possible range is [-180, 180]: "},
    {"TOO_MANY_PTS", "Too many points"},
    {"INVALID_TRAVMODE", "Invalid travel mode"},
    {"INVALID_METRIC", "Invalid metric"},
    {"PATH_NOT_FND", "Path not found "},
    {"SAME_WAY", "Points on same way"},
    {"NO_PROPERTY", "No property with this name"},
    {"GRAPH_MISSING", "No graph with this name found"},
    {"NO_MAP_TYPE", "No map type provided"},
    {"NO_MAP_NAME", "No map name provided"},
    {"INVALID_BBOX", "Invalid bounding box"},
    {"UNKNOWN_GRAPH_TYPE","Unknown graph type"},
    {"NOT_ENOUGH_ARGS","Not enough arguments provided"},
    {"INVALID_OBJTYPE", "Invalid object type"},
    // failed services
    {"KDTREE_POINT_MISSING", "Cannot find specified point"},
    {"OBJECT_POINT_MISSING", "Cannot find specified object"},
    {"FAILED_LOAD_GRAPH", "Cannot load graph"},
    {"FAILED_UNLOAD_GRAPH", "Cannot unload graph"},
    {"FAILED_NEAREST_NEIGHBOR", "Cannot find nearest neighbor for all points"},
    {"FAILED_GET_TAGS", "Cannot get tags for objects"},
    {"FAILED_MATCH_TAG", "Cannot match a tag"},
    {"FAILED_FIND_MAP", "Cannot find map file"},
    {"FAILED_SEARCH", "Cannot match conditions in search"},
    {"FAILED_RESOLVE_NODEIDS", "Cannot resolve provided ids"},
    {"FAILED_ASSIGN_PTS", "Cannot find coordinates for search result."},
    {"FAILED_DECODE_ADDR", "Cannot decode address."}};

std::set<std::string> GeoRouting::travelModes_ = {"driving", "walking", "bicycling", "publictransport", ""};

std::map<std::string, GeoRouting::TravelMode> GeoRouting::modeMap_ =
                                         {{"driving", GeoRouting::TravelMode::CAR},
                                         {"walking", GeoRouting::TravelMode::PEDESTRIAN},
                                         {"bicycling", GeoRouting::TravelMode::PEDESTRIAN},
                                         {"publictransport", GeoRouting::TravelMode::PUBLICTRANSPORT},
                                         {"", GeoRouting::TravelMode::CAR}};

std::set<std::string> GeoRouting::metricModes_ = {"time", "distance", ""};

std::map<std::string, GeoRouting::Metric> GeoRouting::metricMap_ =
                                          {{"distance", GeoRouting::Metric::DISTANCE},
                                           {"time", GeoRouting::Metric::TIME},
                                           {"", GeoRouting::Metric::DISTANCE}};

//#define GEO_DEBUG
#define HTTP_HANDLER(obj, url) new HttpHandlerConnector<GeoRouting>(obj, url)

GeoRouting* GeoRouting::instance_ = nullptr;
/**
 * @brief GeoRouting::GeoRouting
 * @param port
 */
GeoRouting::GeoRouting(int port, const string &workDir) : server_(port,"0.0.0.0", 10) {
    workDir_ = workDir;
    service_.setWorkDir(workDir);
    WAYPOINTS = "waypoints";
    TRAVEL_MODE = "travelmode";
    METRIC = "metric";
    ALTERNATIVE = "altern";

    CTYPE_JSON = "application/json; charset=UTF-8";

    Init();
    LOGG(Logger::DEBUG) << "[SERVER] Started on "<< port <<", workdir "<< workDir_<< Logger::FLUSH;
#ifndef ANDROID
    string welcome = "Point your web browser to http://localhost:"+lexical_cast(port)+"/menu.html";
    LOGG(Logger::PROGRESS) << welcome << Logger::FLUSH;
#endif //ANDROID
}
/**
 * @brief GeoRouting::Init
 * @param mapName
 */
void GeoRouting::Init() {
    // initialize callbacks
    dispatcher_.AddMapping("/heartbeat", HttpGet, HTTP_HANDLER(this,&GeoRouting::heartBeat),true);
    // HTML pages and resources
    dispatcher_.AddMapping("/index.html", HttpGet,HTTP_HANDLER(this,&GeoRouting::index),true);
    dispatcher_.AddMapping("/menu.html", HttpGet,HTTP_HANDLER(this,&GeoRouting::menu),true);
    dispatcher_.AddMapping("/error.html", HttpGet,HTTP_HANDLER(this,&GeoRouting::errorPage),true);
    dispatcher_.AddMapping("/res", HttpGet,HTTP_HANDLER(this,&GeoRouting::serveFile),true);
    // Routing and Graphs
    dispatcher_.AddMapping("/graph/load", HttpGet,HTTP_HANDLER(this,&GeoRouting::loadGraph),true);
    dispatcher_.AddMapping("/graph/unload", HttpGet,HTTP_HANDLER(this,&GeoRouting::unloadGraph),true);
    dispatcher_.AddMapping("/graph/list", HttpGet,HTTP_HANDLER(this,&GeoRouting::getLoadedGraphs),true);
    dispatcher_.AddMapping("/graph/route", HttpGet, HTTP_HANDLER(this,&GeoRouting::route),true);
    dispatcher_.AddMapping("/graph/nearest", HttpGet, HTTP_HANDLER(this,&GeoRouting::nearestNeighbor),true);
    // Search
    dispatcher_.AddMapping("/search/query", HttpGet,HTTP_HANDLER(this,&GeoRouting::search),true);
    dispatcher_.AddMapping("/search/nearest", HttpGet, HTTP_HANDLER(this,&GeoRouting::nearestObject),true);
    dispatcher_.AddMapping("/matchtags", HttpGet,HTTP_HANDLER(this,&GeoRouting::matchTags),true);
    // Tiles
    dispatcher_.AddMapping("/loadtiles", HttpGet,HTTP_HANDLER(this,&GeoRouting::loadTiles),true);
    dispatcher_.AddMapping("/setzoom", HttpGet,HTTP_HANDLER(this,&GeoRouting::setZoom),true);
    dispatcher_.AddMapping("/gettile", HttpGet,HTTP_HANDLER(this,&GeoRouting::getTile),true);
    // Misc
    dispatcher_.AddMapping("/listmaps", HttpGet,HTTP_HANDLER(this,&GeoRouting::listMaps),true);
    dispatcher_.AddMapping("/mapicon", HttpGet,HTTP_HANDLER(this,&GeoRouting::mapicon),true);
    dispatcher_.AddMapping("/mapinfo", HttpGet,HTTP_HANDLER(this,&GeoRouting::getMapInfo),true);
    dispatcher_.AddMapping("/download", HttpGet,HTTP_HANDLER(this,&GeoRouting::download),true);
    dispatcher_.AddMapping("/decompress", HttpGet,HTTP_HANDLER(this,&GeoRouting::decompress),true);
    dispatcher_.AddMapping("/taskstatus", HttpGet,HTTP_HANDLER(this,&GeoRouting::taskStatus),true);
    dispatcher_.AddMapping("/writedata", HttpGet,HTTP_HANDLER(this,&GeoRouting::writeData),true);
    dispatcher_.AddMapping("/reload", HttpGet,HTTP_HANDLER(this,&GeoRouting::resetServices),true);
    dispatcher_.AddMapping("/getconfig", HttpGet,HTTP_HANDLER(this,&GeoRouting::getConfig),true);
    dispatcher_.AddMapping("/terminate", HttpGet,HTTP_HANDLER(this,&GeoRouting::terminate),true);
    server_.RegisterHandler(&dispatcher_);
}

/**
 * @brief GeoRouting::resetServices
 * @param context
 */
void GeoRouting::resetServices(HttpServerContext *context) {
    string workdir;
    if(!FsUtil::currentWorkDir(workdir)) {
        LOGG(Logger::ERROR)<<"Cannot determine current workdir"<< Logger::FLUSH;
        throw std::exception();
    }
    service_.setWorkDir(workdir);
    LOGG(Logger::INFO) << "[SERVER RESET]"<< Logger::FLUSH;
    respondSuccess(context);
}
/**
 * @brief GeoRouting::getConfig
 * @param context
 */
void GeoRouting::getConfig(HttpServerContext *context) {
    vector<TagList> props;
    service_.getConfig(props);
    JSONFormatterNode root("");

    JSONFormatterNode::Nodes nodes;
    for(int i=0; i < (int)props.size();++i)
        nodes.push_back(props[i].toJSONFormatterNode());
    JSONFormatterNode node("response",nodes);
    root.add(JSONFormatterNode::Nodes({successAttr(), ver(), node}));

    respondContent(context, {}, CTYPE_JSON, root);
}
/**
 * @brief getTravelMode
 * @param context
 * @return
 */
GeoRouting::TravelMode GeoRouting::getTravelMode(const map<string,string> &request) const {
    if (request.find("travelmode") != request.end()) {
        string travelMode = request.find("travelmode")->second;
        if(travelModes_.find(travelMode) != travelModes_.end()) {
            return modeMap_.find(travelMode)->second;
        } else {
            LOGG(Logger::ERROR) << ERRORS["INVALID_TRAVMODE"] << Logger::FLUSH;
        }
    }

    return TravelMode::DEFAULT;
}
/**
 * @brief getMetric
 * @param context
 * @return
 */
GeoRouting::Metric GeoRouting::getMetric(const map<string,string> &request) const {
    if (request.find("metric") != request.end()) {
        string metric = request.find("metric")->second;
        if(metricModes_.find(metric) != metricModes_.end()) {
            return metricMap_.find(metric)->second;
        } else {
            LOGG(Logger::ERROR) << ERRORS["INVALID_METRIC"] << Logger::FLUSH;
        }
    }
    return Metric::DEFAULT;
}
/**
 * @brief GeoRouting::validate
 * @param context
 * @param wayPoints
 */
bool GeoRouting::validate(const HttpServerContext *context, vector<Point> &wayPoints,
                          TravelMode &mode, Metric &metric, string &error) const {
    // check waypoint argument in HTTP request
    if(!findKey(context, WAYPOINTS)) {
        error = "NO_WAY_PTS";
        return false;
    }

    // parse waypoints
    map<string,string> request = getAllAttributes(context);
    SimpleTokenator tokenator(request[WAYPOINTS], '|', '\"', true);
    if(tokenator.countTokens() < 2) {
        error = "NOT_ENOUGH_WPTS";
        return false;
    }

    vector<string> tokens = tokenator.getTokens();
    for(int i = 0; i < tokens.size(); i++) {
        string lonLatString = tokens[i];
        SimpleTokenator lonLatTokenizer(lonLatString, ',', '\"', true);
        if(lonLatTokenizer.countTokens() < 2) {
            error = "MISS_LONLAT";
            return false;
        }
        if(lonLatTokenizer.countTokens() > 2) {
            error = "TOO_MUCH_LONLAT";
            return false;
        }

        VertexPoint::CoordType lat = lexical_cast<VertexPoint::CoordType>(lonLatTokenizer.nextToken()),
                               lon = lexical_cast<VertexPoint::CoordType>(lonLatTokenizer.nextToken());
        wayPoints.push_back(Point(lat, lon));
    }

    if(wayPoints.size() > 150) {
        error = "TOO_MANY_PTS";
        return false;
    }

    metric = getMetric(request);
    mode = getTravelMode(request);
    return true;
}
/**
 * @brief GeoRouting::runShortestPath
 * @param modeIndex
 * @param wayPoints
 * @param searchResults
 */
template<typename G>
void GeoRouting::runShortestPath(G &g, const GeoRouting::Metric &metric, vector<Point> &wayPoints,
                                 vector<typename G::SearchResult> &searchResults) {

    // start the timer
    Timer timer;
    timer.start();

    size_t wayPointsSize = wayPoints.size();
    for(size_t i = 0, j = 1; j < wayPointsSize; i++, j++) {
        if (metric == Metric::DISTANCE) {
            LOGG(Logger::INFO) << "[DISTANCE METRIC]" << Logger::FLUSH;
            g.shortestPathBidirectionalDijkstra(wayPoints[i], wayPoints[j], searchResults[i]);
        } else {
            LOGG(Logger::INFO) << "[TIME METRIC]" << Logger::FLUSH;
            g.template shortestPathBidirectionalDijkstra<typename Edge::TimeMetric>(wayPoints[i], wayPoints[j], searchResults[i]);
        }
    }

    // get elapsed time
    timer.stop();
    LOGG(Logger::INFO) << "[SPT TIME]: " << timer.getElapsedTimeSec() << " sec" << Logger::FLUSH;
}

/**
 * @brief GeoRouting::runShortestPathPublic
 * @param modeIndex
 * @param wayPoints
 * @param searchResults
 */
template<typename G>
void GeoRouting::runShortestPathPublic(G &g, const GeoRouting::Metric &metric, vector<Point> &wayPoints,
                                       vector<typename G::SearchResult> &searchResults) {
    // start the timer
    Timer timer;
    timer.start();

    size_t wayPointsSize = wayPoints.size();
    for(size_t i = 0, j = 1; j < wayPointsSize; i++, j++) {
        if (metric == Metric::DISTANCE) {
            LOGG(Logger::INFO) << "[DISTANCE METRIC]" << Logger::FLUSH;
            g.shortestPathDijkstra(wayPoints[i], wayPoints[j], searchResults[i]);
        }
    }

    // get elapsed time
    timer.stop();
    LOGG(Logger::INFO) << "[SPT TIME]: " << timer.getElapsedTimeSec() << " sec" << Logger::FLUSH;
}
/**
 * @brief GeoRouting::outputResults
 * @param wayPoints
 * @param modeIndex
 * @param searchResults
 * @param context
 */
template<typename G>
void GeoRouting::outputResults(G &graph, const vector<Point> &wayPoints,
                               const vector<typename G::SearchResult> &searchResults,
                               HttpServerContext *context) {

    // start the timer
    Timer timer;

    // intialize json parser
    JsonRouteFormatter fmt;
    if(wayPoints.size() > 0) {
        fmt.addRoot(wayPoints[0], wayPoints[wayPoints.size()-1]);

        // accumulate legs for a route
        JsonRouteFormatter::Nodes legs;

        // compute path geometries
        for(const typename G::SearchResult &currPath : searchResults) {
            VertexPoint src = currPath.getSrc().getTarget(), dst = currPath.getDst().getTarget();
            if (!currPath.isValid()) {
                stringstream msg;
                msg << src.getPoint() << "|" << dst.getPoint() << endl;
                respondError(context, ERRORS["PATH_NOT_FND"]+msg.str());
                return;
            }
            vector<vector<Point> > multiLines;
            graph.findMultiLinesFromPath(currPath.getSrc(), currPath.getDst(),
                                         currPath.getPath(), multiLines);

            legs.push_back(fmt.getLeg(multiLines, currPath.getOrigWayIds(), src.getPoint(), dst.getPoint(),
                                      currPath.getLength(), currPath.getLength()));
        }

        fmt.addRoute(wayPoints[0], wayPoints[wayPoints.size()-1], legs, 0);

        // time elapsed
        timer.stop();
        LOGG(Logger::INFO) << "[FIND MULTILINE TIME]: " << timer.getElapsedTimeSec() << " sec" << Logger::FLUSH;
    }

    // output results
    JSONFormatterNode root("");
    JSONFormatterNode node("response", fmt.root_);
    root.add(JSONFormatterNode::Nodes({successAttr(), ver(), node}));

    respondContent(context, {}, CTYPE_JSON, root);
}
/**
 * @brief GeoRouting::Run
 */
void GeoRouting::Run() {
    server_.Run();
    GeoRouting::serverStarted_ = true;
}
/**
 * @brief GeoRouting::findKey
 * @param context
 * @param key
 * @return
 */
bool GeoRouting::findKey(const HttpServerContext *context, const string &key) const {
    if(context->parameters.find(key) != context->parameters.end())
        return true;
    return false;
}
/**
 * @brief getMetric
 * @param context
 * @return
 */
string GeoRouting::getProperty(const string &prop, const map<string,string> &request) const {
    auto it = request.find(prop);
    if (it != request.end()) {
        return it->second;
    }
    return "";
}
/**
 * @brief GeoRouting::findKey
 * @param context
 * @param key
 * @return
 */
bool GeoRouting::findKeys(const HttpServerContext *context, const vector<string> &keys) const {
    for(const string &key : keys)
        if(!findKey(context, key))
            return false;
    return true;
}
/**
 * @brief escape
 * @param token
 * @return
 */
string unEscape(const string &token) {
    return StringUtils::replaceAll(token, "+", " ");
}
/**
 * @brief GeoRouting::getArgument
 * @param context
 * @param name
 * @return
 */
template<typename T>
T GeoRouting::getAttribute(const HttpServerContext *context, const string &name) const {
    if(findKeys(context, {name})) {
        return lexical_cast<T>(getProperty(name, context->parameters));
    }
    return T();
}
/**
 *
 */
template<>
string GeoRouting::getAttribute<string>(const HttpServerContext *context, const string &name) const {
    if(findKeys(context, {name})) {
        return unEscape(getProperty(name, context->parameters));
    }
    return "";
}
/**
 * @brief removeBrackets
 * @param s
 * @return
 */
string removeBrackets(string s) {
    s.erase(remove(s.begin(), s.end(), '['), s.end());
    s.erase(remove(s.begin(), s.end(), ']'), s.end());
    s.erase(remove(s.begin(), s.end(), '{'), s.end());
    s.erase(remove(s.begin(), s.end(), '}'), s.end());
    return s;
}
/**
 *
 */
template<typename T>
vector<T> GeoRouting::getAttributes(const HttpServerContext *context, const string &name) const {
    vector<T> args;
    if(findKeys(context, {name})) {
        string str = removeBrackets(getProperty(name, context->parameters));
        SimpleTokenator tokenator(str, ',', '\"', true);
        for(int i = 0; i < tokenator.countTokens(); i++)
            args.push_back(lexical_cast<T>(tokenator.nextToken()));
    }
    return args;
}
/**
 *
 */
template<>
vector<string> GeoRouting::getAttributes(const HttpServerContext *context, const string &name) const {
    vector<string> args;
    if(findKeys(context, {name})) {
        string str = removeBrackets(getProperty(name, context->parameters));
        SimpleTokenator tokenator(str, ',', '\"', true);
        if(str.size() > 0) {
            for(int i = 0; i < tokenator.countTokens(); i++) {
                string token = tokenator.nextToken();
                args.push_back(unEscape(token));
            }
        }
    }
    return args;
}
template<> 
vector<pair<string, string> > GeoRouting::getAttributes(const HttpServerContext *context, const string &name) const {
    vector<pair<string, string> > args;
    if(findKeys(context, {name})) {
        vector<string> tokens = getAttributes<string>(context, name);
        if(tokens.size() > 1 && tokens.size() % 2 == 0) {
            for(int i = 0; i+1 < (int)tokens.size(); i += 2) {
                if(tokens[i] != "" && tokens[i+1] != "")
                    args.push_back({tokens[i],tokens[i+1]});
            }
        } else {
            LOGG(Logger::INFO) << "Attribute has wrong format" << Logger::FLUSH;
        }
    }
    return args;
}
template<> 
vector<Point> GeoRouting::getAttributes(const HttpServerContext *context, const string &name) const {
    vector<Point> args;
    if(findKeys(context, {name})) {
        vector<string> tokens = getAttributes<string>(context, name);
        if(tokens.size() % 2 == 0) {
            for(int i = 0; i+1 < (int)tokens.size(); i += 2) {
                Point::CoordType lat = lexical_cast<Point::CoordType>(tokens[i]);
                Point::CoordType lon = lexical_cast<Point::CoordType>(tokens[i+1]);
                args.push_back({lat, lon});
            }
        } else {
            LOGG(Logger::INFO) << "Attribute has wrong format" << Logger::FLUSH;
        }
    }
    return args;
}
/**
 * @brief GeoRouting::getAllAttributes
 * @param context
 * @return
 */
map<string, string> GeoRouting::getAllAttributes(const WebToolkit::HttpServerContext *context) const {
    return context->parameters;
}
/**
 * @brief GeoRouting::getRequestResource
 * @param context
 * @return
 */
string GeoRouting::getRequestResource(const HttpServerContext *context) const {
    return context->requestHeader.resource;
}
/**
 * @brief GeoRouting::handleError
 * @param context
 * @param errMsg
 */
void GeoRouting::respondError(WebToolkit::HttpServerContext *context, const string& errMsg) const {
    context->responseHeader.contentType = CTYPE_JSON;
    JSONFormatterNode root("");
    JSONFormatterNode ok("ok", "false");
    JSONFormatterNode err("error", errMsg);
    root.add(JSONFormatterNode::Nodes({ok, err, ver()}));
    context->responseBody << root;
    LOGG(Logger::ERROR) << errMsg << Logger::FLUSH;
}
/**
 * @brief GeoRouting::respondSuccess
 * @param context
 */
void GeoRouting::respondSuccess(WebToolkit::HttpServerContext *context) const {
    context->responseHeader.contentType = "application/json; charset=UTF-8";
    JSONFormatterNode root("");
    JSONFormatterNode ok("ok", "true");
    root.add(JSONFormatterNode::Nodes({ok, ver()}));
    context->responseBody << root;
}
/**
 * @brief GeoRouting::successAttr
 * @param context
 */
JSONFormatterNode::Node GeoRouting::successAttr() const {
    return JSONFormatterNode::Node("ok", "true");
}
/**
 * @brief GeoRouting::ver
 * @param context
 */
JSONFormatterNode::Node GeoRouting::ver() const {
    return JSONFormatterNode::Node("version", ServerOption::version());
}
/**
 * @brief GeoRouting::respondFile
 * @param context
 * @param fileName
 */
void GeoRouting::respondFile(HttpServerContext *context, const string &fileName) const {
    LOGG(Logger::INFO) << "Reading " << fileName <<" contents" << Logger::FLUSH;
    if(FsUtil::fileExists(fileName)) {
        context->ServeFile(fileName);
    } else {
        LOGG(Logger::ERROR) << "Couldn't read " << fileName << Logger::FLUSH;
    }
}
/**
 * @brief GeoRouting::respondContent
 */
template<typename T>
void GeoRouting::respondContent(HttpServerContext *context,
                                const std::map<string, string> &headers,
                                const std::string &contentType,
                                const T &content) const {
    if(!contentType.empty())
        context->responseHeader.contentType = contentType;
    map<string, string>::const_iterator iter;
    for(iter = headers.begin();iter!=headers.end();++iter)
        context->responseHeader.customHeaders.insert((*iter));
    context->responseBody << content;
}
/**
 * @brief GeoRouting::filterTagList
 */
TagList GeoRouting::filterTagList(TagList & tagList, bool (TagFilter::* func)(const string &)) {
    TagList newTags;
    newTags.setId(tagList.getId());
    newTags.setPoint(tagList.getPoint());
    vector<KeyValuePair> tags = tagList.getTags();
    for(KeyValuePair &kv : tags) {
        if((tagFilter_.*func)(kv.getKey()))
            newTags.add(kv);
    }
    return newTags;
}
/**
 * @brief GeoRouting::sortAccording
 * @param ids
 * @param tagList
 */
bool GeoRouting::sortAccording(const vector<Vertex::VertexId> &ids, vector<TagList> &tagList) const {
    if(ids.size() == 0)
        return true;

    // sort tags by the first occurence of the id
    map<Vertex::VertexId, int> order;
    for(int i = (int)ids.size()-1; i >= 0; i--) {
        order[ids[i]] = i;
    }

    TagListComparator<TagList> comp(order);
    stable_sort(tagList.begin(), tagList.end(), comp);

    map<Vertex::VertexId, TagList> prev;
    for(int i = 0; i < tagList.size(); i++) {
        prev[tagList[i].getId()] = tagList[i];
    }

    vector<TagList> duplicated(ids.size());
    for(int i = 0; i < ids.size(); i++) {
        if(prev.count(ids[i]) == 0)
            return false;
        duplicated[i] = prev[ids[i]];
    }
    tagList = duplicated;
    return true;
}
/**
 * @brief sortAccording
 * @param ids
 * @param tagList
 */
void GeoRouting::sortAccording(const Point &pt, std::vector<TagList> &tagList) const {
    vector<pair<Point::CoordType, Vertex::VertexId> > pts;
    for(int i = 0; i < tagList.size(); i++) {
        pts.push_back({pointDistance(pt, tagList[i].getPoint()), tagList[i].getId()});
    }

    sort(pts.begin(), pts.end());

    // sort tags by the first occurence of the id
    map<Vertex::VertexId, int> order;
    for(int i = (int)pts.size()-1; i >= 0; i--) {
        order[pts[i].second] = i;
    }

    // the sorting should be stable here as tags are sorted according to the
    // tag name
    TagListComparator<TagList> comp(order);
    stable_sort(tagList.begin(), tagList.end(), comp);
}
/**
 * @brief GeoRouting::index
 * @param context
 */
void GeoRouting::index(HttpServerContext *context) {
    vector<string> indexPath = {workDir_, Folder::COMMON, FileName::INDEX_HTML};
    string indexFilePath = FsUtil::makePath(indexPath);
    respondFile(context, indexFilePath);
}
/**
 * @brief GeoRouting::menu
 * @param context
 */
void GeoRouting::menu(HttpServerContext *context) {
    vector<string> menuPath = {workDir_, Folder::COMMON, FileName::MENU_HTML};
    string menuFilePath = FsUtil::makePath(menuPath);
    respondFile(context, menuFilePath);
}
/**
 * @brief GeoRouting::error
 * @param context
 */
void GeoRouting::errorPage(HttpServerContext *context) {
    vector<string> errorPath = {workDir_, Folder::COMMON, Folder::RESOURCE, FileName::ERROR_HTML};
    string errorFilePath = FsUtil::makePath(errorPath);
    respondFile(context, errorFilePath);
}
/**
 * @brief removeEmptyTags
 * @param tags
 */
void removeEmptyTags(vector<string> &tags) {
    sort(tags.begin(), tags.end());
    reverse(tags.begin(), tags.end());
    while(tags.size() > 0 && tags[tags.size()-1] == "")
        tags.pop_back();
}
/**
 * @brief GeoROuting::getMapInfo
 * @param context
 */
void GeoRouting::getMapInfo(HttpServerContext *context) {
    if(findKeys(context, {"mapname"})) {
        string mapName = getAttribute<string>(context, "mapname");

        // gather the conditions
        ConditionContainer conds;
        conds.addIdsIn({0});

        // output tags are guaranteed to be sorted by tag name and id
        vector<TagList> tags;
        int limit = std::numeric_limits<int>::max();
        if(!service_.simpleSearch(mapName, SqlConsts::MAP_INFO_TABLE, conds, 0, limit, tags)) {
            respondError(context, ERRORS["FAILED_GET_TAGS"]);
            return;
        }
        JSONFormatterNode::Nodes nodes;
        std::vector <JSONFormatterNode> nodes_ = tags[0].toJSONFormatterNode().nodes();
        JSONFormatterNode::Node response("response");
        for(int i = 0; i < nodes_.size();++i) {
            nodes.push_back(nodes_[i]);
        }

        response.add(nodes);

        JSONFormatterNode::Node root;
        root.add(JSONFormatterNode::Nodes({successAttr(), ver(), response}));

        respondContent(context, {}, CTYPE_JSON, root);
    } else {
        respondError(context, "Not enough arguments, provide mapname");
    }
}
/**
 * @brief GeoRouting::heartBeat
 * @param context
 */
void GeoRouting::heartBeat(HttpServerContext *context) {
    respondSuccess(context);
}
/**
 * @brief GeoRouting::search
 * @param context
 */
void GeoRouting::search(HttpServerContext *context) {
    // parse input arguments
    if(findKeys(context, {"mapname", "source"})) {
        // in case there are node ids search among them only
        if(findKeys(context, {"ids"}) ) {
            return getTags(context);
        }

        string mapName = getAttribute<string>(context, "mapname");
        // get the reference location
        vector<Point::CoordType> near = getAttributes<Point::CoordType>(context,"near");
        // get required tags
        vector<string> tags = getAttributes<string>(context, "tags");
        // Delete this
        //removeEmptyTags(tags);
        // q
        string q = StringUtils::escape(getAttribute<string>(context, "q"));
        q = StringUtils::replaceAll(q, "\"", " ");
        // q
        string source = getAttribute<string>(context, "source");
        // tags that have specific value
        vector<pair<string,string> > tagVals = getAttributes<pair<string, string> >(context, "tagsvals");
        // offset and limit of the result set
        int offset = Service::DEFAULT_SEARCH_RESULT_OFFSET;
        string offStr = getAttribute<string>(context, "offset");
        if(offStr != "") {
            offset = boost::lexical_cast<int>(offStr);
        }

        int limit = Service::DEFAULT_SEARCH_RESULT_LIMIT;
        string limStr = getAttribute<string>(context, "limit");
        if(limStr != "")
            limit = boost::lexical_cast<int>(limStr);

        if(tags.size() == 0 && q == "" && tagVals.size() == 0) {
            respondError(context, ERRORS["NOT_ENOUGH_ARGS"]);
            return;
        }

        // full text search on location, required tokens and free tokens
        ConditionContainerFullText conds;
        for(int i = 0; i < tagVals.size(); i++) {
            if(tagVals[i].first != "" && tagVals[i].second != "")
                conds.addTokensExist({tagVals[i].first+"="+tagVals[i].second});
        }
        if(tags.size() > 0) {
            conds.addTokensExist(tags);
        }
        if(near.size() == 2) {
            conds.addLocation({near[0], near[1]});
        }
        if(q != "") {
            SimpleTokenator tokenator(q, ' ', '\"', false);
            vector<string> tokens = tokenator.getTokens();
            conds.addTokensApproxExist(tokens);
        }

        // output tags are guaranteed to be sorted by tag name and id
        vector<TagList> foundTags;
        if(!service_.fulltextSearch(mapName, source, conds, offset, limit, foundTags)) {
            respondError(context, ERRORS["FAILED_SEARCH"]);
            return;
        }

        // resolve to osm ids
        if(source == SqlConsts::OSM_TAG_TABLE && !resolveToOsmIds(context, mapName, {}, foundTags)) {
            respondError(context, ERRORS["FAILED_RESOLVE_NODEIDS"]);
            return;  
        }

        JSONFormatterNode root("");
        JSONFormatterNode::Nodes nodes = tagsToJSON(foundTags);       
        JSONFormatterNode node("response", nodes);
        root.add(JSONFormatterNode::Nodes({successAttr(), ver(), node}));
        respondContent(context, {}, CTYPE_JSON, root);
    } else {
        respondError(context, ERRORS["NOT_ENOUGH_ARGS"]);
        return;
    }
}
/**
 * @brief GeoRouting::tagsToJSON
 * @param ids
 * @param tags
 * @return
 */
template<typename T>
JSONFormatterNode::Nodes GeoRouting::tagsToJSON(vector<T> &tags) {
    JSONFormatterNode::Nodes nodes;
    for(int i = 0; i < tags.size(); i++) {
        nodes.push_back(tags[i].toJSONFormatterNode());
    }
    return nodes;
}
/**
 * @brief GeoRouting::getPoints
 */
bool GeoRouting::getPoints(HttpServerContext *context, const string &mapName, 
                           const vector<Vertex::VertexId> &ids, vector<Point> &points) {
    vector<int8_t> found;
    if(!service_.findCoordinates(mapName, ids, points, found)) {
        respondError(context, ERRORS["NOT_ENOUGH_ARGS"]);
        return false;
    }
    for(int i = 0; i < points.size(); i++) {
        if(!found[i]) {
            respondError(context, ERRORS["NOT_ENOUGH_ARGS"]);
            return false;
        }
    }

    return true;
}
/**
 * @brief GeoRouting::assignPoints
 */
bool GeoRouting::assignPoints(HttpServerContext *context, const string &mapName, vector<TagList> &tags) {
    vector<Point> points;
    vector<int8_t> found;
    vector<Vertex::VertexId> needed;
    for(int i = 0; i < tags.size(); i++) {
        needed.push_back(tags[i].getId());
    }
    if(!service_.findCoordinates(mapName, needed, points, found)) {
        respondError(context, ERRORS["OBJECT_POINT_MISSING"]);
        return false;
    }
    for(int i = 0; i < needed.size(); i++) {
        if(!found[i]) {
            respondError(context, ERRORS["OBJECT_POINT_MISSING"]);
            return false;
        }
    }
    for(int i = 0; i < tags.size(); i++) {
        tags[i].setPoint(points[i]);
    }
    return true;
}
/**
 * @brief GeoRouting::resolveToOsmIds
 */
bool GeoRouting::resolveToOsmIds(HttpServerContext *context, const string &mapName, 
                                 const vector<Vertex::VertexId> &ids, vector<TagList> &tags) {
    string path;
    if(!service_.findFile(mapName, path)) {
        respondError(context, ERRORS["FAILED_FIND_MAP"]);
        return false;
    }

    if(!sortAccording(ids, tags)) {
        respondError(context, ERRORS["FAILED_GET_TAGS"]);
        return false;
    }

    URL url(path);
    if(!service_.resolveToOsmIds(url, tags)) {
        respondError(context, ERRORS["FAILED_RESOLVE_NODEIDS"]);
        return false;
    }   

    return true;
}
/**
 * @brief GeoRouting::getTags
 * @param context
 */
void GeoRouting::getTags(HttpServerContext *context) {
    // parse input arguments
    if(findKeys(context, {"mapname", "source"})) {       
        string mapName = getAttribute<string>(context, "mapname");
        // get ids
        vector<Vertex::VertexId> ids;
        vector<string> resolve = getAttributes<string>(context, "ids");
        if(!service_.resolveFromOsmIds(mapName, resolve, ids)) {
            respondError(context, ERRORS["FAILED_RESOLVE_NODEIDS"]);
            return;
        }
        string source = getAttribute<string>(context, "source");
        // tags that have specific value
        vector<pair<string,string> > tagVals = getAttributes<pair<string,string> >(context, "tagsvals");
        // offset and limit of the result set
        // TODO Why there are no hastagsvals?
        int offset = Service::DEFAULT_SEARCH_RESULT_OFFSET;
        string offStr = getAttribute<string>(context, "offset");
        if(offStr != "") {
            offset = boost::lexical_cast<int>(offStr);
        }

        int limit = Service::DEFAULT_SEARCH_RESULT_LIMIT;
        string limStr = getAttribute<string>(context, "limit");
        if(limStr != "")
            limit = boost::lexical_cast<int>(limStr);

        // if we have definite ids, we will extract all of them
        if(ids.size() != 0) {
            limit = ids.size();
            offset = 0;
        }

        if(ids.size() == 0 && tagVals.size() == 0) {
            respondError(context, ERRORS["NOT_ENOUGH_ARGS"]);
            return;
        }

        // validate tables
        set<string> tables = {SqlConsts::OSM_TAG_TABLE, SqlConsts::GTFS_TAGS_TABLE, SqlConsts::GTFS_GROUP_TAG_TABLE};
        if(tables.count(source) == 0) {
            respondError(context, ERRORS["INVALID_OBJTYPE"]);
            return;
        }

        ConditionContainer cont;
        if(ids.size() > 0) cont.addIdsIn(ids);

        vector<TagList> tags;
        if(!service_.simpleSearch(mapName, source, cont, offset, limit, tags)) {
            respondError(context, ERRORS["FAILED_GET_TAGS"]);
            return;
        }

        // resolve coordinates
        set<string> withPoints = {SqlConsts::OSM_TAG_TABLE};
        if(withPoints.count(source) && !assignPoints(context, mapName, tags)) {
            respondError(context, ERRORS["FAILED_ASSIGN_PTS"]);
            return;
        }

        // resolve to osm ids
        if(source == SqlConsts::OSM_TAG_TABLE && !resolveToOsmIds(context, mapName, ids, tags)) {
            respondError(context, ERRORS["FAILED_RESOLVE_NODEIDS"]);
            return;  
        }

        JSONFormatterNode root("");
        JSONFormatterNode::Nodes nodes = tagsToJSON(tags);
        JSONFormatterNode node("response", nodes);
        root.add(JSONFormatterNode::Nodes({successAttr(), ver(), node}));

        respondContent(context, {}, CTYPE_JSON, root);
    } else {
        respondError(context, ERRORS["NOT_ENOUGH_ARGS"]);
        return;
    }
}
/**
 * @brief GeoRouting::index
 * @param context
 */
void GeoRouting::loadGraph(HttpServerContext *context) {
    LOGG(Logger::INFO) << "Loading graph..." << Logger::FLUSH;
    if(findKeys(context, {"mapname", "maptype"})) {
        string mapType = getAttribute<string>(context, "maptype");
        string mapName = getAttribute<string>(context, "mapname");

        if(service_.addGraph(mapName, mapType)) {
            respondSuccess(context);
        } else {
            respondError(context, ERRORS["FAILED_LOAD_GRAPH"]);
        }
    } else {
        respondError(context, ERRORS["NOT_ENOUGH_ARGS"]);
    }
}
/**
 * @brief GeoRouting::index
 * @param context
 */
void GeoRouting::unloadGraph(HttpServerContext *context) {
    LOGG(Logger::INFO) << "Unloading graph..." << Logger::FLUSH;
    if(findKeys(context, {"mapname", "maptype"})) {
        string mapType = getAttribute<string>(context, "maptype");
        string mapName = getAttribute<string>(context, "mapname");

        if(service_.removeGraph(mapName, mapType)) {
            respondSuccess(context);
        } else {
            respondError(context, ERRORS["FAILED_UNLOAD_GRAPH"]);
        }
    } else {
        respondError(context, ERRORS["NOT_ENOUGH_ARGS"]);
    }
}
/**
 * @brief GeoRouting::getLoadedGraphs
 * @param context
 */
void GeoRouting::getLoadedGraphs(HttpServerContext *context) {
    LOGG(Logger::INFO) << "Getting loaded graphs..." << Logger::FLUSH;
    if(findKeys(context, {"maptype"})) {
        string mapType = getAttribute<string>(context, "maptype");
        set<string> tables = {SqlConsts::OSM_TAG_TABLE, SqlConsts::GTFS_TAGS_TABLE};
        if(tables.count(mapType)) {
            JSONFormatterNode response("");
            JSONFormatterNode graphs("response", service_.getLoadedGraphs(mapType));
            response.add(JSONFormatterNode::Nodes({successAttr(), ver(), graphs}));
            respondContent(context, {}, CTYPE_JSON, response);
        } else {
            respondError(context, ERRORS["UNKNOWN_GRAPH_TYPE"]);
        }
    } else {
        respondError(context, ERRORS["NO_MAP_TYPE"]);
    }
}
/**
 * @brief GeoRouting::nearestNeighbor
 * @param context
 */
void GeoRouting::nearestNeighbor(HttpServerContext* context) {
    if(findKeys(context, {"lat","lon","mapname","maptype"})) {
        VertexPoint::CoordType lon = getAttribute<VertexPoint::CoordType>(context, "lon");
        VertexPoint::CoordType lat = getAttribute<VertexPoint::CoordType>(context, "lat");

        // find closest point
        Point p(lat, lon);
        OsmGraphCore::NearestPointResult nearest;

        string mapType = getAttribute<string>(context, "maptype");
        string mapName = getAttribute<string>(context, "mapname");

        Timer timer;
        bool found = false;
        if(mapType == SqlConsts::OSM_TAG_TABLE) {
            if(!service_.existsGraph(mapName, mapType)) {
                if(!service_.addGraph(mapName, mapType)) {
                    respondError(context, ERRORS["FAILED_LOAD_GRAPH"]);
                    return;
                }
            }
            found = service_.findNearestPoint(mapName, mapType, p, nearest);
        }
        else if(mapType == SqlConsts::GTFS_TAGS_TABLE) {
            if(service_.existsGraph(mapName, mapType)) {
                found = service_.findNearestPoint(mapName, mapType, p, nearest);
            }
        }
        if(!found) {
            respondError(context, ERRORS["KDTREE_POINT_MISSING"]);
            return;
        }

        JSONFormatterNode node("response");
        node.add({JSONFormatterNode::Node("id", nearest.getTarget().getId()),
                  JSONFormatterNode::Node("lat", nearest.getTarget().getPoint().lat()),
                  JSONFormatterNode::Node("lon", nearest.getTarget().getPoint().lon())});
        JSONFormatterNode root("");
        root.add(JSONFormatterNode::Nodes({successAttr(), ver(), node}));

        timer.stop();

        LOGG(Logger::DEBUG) << "[NEAREST NEIGHBOR]: " << timer.getElapsedTimeSec() << Logger::FLUSH;
        respondContent(context, {}, CTYPE_JSON, root);
    } else {
        respondError(context, ERRORS["NOT_ENOUGH_ARGS"]);
    }
}
/**
 * @brief GeoRouting::nearestObject
 * @param context
 */
void GeoRouting::nearestObject(WebToolkit::HttpServerContext* context) {
    if(findKeys(context, {"mapname", "source"})) {
        string mapName = getAttribute<string>(context, "mapname");
        bool decode = getAttribute<string>(context, "decode") == "true";
        string source = getAttribute<string>(context, "source");
        // argument validation
        set<string> tables = {SqlConsts::OSM_TAG_TABLE, SqlConsts::GTFS_TAGS_TABLE};
        if(tables.count(source) == 0) {
            respondError(context, ERRORS["INVALID_OBJTYPE"]);
            return;
        }

        string points = getAttribute<string>(context, "points");
        string nodeids = getAttribute<string>(context, "ids");
        if(!((points == "") ^ (nodeids == ""))) {
            respondError(context, ERRORS["NOT_ENOUGH_ARGS"]);
            return;
        }      

        vector<Vertex::VertexId> ids;
        if(nodeids != "") {
            vector<string> resolve = getAttributes<string>(context, "ids");
            if(!service_.resolveFromOsmIds(mapName, resolve, ids)) {
                respondError(context, ERRORS["FAILED_RESOLVE_NODEIDS"]);
                return;
            }
        }

        // if we are given points, find the nearest objects to those points
        vector<Point> pts = getAttributes<Point>(context, "points");
        for(int i = 0; i < (int)pts.size(); i++) {
            // find closest point
            OsmGraphCore::NearestPointResult nearest;
            if(!service_.findNearestObject(mapName, pts[i], nearest)) {
                respondError(context, ERRORS["FAILED_NEAREST_NEIGHBOR"]);
                return;
            }
            else
                ids.push_back(nearest.getTarget().getId());
        }

        // decode address tags based on points
        vector<TagList> addrTags;
        if(decode) {   
            if(nodeids != "" && !getPoints(context, mapName, ids, pts)) {
                respondError(context, ERRORS["FAILED_ASSIGN_PTS"]);
                return;
            }

            if(!service_.decodeAddress(mapName, pts, addrTags)) {
                respondError(context, ERRORS["FAILED_DECODE_ADDR"]);
                return;
            }
        }

        // at this point we have the ids of the nearest decodable or
        // nearest neighbor points, now just get their tags and/or decode their
        // addresses
        ConditionContainer conds;
        conds.addIdsIn(ids);

        // extract all the tags
        vector<TagList> tags;
        if(!service_.simpleSearch(mapName, source, conds, 0, ids.size(), tags)) {
            respondError(context, ERRORS["FAILED_GET_TAGS"]);
            return;
        }

        // resolve coordinates
        if(!assignPoints(context, mapName, tags)) {
            respondError(context, ERRORS["FAILED_ASSIGN_PTS"]);
            return;
        }

        // if the object type is osm, resolve internal ids to osm ids
        vector<TagList> orig = tags;
        if(source == SqlConsts::OSM_TAG_TABLE && !resolveToOsmIds(context, mapName, ids, tags)) {
            respondError(context, ERRORS["FAILED_RESOLVE_NODEIDS"]);
            return;  
        }

        // add address tags
        for(int i = 0; i < tags.size(); i++) {
            if(decode) {
                tags[i] = filterTagList(tags[i], &TagFilter::isNotAddressTag);
                auto keyVals = addrTags[i].getTags();
                for(auto tag : keyVals) {
                    tags[i].add(tag);
                }
            }
        }

        JSONFormatterNode root("");
        JSONFormatterNode::Nodes nodes = tagsToJSON(tags);
        JSONFormatterNode node("response", nodes);
        root.add(JSONFormatterNode::Nodes({successAttr(), ver(), node}));

        respondContent(context, {}, CTYPE_JSON, root);   
    } else {
        respondError(context, ERRORS["NOT_ENOUGH_ARGS"]);
        return;
    }
}
/**
 * @brief GeoRouting::route
 * @param context
 */
void GeoRouting::route(HttpServerContext* context) {
    typedef Graph<AdjacencyList>::SearchResult OsmSearchResult;
    typedef Graph<AdjacencyListGTFS>::SearchResult GtfsSearchResult;

    // validate request and parse parameters
    string error;
    Metric metric;
    TravelMode mode;
    vector<Point> wayPoints;
    if(!validate(context, wayPoints, mode, metric, error)) {
        respondError(context, ERRORS[error]);
        return;
    }

    string mapType = getAttribute<string>(context, "maptype");
    string mapName = getAttribute<string>(context, "mapname");

    // run algorithms
    if(mapType == "osm") {
        if(service_.existsGraph(mapName, mapType)) {
            vector<OsmSearchResult> searchResults(wayPoints.size()-1, OsmSearchResult());
            runShortestPath(service_.getOsmGraph(mapName), metric, wayPoints, searchResults);
            outputResults(service_.getOsmGraph(mapName), wayPoints, searchResults, context);
        } else {
            respondError(context, ERRORS["GRAPH_MISSING"]);
        }
    } else if(mapType == "gtfs") {
        if(service_.existsGraph(mapName, mapType)) {
            vector<GtfsSearchResult> searchResults(wayPoints.size()-1, GtfsSearchResult());
            runShortestPathPublic(service_.getGtfsGraph(mapName), metric, wayPoints, searchResults);
            outputResults(service_.getGtfsGraph(mapName), wayPoints, searchResults, context);
        } else {
            respondError(context, ERRORS["GRAPH_MISSING"]);
        }
    } else {
        respondError(context, ERRORS["NO_MAP_TYPE"]);
    }
}
/**
 * @brief GeoRouting::serveFile
 * @param context
 */
void GeoRouting::serveFile(HttpServerContext* context) {
    string file = StringUtils::substr(getRequestResource(context), 50);
    // filter .. entries to prevent walking to other dirs than /res
    file = StringUtils::replaceAll(file, "..","");
    vector<string> resPath = {workDir_, Folder::COMMON, Folder::RESOURCE, file};
    string resFile = FsUtil::makePath(resPath);
    respondFile(context, resFile);
}
/**
 * @brief GeoRouting::getTileString
 */
string GeoRouting::getTileString(const string& mapName, const string& layerId, int x, int y, int z, const string& ext) {
    return service_.getTileString(mapName, layerId, x, y, z, true, ext);
}
/**
 * @brief GeoRouting getTileBitmap
 */
mapnik::image_32 GeoRouting::getTileBitmap(const string& mapName, const string& layerId, int x, int y, int z) {
    return service_.getTileBitmap(mapName, layerId, x, y, z);
}
/**
 * @brief GeoRouting::tiles
 * @param context
 */
void GeoRouting::getTile(HttpServerContext *context) {
    if(findKeys(context, {"mapname", "layerid", "x", "y", "z"})) {
        int zoom = getAttribute<int>(context, "z");
        int row = getAttribute<int>(context, "y");
        int column = getAttribute<int>(context, "x");
        string mapName = getAttribute<string>(context, "mapname");
        string layerId = getAttribute<string>(context, "layerid");
        string ext = getAttribute<string>(context, "ext");
        bool force = getAttribute<string>(context, "force") == "true";

        string mime = "image/png";
        if(ext == "") {
            mime = "image/png";
            ext = "png";
        }
        if(ext == "jpeg") {
            mime = "image/jpeg";
            ext = "jpeg";
        }
        if(ext == "svg") {
            mime = "image/svg+xml; charset=UTF-8";
        }

        respondContent(context, {}, mime, service_.getTileString(mapName, layerId, column, row, zoom, force, ext));
    } else {
        respondError(context, "Arguments mapname,layerid,x,y,z were not found");
    }
}
/**
 * @brief GeoRouting::loadTiles
 * @param context
 */
void GeoRouting::loadTiles(WebToolkit::HttpServerContext* context) {
    if(findKeys(context, {"mapname"})) {
        string mapName = context->parameters["mapname"];
        LOGG(Logger::DEBUG) << "Loading tiles for rendering "<<mapName<< Logger::FLUSH;
        if(service_.loadTiles(mapName))
            respondSuccess(context);
        else
            respondError(context, "Couldn't load map "+mapName);
    } else {
        respondError(context, "Argument mapname was not found");
    }
}
/**
 * @brief GeoRouting::setZoom
 * @param context
 */
void GeoRouting::setZoom(HttpServerContext *context) {
    if(findKeys(context, {"mapname", "zoom"})) {
        string mapName = context->parameters["mapname"];
        int zoom = boost::lexical_cast<int>(context->parameters["zoom"]);
        if(service_.setZoom(mapName, zoom))
            respondSuccess(context);
        else
            respondError(context, "Couldn't set zoom for "+mapName+" "+std::to_string(zoom));
    } else {
        respondError(context, "Argument mapname, zoom were not found");
    }
}
/**
 * @brief GeoRouting::listMaps
 * @param context
 */
void GeoRouting::listMaps(HttpServerContext *context) {
    JSONFormatterNode root("");
    JSONFormatterNode::Nodes nodes;

    vector<string> maps;
    vector<string> workDir = {workDir_};
    set<string> dirs = {FsUtil::makePath(workDir)};
    service_.getOfflineMaps("",dirs,maps);

    JSONFormatterNode node("response", maps);
    root.add(JSONFormatterNode::Nodes({successAttr(), ver(), node}));
    respondContent(context, {}, CTYPE_JSON, root);
}
/**
 * @brief GeoRouting::mapicon
 * @param context
 */
void GeoRouting::mapicon(HttpServerContext *context) {
    if(findKeys(context, {"mapname"})) {
        string mapName = getAttribute<string>(context, "mapname");
        string img;
        if(service_.getMapIcon(mapName, img)) {
            respondContent(context, {}, "image/png", img);
        } else {
            // No icon TODO send an empty one

        }
    }
}
/**
 * @brief GeoRouting::matchTags
 * @param context
 */
void GeoRouting::matchTags(WebToolkit::HttpServerContext* context) {
    if(findKeys(context, {"mapname"})) {
        string mapName = getAttribute<string>(context, "mapname");
        string tag = getAttribute<string>(context, "tag");

        vector<string> tags;
        if(!service_.matchTags(mapName, tag, tags)) {
            respondError(context, ERRORS["FAILED_MATCH_TAG"]);
            return;
        }

        JSONFormatterNode root("");
        JSONFormatterNode node("response");
        node.add({JSONFormatterNode("tags", tags)});
        root.add(JSONFormatterNode::Nodes{successAttr(), ver(), node});

        respondContent(context, {}, CTYPE_JSON, root);
    } else
        respondError(context, "Argument id or mapname was not found");
}
/**
 * @brief GeoRouting::download
 * @param context
 */
void GeoRouting::download(HttpServerContext *context) {
    if(findKeys(context, {"mapname","size"})) {
        size_t mbRequired = getAttribute<uint64_t>(context, "size");
        // To download and decompress a map one needs approx 3.5*size MB
        if(!service_.checkAvailableDiskSpace(workDir_, 3.5*mbRequired)) {
            respondError(context, "Not enough disk space");
            return;
        }
        string mapName = getAttribute<string>(context, "mapname");
        int id = -1;
        if(service_.downloadFile(mapName, id)) {
            JSONFormatterNode root("");
            JSONFormatterNode node("response");
            node.add({JSONFormatterNode("id",id)});
            root.add(JSONFormatterNode::Nodes{node});
            respondContent(context, {}, CTYPE_JSON, root);
        } else
            respondError(context, "Failed to download");
    } else
        respondError(context, ERRORS["NOT_ENOUGH_ARGS"]);
}
/**
 * @brief GeoRouting::decompress
 * @param context
 */
void GeoRouting::decompress(HttpServerContext *context) {
    if(findKeys(context, {"mapname"})) {
        string mapName = getAttribute<string>(context, "mapname");
        int id = -1;
        if(service_.decompressFile(mapName, id)) {
            JSONFormatterNode root("");
            JSONFormatterNode node("response");
            node.add({JSONFormatterNode("id",id)});
            root.add(JSONFormatterNode::Nodes{node});
            respondContent(context, {}, CTYPE_JSON, root);
        } else
            respondError(context, "Failed to decompress");
    } else
        respondError(context, "Argument mapname was not found");
}
/**
 * @brief GeoRouting::taskStatus
 * @param context
 */
void GeoRouting::taskStatus(HttpServerContext *context) {
    if(findKeys(context, {"id"})) {
        int id = getAttribute<int>(context, "id");
        double percent = -1.0;
        string status, error;
        if(service_.getTaskStatus(id, percent, status, error)) {
            JSONFormatterNode root("");
            JSONFormatterNode node("response");
            node.add({JSONFormatterNode("id",id),JSONFormatterNode("percent",percent),
                     JSONFormatterNode("status",status),JSONFormatterNode("error",error)});
            root.add(JSONFormatterNode::Nodes{node});
            respondContent(context, {}, CTYPE_JSON, root);
        } else
            respondError(context, "Failed to start downloading");
    } else
        respondError(context, "Argument mapname was not found");
}
/**
 * @brief GeoRouting::writeData
 * @param context
 */
void GeoRouting::writeData(HttpServerContext *context) {
    if(findKeys(context, {"file", "objtype"})) {
        string file = getAttribute<string>(context, "file");
        string table = getAttribute<string>(context, "objtype");
        string id = getAttribute<string>(context, "mapname");
        //bool overwrite = getAttribute<string>(context, "overwrite") == "true";

        TagList tagList;
        set<string> ignored = {"file", "id", "objtype", "overwrite"};
        for(const auto &pair : getAllAttributes(context)) {
            if(ignored.count(pair.first) == 0)
                tagList.add({pair.first, pair.second});
        }

        if(id == "") {
            tagList.setId(Vertex::NullVertexId);
            LOGG(Logger::INFO) << "Creating new value" << Logger::FLUSH;
        } else {
            Vertex::VertexId idd = lexical_cast<Vertex::VertexId>(id);
            if(std::to_string(idd) != id) {
                LOGG(Logger::INFO) << "Non integer id" << Logger::FLUSH;
                respondError(context, "Non integer id");
            }
            tagList.setId(idd);
        }

        if(!service_.writeData(file, table, tagList))
            respondError(context, "Couldn't write data");
        else
            respondSuccess(context);
    } else
        respondError(context, "Argument mapname or object was not found");
}
/**
 * @brief GeoRouting::Handle
 * @param context
 */
void GeoRouting::Handle(HttpServerContext* /*context*/) {
    ;
}
/**
 * @brief GeoRouting::isStarted
 * @return
 */
bool GeoRouting::isStarted() {
    return GeoRouting::serverStarted_;
}
/**
 * @brief GeoRouting::terminate
 * @return
 */
void GeoRouting::terminate(HttpServerContext* /*context*/) {
    std::lock_guard<std::mutex> lk(stopMutex_);
    LOGG(Logger::INFO) << "[SERVER] Terminating..." << Logger::FLUSH;
    GeoRouting::serverStarted_ = false;
    terminateCond_.notify_one();
}
/**
 * @brief GeoRouting::waitForTermination
 */
void GeoRouting::waitForTermination() {
    while(true) {
        std::unique_lock<std::mutex> lk(stopMutex_);
        terminateCond_.wait(lk,[]{return !serverStarted_;});
        break;
    }
}

bool GeoRouting::loadLicense(const string& licenseFilePath,const std::string &packageName) {
    if(!lMgr_.load(licenseFilePath))
        return false;

    // expiration date
    string expirationDateString;
    if(!lMgr_.getString("products.server.expirationDate", expirationDateString)) {
        LOGG(Logger::ERROR) <<  "Failed to read license data" << Logger::FLUSH;
        return false;
    }
    DateTime expirationDate(expirationDateString);

    if(!expirationDate.isValid() || lMgr_.expired(expirationDate)) {
        LOGG(Logger::ERROR) <<  "Demo period expired. Please extend your license on the website http://urbanlabs.in" << Logger::FLUSH;
        return false;
    }
#ifdef ANDROID
    // check package name
    std::set<std::string> pNames;
    if(!lMgr_.getStringSet("products.android.packageNames", pNames)) {
        LOGG(Logger::ERROR) << "Failed to load packageNames from license" << Logger::FLUSH;
        return false;
    }

    if(!pNames.count("*")) {
        if(!pNames.count(packageName)) {
            LOGG(Logger::DEBUG) <<  "License invalid: package" << packageName << "not found" << Logger::FLUSH;
            return false;
        }
    } else {
        LOGG(Logger::DEBUG) <<  "License valid for any package name" << Logger::FLUSH;
    }
    
    // TODO max request count in demo mode
#endif // ANDROID    
    return true;
}
/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char* argv[]) {
    ServerOption sOptions;

    if(!sOptions.init(argc, argv)) {
        std::cout << sOptions.help(true) << std::endl;
        exit(EXIT_FAILURE);
    }

    if(sOptions.has(ServerOption::DEBUG)) {
        INIT_LOGGING(Logger::INFO);
    } else {
        INIT_LOGGING(Logger::PROGRESS);
    }

    // determine parser binary workdir to configure importer and read license
    char* sdkHome = getenv("SPUTNIK_SDK_HOME");
    string serverBinaryworkdir;
    if (sdkHome != NULL) {
        serverBinaryworkdir = string(sdkHome);
    } else {
        if(!FsUtil::currentWorkDir(serverBinaryworkdir)) {
            LOGG(Logger::ERROR) << "Cannot determine current workdir" << Logger::FLUSH;
            exit(EXIT_FAILURE);
        }
    }

    if(sOptions.has(ServerOption::HELP)) {
        std::cout << sOptions.help(true) << std::endl;
        exit(EXIT_SUCCESS);
    }

    if(sOptions.has(ServerOption::VERSION)) {
        std::cout << sOptions.version_detailed() << std::endl;
        exit(EXIT_SUCCESS);
    }
    // validate all arguments before anything else
    if(sOptions.validate() != ServerOption::SUCCESS) {
        std::cout << sOptions.help(true) << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        int port = sOptions.get<int>(ServerOption::PORT);
        GeoRouting app(port, serverBinaryworkdir);

        vector<string> licPathEntries = {serverBinaryworkdir, "config", "license.key"};
        string licPath = FsUtil::makePath(licPathEntries);
        if(!app.loadLicense(licPath)) {
            exit(EXIT_FAILURE);
        }

        app.Run();
        app.waitForTermination();
    }
    catch(exception& e) {
        LOGG(Logger::ERROR) << "Caught exception" << e.what() << Logger::FLUSH;
    }
}
