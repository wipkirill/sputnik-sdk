#include "test_graph_routing.h"

using namespace std;

void TestGraphRouting::initTestCase()
{
}
void TestGraphRouting::loadGraph_data() {
	QTest::addColumn<QueryArgs>("query");
    QTest::addColumn<QVariantMap>("expected");
    QTest::addColumn<QString>("outputFormat");

    vector<QString> fmts = {"json"};
    for(int i = 0; i < (int)fmts.size(); ++i) {
    	QString fmt = fmts[i];
    	
    	QVariantMap errMap;
	    errMap["ok"] = QVariant(false);

	    // provides empty args to loadgraph
	    QueryArgs empty;
	    QTest::newRow("No args") << empty << errMap << fmt;

	    // no maptype
	    QueryArgs noMapType;
	    noMapType << KV("mapname","saar");
	    QTest::newRow("No maptype") << noMapType << errMap << fmt;

	    // no mapname
	    QueryArgs noMapName;
	    noMapName << KV("maptype","osm");
	    QTest::newRow("No mapname") << noMapName << errMap << fmt;

	    // wrong maptype
	    QueryArgs wrongMapType;
	    wrongMapType << KV("maptype","2312") << KV("mapname", "saar");
	    QTest::newRow("Wrong maptype") << wrongMapType << errMap << fmt;

	    // wrong mapname
	    QueryArgs wrongMapName;
	    wrongMapName << KV("maptype","osm") << KV("mapname", "sdfsdf");
	    QTest::newRow("Wrong mapname") << wrongMapName << errMap << fmt;

	    // a valid sample
	    QVariantMap okMap;
	    okMap["ok"] = QVariant(true);
	    QueryArgs validArgs;
	    validArgs << KV("maptype","osm") << KV("mapname", "saar");
	    QTest::newRow("Valid") << validArgs << okMap << fmt;
    }   
}

void TestGraphRouting::loadGraph() {
    TEST(Sputnik::LOADGRAPH);
}

void TestGraphRouting::getLoadedGraphs_data() {
    QTest::addColumn<QueryArgs>("query");
    QTest::addColumn<QVariantMap>("expected");
    QTest::addColumn<QString>("outputFormat");

    vector<QString> fmts = {"json"};
    for(int i = 0; i < (int)fmts.size(); ++i) {
    	QString fmt = fmts[i];
    	
    	QVariantMap errMap;
	    errMap["ok"] = QVariant(false);

	    // provides empty args to loadgraph
	    QueryArgs empty;
	    QTest::newRow("No args") << empty << errMap << fmt;

	    // no maptype
	    QueryArgs noMapType;
	    noMapType << KV("mapname","saar");
	    QTest::newRow("No maptype") << noMapType << errMap << fmt;

	    // a valid sample
	    QVariantMap okMap;
	    okMap["ok"] = QVariant(true);

	    QList<QString> graphs = {"saar"};
	    okMap["response"] = QVariant(graphs);
	    QueryArgs validArgs;
	    validArgs << KV("maptype","osm");
	    QTest::newRow("Valid") << validArgs << okMap << fmt;
    }   
}

void TestGraphRouting::getLoadedGraphs() {
    TEST(Sputnik::GETLOADEDGRAPHS);
}


void TestGraphRouting::nearestNeighbor_data() {
    QTest::addColumn<QueryArgs>("query");
    QTest::addColumn<QVariantMap>("expected");
    QTest::addColumn<QString>("outputFormat");

    vector<QString> fmts = {"json"};
    for(int i = 0; i < (int)fmts.size(); ++i) {
    	QString fmt = fmts[i];
    	
    	QVariantMap errMap;
	    errMap["ok"] = QVariant(false);

	    // provides empty args to loadgraph
	    QueryArgs empty;
	    QTest::newRow("No args") << empty << errMap << fmt;

	    // no maptype
	    QueryArgs noMapType;
	    noMapType << KV("mapname","saar");
	    QTest::newRow("No maptype") << noMapType << errMap << fmt;

	    // no mapname
	    QueryArgs noMapName;
	    noMapName << KV("maptype","osm");
	    QTest::newRow("No mapname") << noMapName << errMap << fmt;

	    // no mapname
	    QueryArgs noLatLon;
	    noLatLon << KV("maptype","osm") << KV("mapname","saar");
	    QTest::newRow("No lat lon") << noLatLon << errMap << fmt;

	    // a valid sample
	    QVariantMap okMap;
	    okMap["ok"] = QVariant(true);
	    QVariantMap response;
    	response["id"] = (long long)2282882739;
    	response["lat"] = 49.2310656;
    	response["lon"] = 6.9710448;
    	okMap["response"] = response;
	    QueryArgs validArgs;

	    validArgs << KV("maptype","osm") << KV("mapname","saar")
	    	<<KV("lat","49.2310715") << KV("lon","6.97104454");
	    QTest::newRow("Valid") << validArgs << okMap << fmt;
    }   
}

void TestGraphRouting::nearestNeighbor() {
    TEST(Sputnik::NEARESTNEIGHBOR);
}

void TestGraphRouting::route() {
    TEST(Sputnik::ROUTE);
}


void TestGraphRouting::route_data() {
    QTest::addColumn<QueryArgs>("query");
    QTest::addColumn<QVariantMap>("expected");
    QTest::addColumn<QString>("outputFormat");

    vector<QString> fmts = {"json"};
    for(int i = 0; i < (int)fmts.size(); ++i) {
    	QString fmt = fmts[i];
    	
    	QVariantMap errMap;
	    errMap["ok"] = QVariant(false);

	    // provides empty args to loadgraph
	    QueryArgs empty;
	    QTest::newRow("No args") << empty << errMap << fmt;

	    // no maptype
	    QueryArgs noMapType;
	    noMapType << KV("mapname","saar");
	    QTest::newRow("No maptype") << noMapType << errMap << fmt;

	    // no mapname
	    QueryArgs noMapName;
	    noMapName << KV("maptype","osm");
	    QTest::newRow("No mapname") << noMapName << errMap << fmt;

	    // no mapname
	    QueryArgs noLatLon;
	    noLatLon << KV("maptype","osm") << KV("mapname","saar");
	    QTest::newRow("No lat lon") << noLatLon << errMap << fmt;

	    // a valid sample from Blumenstr to MPI
	    QVariantMap okMap;
	    okMap["ok"] = QVariant(true);
	    QueryArgs validArgs;

	    validArgs << KV("maptype","osm") << KV("mapname","saar")
	    	<<KV("waypoints","49.2367973,6.99887466|49.2573738,7.04673719");
	    QTest::newRow("A valid route from Blumenstr to MPI") << validArgs << okMap << fmt;
    }   
}

void TestGraphRouting::unloadGraphs_data() {
    QTest::addColumn<QueryArgs>("query");
    QTest::addColumn<QVariantMap>("expected");
    QTest::addColumn<QString>("outputFormat");

    vector<QString> fmts = {"json"};
    for(int i = 0; i < (int)fmts.size(); ++i) {
    	QString fmt = fmts[i];
    	
    	QVariantMap errMap;
	    errMap["ok"] = QVariant(false);

	    // provides empty args to loadgraph
	    QueryArgs empty;
	    QTest::newRow("No args") << empty << errMap << fmt;

	    // no maptype
	    QueryArgs noMapType;
	    noMapType << KV("mapname","saar");
	    QTest::newRow("No maptype") << noMapType << errMap << fmt;

	    // a valid sample
	    QVariantMap okMap;
	    okMap["ok"] = QVariant(true);

	    QueryArgs validArgs;
	    validArgs << KV("maptype","osm") << KV("mapname","saar");
	    QTest::newRow("Valid") << validArgs << okMap << fmt;
    }   
}

void TestGraphRouting::unloadGraphs() {
    TEST(Sputnik::UNLOADGRAPHS);
}

void TestGraphRouting::listMaps_data() {
    QTest::addColumn<QueryArgs>("query");
    QTest::addColumn<QVariantMap>("expected");
    QTest::addColumn<QString>("outputFormat");

    vector<QString> fmts = {"json"};
    for(int i = 0; i < (int)fmts.size(); ++i) {
    	QString fmt = fmts[i];

	    // a valid sample
	    QVariantMap okMap;
	    okMap["ok"] = QVariant(true);
	    QList<QString> maps = {"saar"};
	    okMap["response"] = QVariant(maps);
	    QueryArgs validArgs;
	    QTest::newRow("Valid") << validArgs << okMap << fmt;
    }   
}

void TestGraphRouting::listMaps() {
    TEST(Sputnik::LISTMAPS);
}

void TestGraphRouting::mapInfo_data() {
    QTest::addColumn<QueryArgs>("query");
    QTest::addColumn<QVariantMap>("expected");
    QTest::addColumn<QString>("outputFormat");

    vector<QString> fmts = {"json"};
    for(int i = 0; i < (int)fmts.size(); ++i) {
    	QString fmt = fmts[i];
    	
    	QVariantMap errMap;
	    errMap["ok"] = QVariant(false);

	    // provides empty args to loadgraph
	    QueryArgs empty;
	    QTest::newRow("No args") << empty << errMap << fmt;

	     // a valid sample
	    QVariantMap okMap;
	    okMap["ok"] = QVariant(true);
	    QueryArgs validArgs;
	    validArgs << KV("mapname","saar");
	    QTest::newRow("Valid") << validArgs << okMap << fmt;
    }   
}

void TestGraphRouting::mapInfo() {
    TEST(Sputnik::MAPINFO);
}

void TestGraphRouting::cleanupTestCase()
{
}