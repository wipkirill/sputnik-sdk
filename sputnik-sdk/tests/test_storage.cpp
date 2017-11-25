#include <iostream>

#include "test_storage.h"

#include <UrbanLabs/Sdk/Utils/URL.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Utils/Properties.h>
#include <UrbanLabs/Sdk/Storage/ObjectIdStorage.h>
#include <UrbanLabs/Sdk/Storage/SqliteConnection.h>
#include <UrbanLabs/Sdk/Storage/ConnectionsManager.h>

using namespace std;

void TestObjectIdStorage::test() {
	INIT_LOGGING(Logger::INFO);

	URL url("saar");
	Properties props = {{"create","0"},{"type","sqlite"},{"memory", "0"},{"objectidstorage.table","osm_id"},
						{"objectidstorage.innerid","internal_id"},{"objectidstorage.outerid","osm_id"}};
	
	// test connection
	auto conn_ = ConnectionsManager::getConnection(url, props);
	QVERIFY(conn_->open(url, props));
	QVERIFY(conn_ != 0);

	string table = props.get("objectidstorage.table");
    string inner = props.get("objectidstorage.innerid");
    string outer = props.get("objectidstorage.outerid"); 

    SqlQuery inners = SqlQuery::q().select({inner}).from(table).where(outer+"=?");
    SqlQuery outers = SqlQuery::q().select({inner}).from(table).where(inner+"=?");

    // prepare statement
    unique_ptr<PrepStmt> innerStmt_;
    string innerQuery = inners.toString();
    if(!inners.isValid() || !conn_ || !conn_->prepare(innerQuery, innerStmt_))
        return;

	// initialize storage
	ObjectIdStorage storage;
	QVERIFY(storage.open(url, props));
	QVERIFY(storage.close());
	QVERIFY(conn_->close());
}