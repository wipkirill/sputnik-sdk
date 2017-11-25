#include "test_search.h"

using namespace std;
/**
*
*/
void TestSearch::initTestCase() {
}
/**
*
*/
void TestSearch::simple_data() {
	QTest::addColumn<QueryArgs>("query");
    QTest::addColumn<QVariantMap>("expected");
    QTest::addColumn<QVariantMap>("tags");
    QTest::addColumn<QString>("outputFormat");

    vector<QString> fmts = {"json"};
    for(int i = 0; i < (int)fmts.size(); ++i) {
    	QString fmt = fmts[i];
    	QVariantMap emptyTags = {};
    	
    	QVariantMap errMap;
	    errMap["ok"] = QVariant(false);

	    // provides empty args to search
	    QueryArgs empty;
	    QTest::newRow("No args") << empty << errMap << emptyTags << fmt;

	    // no maptype
	    QueryArgs noMapType;
	    noMapType << KV("mapname","saar");
	    QTest::newRow("No source type") << noMapType << errMap << emptyTags << fmt;
	    // use tagsvals
		{
		 	// simply find Nilles on Blumenstr
		    QueryArgs simpleArgs;
		    QVariantMap okMap;
		    okMap["ok"] = QVariant(true);
		    simpleArgs << KV("source","osm") << KV("mapname","saar") << KV("q","nilles") << KV("tagsvals","[amenity,bar]");
		    QVariantMap nillesTags;
		    nillesTags["name"] = "nilles";
		    nillesTags["amenity"] = "bar";
		    QTest::newRow("Find Nilles") << simpleArgs << okMap << nillesTags << fmt;

		}
		{
		 	// simply find Johanneskirche
		    QueryArgs simpleArgs;
		    QVariantMap okMap;
		    okMap["ok"] = QVariant(true);
		    simpleArgs << KV("source","osm") << KV("mapname","saar") << KV("q","johanneskirche") << KV("tagsvals","[amenity,place_of_worship]");
		    QVariantMap kircheTags;
		    kircheTags["name"] = "johanneskirche";
		    kircheTags["amenity"] = "place_of_worship";
		    QTest::newRow("Find Johanneskirche") << simpleArgs << okMap << kircheTags << fmt;

		}
	   	{
		 	// simply find Green Buddha
		    QueryArgs simpleArgs;
		    QVariantMap okMap;
		    okMap["ok"] = QVariant(true);
		    simpleArgs << KV("source","osm") << KV("mapname","saar") << KV("q","green") << KV("tagsvals","[amenity,bar]");
		    QVariantMap bTags;
		    bTags["name"] = "green buddha";
		    bTags["amenity"] = "bar";
		    QTest::newRow("Find Green Buddha") << simpleArgs << okMap << bTags << fmt;

		}
		{
		 	// simply find UdS
		    QueryArgs simpleArgs;
		    QVariantMap okMap;
		    okMap["ok"] = QVariant(true);
		    simpleArgs << KV("source","osm") << KV("mapname","saar") << KV("q","universität des saarlandes") << KV("tagsvals","[amenity,university]");
		    QVariantMap uTags;
		    uTags["name"] = "universität des saarlandes";
		    uTags["amenity"] = "university";
		    QTest::newRow("Find UdS") << simpleArgs << okMap << uTags << fmt;
		}
		{
		 	// simply find UdS
		    QueryArgs simpleArgs;
		    QVariantMap okMap;
		    okMap["ok"] = QVariant(true);
		    simpleArgs << KV("source","osm") << KV("mapname","saar") << KV("q","radhaus am rathaus") << KV("tagsvals","[shop,bicycle]");
		    QVariantMap uTags;
		    uTags["name"] = "radhaus am rathaus";
		    uTags["shop"] = "bicycle";
		    QTest::newRow("Find Radhaus am Rathaus") << simpleArgs << okMap << uTags << fmt;
		}

		// Use tags
		{
		 	// simply find bäckerei sander
		    QueryArgs simpleArgs;
		    QVariantMap okMap;
		    okMap["ok"] = QVariant(true);
		    simpleArgs << KV("source","osm") << KV("mapname","saar") << KV("q","bäckerei sander") << KV("tags","[shop]");
		    QVariantMap uTags;
		    uTags["name"] = "bäckerei sander";
		    uTags["shop"] = "bakery";
		    QTest::newRow("Find Bäckerei sander") << simpleArgs << okMap << uTags << fmt;
		}
		{
		 	// simply find Mono
		    QueryArgs simpleArgs;
		    QVariantMap okMap;
		    okMap["ok"] = QVariant(true);
		    simpleArgs << KV("source","osm") << KV("mapname","saar") << KV("q","mono") << KV("tags","[amenity]");
		    QVariantMap uTags;
		    uTags["name"] = "mono";
		    uTags["amenity"] = "bar";
		    QTest::newRow("Find Mono") << simpleArgs << okMap << uTags << fmt;
		}
		{
		 	// simply find nauwieser viertel
		    QueryArgs simpleArgs;
		    QVariantMap okMap;
		    okMap["ok"] = QVariant(true);
		    simpleArgs << KV("source","osm") << KV("mapname","saar") << KV("q","nauwieser viertel") << KV("tags","[place]");
		    QVariantMap uTags;
		    uTags["name"] = "nauwieser viertel";
		    uTags["place"] = "neighbourhood";
		    QTest::newRow("Find Nauwieser viertel") << simpleArgs << okMap << uTags << fmt;
		}
    }   
}
/**
*
*/
void TestSearch::simple() {
    QString resp;
    QFETCH(QueryArgs, query);
    QFETCH(QVariantMap, expected);
    QFETCH(QVariantMap, tags);
    QFETCH(QString, outputFormat);
    if(NetworkUtil::request(Sputnik::SEARCH, query, resp))
        Validator::validate(expected, resp, outputFormat);
   
    QVariantMap output;
    Validator::extractResponse(resp, true, "", outputFormat, output);
    if(output.find("response") != output.end()) {
    	QList<QVariant> list = output["response"].toList();
    	Validator::checkTagsInResponse(list, 0, tags);
    }
}
/**
*
*/
void TestSearch::testMultipleObjects_data() {
    QTest::addColumn<QueryArgs>("query");
    QTest::addColumn<QVariantMap>("expected");
    QTest::addColumn<QVariantMap>("tags");
    QTest::addColumn<QString>("outputFormat");

    vector<QString> fmts = {"json"};
    for(int i = 0; i < (int)fmts.size(); ++i) {
    	QString fmt = fmts[i];
    	QVariantMap emptyTags = {};

	    // use tagsvals
		{
		 	// simply find Nilles on Blumenstr
		    QueryArgs simpleArgs;
		    QVariantMap okMap;
		    okMap["ok"] = QVariant(true);
		    simpleArgs << KV("source","osm") << KV("mapname","saar") << KV("q","netto") << KV("tagsvals","[shop,supermarket]");
		    QVariantMap nettoTags;
		    nettoTags["name"] = "netto";
		    nettoTags["shop"] = "supermarket";
		    QTest::newRow("Find Netto") << simpleArgs << okMap << nettoTags << fmt;
		}
		{
		 	// simply find Nilles on Blumenstr
		    QueryArgs simpleArgs;
		    QVariantMap okMap;
		    okMap["ok"] = QVariant(true);
		    simpleArgs << KV("source","osm") << KV("mapname","saar") << KV("q","rewe") << KV("tagsvals","[shop,supermarket]");
		    QVariantMap nettoTags;
		    nettoTags["name"] = "rewe";
		    nettoTags["shop"] = "supermarket";
		    QTest::newRow("Find Rewe") << simpleArgs << okMap << nettoTags << fmt;
		}
    } 
}
/**
*
*/
void TestSearch::testMultipleObjects() {
    QString resp;
    QFETCH(QueryArgs, query);
    QFETCH(QVariantMap, expected);
    QFETCH(QVariantMap, tags);
    QFETCH(QString, outputFormat);
    if(NetworkUtil::request(Sputnik::SEARCH, query, resp))
        Validator::validate(expected, resp, outputFormat);
   
    QVariantMap output;
    Validator::extractResponse(resp, true, "", outputFormat, output);
    if(output.find("response") != output.end()) {
    	QList<QVariant> list = output["response"].toList();
    	Validator::checkTagsInResponse(list, -1, tags);
    }
}
/**
*
*/
void TestSearch::testSearchByIds() {
	TEST(Sputnik::SEARCH);
}
/**
*
*/
void TestSearch::testSearchByIds_data() {
	QTest::addColumn<QueryArgs>("query");
    QTest::addColumn<QVariantMap>("expected");
    QTest::addColumn<QString>("outputFormat");

    vector<QString> fmts = {"json"};
    for(int i = 0; i < (int)fmts.size(); ++i) {
    	QString fmt = fmts[i];

	    // a valid sample
	    QVariantMap okMap;
	    okMap["ok"] = QVariant(true);
	    QueryArgs validArgs;


	    validArgs << KV("source","osm") << KV("mapname", "saar") <<
	    KV("ids","[w40506536,w50029776,w59147369,w42859225,w83219996,w83220011,w83686786,w24634726,w117607685,w35680716,w89206406,w40530481,w89206386,w124841388,w117608225,w24004286,w124841389,w24244512,w12374203,w157594654,w59135862,w157594651,w204673868,w204676963,w11824659,w204680976,w14331006,w31584719,w15270085,w204683255,w204683257,w51007344,w144521836,w144521838,w13990874,w111489651,w24244519,w123432546,w13990878,w51066404,w122498547,w50085755,w122498546,w56024577,w24504797,w208350826,w50085751,w219935364,w125528928,w26761695,w26761696,w24292533,w26761694,w12982456,w59280462,w141792918,w141792916,w141792914,w34993156,w23289813,w59098855,w50087541,w24983645,w23698382,w26794462,w26794461,w23546502,w31242663,w23546504]");
	    QTest::newRow("Ids") << validArgs << okMap  << fmt;
    } 
}
/**
*
*/
void TestSearch::testNearestObjectBasic() {
	QString resp;
    QFETCH(QueryArgs, query);
    QFETCH(QVariantMap, expected);
    QFETCH(QVariantMap, tags);
    QFETCH(QString, outputFormat);
    if(NetworkUtil::request(Sputnik::NEARESTOBJECT, query, resp))
        Validator::validate(expected, resp, outputFormat);
   
    QVariantMap output;
    Validator::extractResponse(resp, true, "", outputFormat, output);
    if(output.find("response") != output.end()) {
    	QList<QVariant> list = output["response"].toList();
    	Validator::checkTagsInResponse(list, 0, tags);
    }
}
/**
*
*/
void TestSearch::testNearestObjectBasic_data() {
	QTest::addColumn<QueryArgs>("query");
    QTest::addColumn<QVariantMap>("expected");
    QTest::addColumn<QVariantMap>("tags");
    QTest::addColumn<QString>("outputFormat");

    vector<QString> fmts = {"json"};
    for(int i = 0; i < (int)fmts.size(); ++i) {
    	QString fmt = fmts[i];

	    // a valid sample
	    QVariantMap okMap;
	    okMap["ok"] = QVariant(true);
	    QueryArgs validArgs;

	    QVariantMap uTags;
	    uTags["addr:country"] = "de";
		uTags["addr:city"] = "saarbrücken";
		uTags["addr:housenumber"] = "2";
		uTags["addr:postcode"] = "66111";
		uTags["addr:street"] = "cecilienstraße";
		uTags["addr:suburb"] = "st. johann";
		uTags["amenity"] = "place_of_worship";
		uTags["name"] = "johanneskirche";

	    validArgs << KV("source","osm") << KV("mapname", "saar") <<
	    KV("points","[[49.23589,6.99677]]") << KV("decode","true");
	    QTest::newRow("Valid") << validArgs << okMap << uTags << fmt;
    } 
}
/**
*
*/
void TestSearch::testNearestObject() {
	TEST(Sputnik::NEARESTOBJECT);
}
/**
*
*/
void TestSearch::testNearestObject_data() {
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
	    QTest::newRow("No source") << noMapType << errMap << fmt;

	    // no mapname
	    QueryArgs noMapName;
	    noMapName << KV("source","osm");
	    QTest::newRow("No mapname") << noMapName << errMap << fmt;

	    // wrong maptype
	    QueryArgs wrongMapType;
	    wrongMapType << KV("source","2312") << KV("mapname", "saar");
	    QTest::newRow("Wrong source") << wrongMapType << errMap << fmt;

	    // wrong mapname
	    QueryArgs wrongMapName;
	    wrongMapName << KV("source","osm") << KV("mapname", "sdfsdf");
	    QTest::newRow("Wrong mapname") << wrongMapName << errMap << fmt;
    } 
}
/**
*
*/
void TestSearch::cleanupTestCase() {
}