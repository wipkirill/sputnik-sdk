#pragma once

#include <WebToolkit/WebToolkit.h>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/GraphCore/Model.h>
#include <UrbanLabs/Sdk/GraphCore/Graph.h>
#include <UrbanLabs/Sdk/OSM/TagFilter.h>
#include <UrbanLabs/Sdk/Utils/LicenseManager.h>
#include <WebService/ServerOption.h>
#include <Api/Services.h>

/**
 * @brief The GeoRouting class
 * Description of GeoRouting server interface
 */
class GeoRouting : public WebToolkit::HttpHandler {
public:
    enum class TravelMode : int {
        CAR = 0,
        PEDESTRIAN = 1,
        BICYCLE = 1,
        PUBLICTRANSPORT = 2,
        DEFAULT = 0
    };

    enum class Metric : int {
        TIME = 1,
        DISTANCE = 0,
        DEFAULT = 0
    };
private:
    std::string WAYPOINTS;
    std::string TRAVEL_MODE;
    std::string METRIC;
    std::string ALTERNATIVE;
private:
    std::string CTYPE_JSON;
private:
    static std::set<std::string> travelModes_;
    static std::set<std::string> metricModes_;
    static std::map<std::string, GeoRouting::TravelMode> modeMap_;
    static std::map<std::string, GeoRouting::Metric> metricMap_;
    static std::map<std::string, std::string> ERRORS;
private:
    LicenseManager lMgr_;
    Service service_;
    TagFilter tagFilter_;
    WebToolkit::Server server_;
    static bool serverStarted_;
    std::mutex stopMutex_;
    std::condition_variable terminateCond_;
    // application working directory(searches for maps and other resources)
    std::string workDir_;
    WebToolkit::URIDispatcher dispatcher_;
    Graph<AdjacencyListGTFS> graphGTFS_;
    bool validate(const WebToolkit::HttpServerContext* context, vector<Point> &wayPoints,
                  GeoRouting::TravelMode &mode, GeoRouting::Metric &metric, std::string& error) const;

    template<typename G>
    void outputResults(G &graph, const vector<Point> &wayPoints,
                       const vector<typename G::SearchResult> &searchResults,
                       WebToolkit::HttpServerContext *context);
    template<typename G>
    void runShortestPath(G &graph, const GeoRouting::Metric &metric, vector<Point> &wayPoints,
                         vector<typename G::SearchResult> &searchResults);
    template<typename G>
    void runShortestPathPublic(G &graph, const GeoRouting::Metric &metric, vector<Point> &wayPoints,
                               vector<typename G::SearchResult> &searchResults);
private:
    GeoRouting::TravelMode getTravelMode(const std::map<std::string,std::string> &request) const;
    GeoRouting::Metric getMetric(const std::map<std::string,std::string> &request) const;
    template<typename T>
    JSONFormatterNode::Nodes tagsToJSON(std::vector<T> &tags);
    JSONFormatterNode::Node successAttr() const;
    JSONFormatterNode::Node ver() const;
public:
    GeoRouting(int port, const string &workDir);
    static GeoRouting* instance_;
    bool loadLicense(const string& licenseFilePath, const std::string &packageName = "");
    void Run();
    void Handle(WebToolkit::HttpServerContext* context);
    void Init();
    void heartBeat(WebToolkit::HttpServerContext* context);
    // routing
    void route(WebToolkit::HttpServerContext* context);
    void unloadGraph(WebToolkit::HttpServerContext *context);
    void loadGraph(WebToolkit::HttpServerContext *context);
    void getLoadedGraphs(WebToolkit::HttpServerContext *context);
    void nearestNeighbor(WebToolkit::HttpServerContext* context);
    // search
    void nearestObject(WebToolkit::HttpServerContext* context);
    bool assignPoints(WebToolkit::HttpServerContext *context, const std::string &mapName, std::vector<TagList> &tags);
    bool getPoints(WebToolkit::HttpServerContext *context, const std::string &mapName, 
                   const std::vector<Vertex::VertexId> &ids, std::vector<Point> &points);
    bool resolveToOsmIds(WebToolkit::HttpServerContext *context, const std::string &mapName, 
                         const std::vector<Vertex::VertexId> &ids, std::vector<TagList> &tags);
    void getTags(WebToolkit::HttpServerContext *context);
    void search(WebToolkit::HttpServerContext *context);
    void matchTags(WebToolkit::HttpServerContext* context);
    // pages
    void index(WebToolkit::HttpServerContext* context);
    void menu(WebToolkit::HttpServerContext* context);
    void errorPage(WebToolkit::HttpServerContext* context);
    void serveFile(WebToolkit::HttpServerContext* context);
    // tiles
    void loadTiles(WebToolkit::HttpServerContext* context);
    void setZoom(WebToolkit::HttpServerContext* context);
    void getTile(WebToolkit::HttpServerContext *context);
    std::string getTileString(const string& mapName, const string& layerId, int x, int y, int z, const string& ext);
    mapnik::image_32 getTileBitmap(const string& mapName, const string& layerId, int x, int y, int z);
    // maps
    void listMaps(WebToolkit::HttpServerContext* context);
    void mapicon(WebToolkit::HttpServerContext* context);
    void getMapInfo(WebToolkit::HttpServerContext *context);
    // functional
    void download(WebToolkit::HttpServerContext* context);
    void decompress(WebToolkit::HttpServerContext* context);
    void taskStatus(WebToolkit::HttpServerContext* context);
    void writeData(WebToolkit::HttpServerContext* context);
    void resetServices(WebToolkit::HttpServerContext* context);
    void getConfig(WebToolkit::HttpServerContext* context);
    // status of the server
    static bool isStarted();
    void terminate(WebToolkit::HttpServerContext *context);
    void waitForTermination();
    // helper methods
    bool findKeys(const WebToolkit::HttpServerContext *context, const vector<string> &keys) const;
    bool findKey(const WebToolkit::HttpServerContext *context, const std::string & key) const;
    std::string getProperty(const std::string &, const std::map<std::string,std::string> &) const;
    void respondError(WebToolkit::HttpServerContext *context, const std::string& errMsg) const;
    void respondSuccess(WebToolkit::HttpServerContext *context) const;
    void respondFile(WebToolkit::HttpServerContext *context, const std::string & fileName) const;
    template<typename T>
    void respondContent(WebToolkit::HttpServerContext *context,
                        const std::map<std::string,std::string> &headers, const std::string &contentType,
                        const T &content) const;

    template<typename T>
    std::vector<T> getAttributes(const WebToolkit::HttpServerContext *context, const std::string &name) const;
    template<typename T>
    T getAttribute(const WebToolkit::HttpServerContext *context, const std::string &name) const;
    TagList filterTagList(TagList & tagList, bool (TagFilter::* func)(const string &));
    bool sortAccording(const std::vector<Vertex::VertexId> &ids, std::vector<TagList> &tagList) const;
    void sortAccording(const Point &pt, std::vector<TagList> &tagList) const;
    std::map<std::string, std::string> getAllAttributes(const WebToolkit::HttpServerContext *context) const;
    std::string getRequestResource(const WebToolkit::HttpServerContext *context) const;
};

