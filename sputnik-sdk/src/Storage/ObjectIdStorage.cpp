#include <map>
#include <UrbanLabs/Sdk/Storage/ObjectIdStorage.h>
#include <UrbanLabs/Sdk/Storage/ConnectionsManager.h>

using namespace std;

//
ObjectIdStorage::Initializer::Initializer(const URL &url, const Properties &props)
:  url_(url), props_(props) {;}
//
bool ObjectIdStorage::Initializer::init(ObjectIdStorage &storage) const {
    if(!storage.open(url_, props_)) {
        LOGG(Logger::ERROR) << "Couldnt init tag storage" << Logger::FLUSH;
        storage.close();
        return false;
    }
    return true;
}
//
bool ObjectIdStorage::Destructor::release(ObjectIdStorage &storage) {
    if(!storage.close())
        return false;
    return true;
}
//
bool ObjectIdStorage::open(const URL &url, const Properties &props) {
    // prepare database connection
    if(conn_ && !conn_->close())
        return false;
    conn_ = ConnectionsManager::getConnection(url, props);
    if(!conn_ || !conn_->open(url, props))
        return false;

    table_ = props.get("objectidstorage.table");
    inner_ = props.get("objectidstorage.innerid");
    outer_ = props.get("objectidstorage.outerid");

    return true;
}
//
bool ObjectIdStorage::close() {
    if(conn_ && !conn_->close())
        return false;
    conn_.reset(0);
    return true;
}
//
ObjectIdStorage::~ObjectIdStorage() {
    if(!close())
        LOGG(Logger::ERROR) << "Couldn't close object id storage" << Logger::FLUSH;
}
//
bool ObjectIdStorage::fromInternal(const vector<Vertex::VertexId> &from, vector<string> &to) {
    if(from.size() == 0)
        return true;

    unique_ptr<PrepStmt> stmt;
    SqlQuery query = SqlQuery::q().select({inner_, outer_}).from(table_).in(inner_, from.size());
    if(!conn_  || !query.isValid() || !conn_->prepare(query.toString(), stmt)) {
    	LOGG(Logger::ERROR) << "Couldn't bind: "+query.toString() << Logger::FLUSH;
        return false;
    }

    if(!stmt->bind(from)) {
    	LOGG(Logger::ERROR) << "Couldn't bind: "+query.toString() << Logger::FLUSH;
        return false;
    }

    map<Vertex::VertexId, string> found;
    while(stmt->step()) {
        Vertex::VertexId inner = stmt->column_int64(0);
        string outer = stmt->column_text(1);
        found[inner] = outer;
    }

	for(auto &id : from) {
		if(found.count(id) == 0) {
			LOGG(Logger::ERROR) << "Couldn't resolve id: " << id << Logger::FLUSH;
		}
	}

    to = vector<string>(from.size());
    for(int i = 0; i < from.size(); i++) {
        to[i] = found[from[i]];
    }

    return true;
}
//
bool ObjectIdStorage::toInternal(const vector<string> &from, vector<Vertex::VertexId> &to) {
    if(from.size() == 0)
        return true;
    
    unique_ptr<PrepStmt> stmt;
    SqlQuery query = SqlQuery::q().select({inner_, outer_}).from(table_).in(outer_, from.size());
    if(!conn_  || !query.isValid() || !conn_->prepare(query.toString(), stmt)) {
    	LOGG(Logger::ERROR) << "Couldn't bind: "+query.toString() << Logger::FLUSH;
        return false;
    }

    if(!stmt->bind(from)) {
    	LOGG(Logger::ERROR) << "Couldn't bind: "+query.toString() << Logger::FLUSH;
        return false;
    }

    map<string, Vertex::VertexId> found;
    while(stmt->step()) {
        Vertex::VertexId inner = stmt->column_int64(0);
        string outer = stmt->column_text(1);
        found[outer] = inner;
    }

	for(auto &id : from) {
		if(found.count(id) == 0) {
			LOGG(Logger::ERROR) << "Couldn't resolve id: " << id << Logger::FLUSH;
		}
	}

    to = vector<Vertex::VertexId>(from.size(), Vertex::NullVertexId);
    for(int i = 0; i < from.size(); i++) {
        to[i] = found[from[i]];
    }

    return true;
}