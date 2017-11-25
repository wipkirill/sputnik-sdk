#include "test_integration.h"

using namespace std;

void TestServer::initTestCase()
{
}
void TestServer::heartBeat_data() {
    QTest::addColumn<QueryArgs>("query");
    QTest::addColumn<QVariantMap>("expected");
    QTest::addColumn<QString>("outputFormat");

    vector<QString> fmts = {"json"};
    for(int i = 0; i < (int)fmts.size(); ++i) {
    	QString fmt = fmts[i];
    	
    	QVariantMap okMap;
	    okMap["ok"] = QVariant(true);

	    QueryArgs empty;
	    QTest::newRow("No args") << empty << okMap << fmt;
    }   
}
void TestServer::heartBeat() {
    QString resp;
    QFETCH(QueryArgs, query);
    QFETCH(QVariantMap, expected);
    QFETCH(QString, outputFormat);
    if(NetworkUtil::request(Sputnik::HEARTBEAT, query, resp))
        Validator::validate(expected, resp, outputFormat);
}


void TestServer::cleanupTestCase()
{
}