#include <sstream>

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/Types.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Storage/SqliteConnection.h>
#include <UrbanLabs/Sdk/Storage/SqlQuery.h>

using std::map;
using std::set;
using std::string;
using std::unique_ptr;
using std::vector;

template<typename T>
void releaseMemory(T &t) {
    T dummy;
    std::swap(t, dummy);
}

// low level sqlite error codes
static map<int, string> ERROR_CODES = {
  //{SQLITE_OK, "Successful result "}, 
  {SQLITE_ERROR, "SQL error or missing database "},
  {SQLITE_INTERNAL, "Internal logic error in SQLite "},
  {SQLITE_PERM, "Access permission denied "},
  {SQLITE_ABORT, "Callback routine requested an abort "},
  {SQLITE_BUSY, "The database file is locked "},
  {SQLITE_LOCKED, "A table in the database is locked "},
  {SQLITE_NOMEM, "A malloc() failed "},
  {SQLITE_READONLY, "Attempt to write a readonly database "},
  {SQLITE_INTERRUPT, "Operation terminated by sqlite3_interrupt()"},
  {SQLITE_IOERR, "Some kind of disk I/O error occurred "},
  {SQLITE_CORRUPT, "The database disk image is malformed "},
  {SQLITE_NOTFOUND, "Unknown opcode in sqlite3_file_control() "},
  {SQLITE_FULL, "Insertion failed because database is full "},
  {SQLITE_CANTOPEN, "Unable to open the database file "},
  {SQLITE_PROTOCOL, "Database lock protocol error "},
  {SQLITE_EMPTY, "Database is empty "},
  {SQLITE_SCHEMA, "The database schema changed "},
  {SQLITE_TOOBIG, "String or BLOB exceeds size limit "},
  {SQLITE_CONSTRAINT, "Abort due to constraint violation "},
  {SQLITE_MISMATCH, "Data type mismatch "},
  {SQLITE_MISUSE, "Library used incorrectly "},
  {SQLITE_NOLFS, "Uses OS features not supported on host "},
  {SQLITE_AUTH, "Authorization denied "},
  {SQLITE_FORMAT, "Auxiliary database format error "},
  {SQLITE_RANGE, "2nd parameter to sqlite3_bind out of range "},
  {SQLITE_NOTADB, "File opened that is not a database file "},
  {SQLITE_NOTICE, "Notifications from sqlite3_log() "},
  {SQLITE_WARNING, "Warnings from sqlite3_log() "},
  //{SQLITE_ROW, "sqlite3_step() has another row ready "},
  //{SQLITE_DONE, "sqlite3_step() has finished executing "}
};

void loggIt(int code) {
    if(ERROR_CODES.count(code)) {
        LOGG(Logger::ERROR) << "[SQLITE LOWLEVEL] " << ERROR_CODES[code] << Logger::FLUSH;
    }
}

//------------------------------------------------------------------------------
// Sqlite Prepared statement
//------------------------------------------------------------------------------
/**
 * @brief SqliteStmt::SqliteStmt
 */
SqliteStmt::SqliteStmt(const string &query, sqlite3_stmt *raw, SqliteConn *conn)
    : offset_(1), finalized_(false), query_(query), conn_(conn), stmt_(raw) {;}
/**
 * @brief SqliteStmt::~SqliteStmt
 */
SqliteStmt::~SqliteStmt() {
    finalize();
}
/**
 * @brief SqliteStmt::bind_int
 * @return
 */
bool SqliteStmt::bind_int(int col, int32_t param, int times) {
    for(int t = 0; t < times; t++)
        if(sqlite3_bind_int(stmt_, col, param) != SQLITE_OK) {
            LOGG(Logger::ERROR) << "couldn't bind the statement: "
                                << sqlite3_errmsg(conn_->getDB()) << Logger::FLUSH;
            if(sqlite3_reset(stmt_) != SQLITE_OK) {
                LOGG(Logger::ERROR) << "couldn't reset the statement: "
                                    << sqlite3_errmsg(conn_->getDB()) << Logger::FLUSH;
            }
            return false;
        }
    return true;
}
/**
 * @brief SqliteStmt::bind_int64
 * @return
 */
bool SqliteStmt::bind_int64(int col, int64_t param, int times) {
    for(int t = 0; t < times; t++)
        if(sqlite3_bind_int64(stmt_, col, param) != SQLITE_OK) {
            LOGG(Logger::ERROR) << "couldn't bind the statement: "
                                << sqlite3_errmsg(conn_->getDB()) << Logger::FLUSH;
            if(sqlite3_reset(stmt_) != SQLITE_OK) {
                LOGG(Logger::ERROR) << "couldn't reset the statement: "
                                    << sqlite3_errmsg(conn_->getDB()) << Logger::FLUSH;
            }
            return false;
        }
    return true;
}
/**
 * @brief SqliteStmt::bind_text
 * @return
 */
bool SqliteStmt::bind_text(int col, const string &param, int times) {
    for(int t = 0; t < times; t++)
        if(sqlite3_bind_text(stmt_, col, param.c_str(), param.size(), SQLITE_TRANSIENT) != SQLITE_OK) {
            LOGG(Logger::ERROR) << "couldn't bind the statement: "
                                << sqlite3_errmsg(conn_->getDB()) << Logger::FLUSH;
            if(sqlite3_reset(stmt_) != SQLITE_OK) {
                LOGG(Logger::ERROR) << "couldn't reset the statement: "
                                    << sqlite3_errmsg(conn_->getDB()) << Logger::FLUSH;
            }
            return false;
        }
    return true;
}
/**
 * @brief bind_blob
 * @param param
 * @param len
 * @param col
 * @return
 */
bool SqliteStmt::bind_blob(int col, const uint8_t* param, size_t len, int times) {
    for(int t = 0; t < times; t++)
        if(sqlite3_bind_blob(stmt_, col, param, len, SQLITE_TRANSIENT) != SQLITE_OK) {
            LOGG(Logger::ERROR) << "couldn't bind the statement: "
                                << sqlite3_errmsg(conn_->getDB()) << Logger::FLUSH;
            if(sqlite3_reset(stmt_) != SQLITE_OK) {
                LOGG(Logger::ERROR) << "couldn't reset the statement: "
                                    << sqlite3_errmsg(conn_->getDB()) << Logger::FLUSH;
            }
            return false;
        }
    return true;
}
/**
 * @brief SqliteStmt::bind_double
 * @param param
 * @param col
 * @return
 */
bool SqliteStmt::bind_double(int col, double param, int times) {
    for(int t = 0; t < times; t++)
        if(sqlite3_bind_double(stmt_, col, param) != SQLITE_OK) {
            LOGG(Logger::ERROR) << "couldn't bind the statement: "
                                << sqlite3_errmsg(conn_->getDB()) << Logger::FLUSH;
            if(sqlite3_reset(stmt_) != SQLITE_OK) {
                LOGG(Logger::ERROR) << "couldn't reset the statement: "
                                    << sqlite3_errmsg(conn_->getDB()) << Logger::FLUSH;
            }
            return false;
        }
    return true;
}
/**
 * @brief SqliteStmt::bind
 * @param param
 * @param times
 * @return
 */
bool SqliteStmt::bind(int32_t param, int times) {
    for(int t = 0; t < times; t++)
        if(!bind_int(offset_+t, param, 1))
            return false;
    offset_ += times;
    return true;
}
/**
 * @brief bind
 * @param param
 * @param times
 * @return
 */
bool SqliteStmt::bind(uint32_t param, int times) {
    for(int t = 0; t < times; t++)
        if(!bind_int64(offset_+t, param, 1))
            return false;
    offset_ += times;
    return true;
}
/**
 * @brief SqliteStmt::bind
 * @param param
 * @param times
 * @return
 */
bool SqliteStmt::bind(int64_t param, int times) {
    for(int t = 0; t < times; t++)
        if(!bind_int64(offset_+t, param, 1))
            return false;
    offset_ += times;
    return true;
}
/**
 * @brief SqliteStmt::bind
 * @param param
 * @param times
 * @return
 */
bool SqliteStmt::bind(uint64_t param, int times) {
    for(int t = 0; t < times; t++)
        if(!bind_int64(offset_+t, param, 1))
            return false;
    offset_ += times;
    return true;
}
/**
 * @brief SqliteStmt::bind
 * @param col
 * @param param
 * @param times
 * @return
 */
bool SqliteStmt::bind(const std::vector<int32_t> &param, int times) {
    if(param.size() == 0)
        return false;

    for(int t = 0; t < times; t++) {
        for(int p = 0; p < param.size(); p++)
            if(!bind_int(offset_+p, param[p], 1))
                return false;
        offset_ += param.size();
    }
    return true;
}
/**
 * @brief SqliteStmt::bind
 * @param param
 * @param times
 * @return
 */
bool SqliteStmt::bind(const std::vector<int64_t> &param, int times) {
    if(param.size() == 0)
        return false;

    for(int t = 0; t < times; t++) {
        for(int p = 0; p < param.size(); p++)
            if(!bind_int64(offset_+p, param[p], 1))
                return false;
        offset_ += param.size();
    }
    return true;
}
/**
 * @brief SqliteStmt::bind
 * @param param
 * @param times
 * @return
 */
bool SqliteStmt::bind(const string &param, int times) {
    for(int t = 0; t < times; t++)
        if(!bind_text(offset_+t, param, 1))
            return false;
    offset_ += times;
    return true;
}
/**
 * @brief SqliteStmt::bind
 * @param param
 * @param times
 * @return
 */
bool SqliteStmt::bind(const vector<string> &param, int times) {
    if(param.size() == 0)
        return false;

    for(int t = 0; t < times; t++) {
        for(int p = 0; p < param.size(); p++)
            if(!bind_text(offset_+p, param[p], 1))
                return false;
        offset_ += param.size();
    }
    return true;
}
/**
 * @brief SqliteStmt::bind
 * @param param
 * @param len
 * @param times
 * @return
 */
bool SqliteStmt::bind(const uint8_t* param, size_t len, int times) {
    for(int t = 0; t < times; t++)
        if(!bind_blob(offset_+t, param, len, 1))
            return false;
    offset_ += times;
    return true;
}
/**
 * @brief SqliteStmt::bind
 * @param param
 * @param times
 * @return
 */
bool SqliteStmt::bind(double param, int times) {
    for(int t = 0; t < times; t++)
        if(!bind_double(offset_+t, param, 1))
            return false;
    offset_ += times;
    return true;
}
/**
 * @brief SqliteStmt::column_int
 * @return
 */
int32_t SqliteStmt::column_int(int col) {
    return sqlite3_column_int(stmt_, col);
}
/**
 * @brief SqliteStmt::column_int64
 * @return
 */
int64_t SqliteStmt::column_int64(int col) {
    return sqlite3_column_int64(stmt_, col);
}
/**
 * @brief SqliteStmt::column_text
 * @return
 */
string SqliteStmt::column_text(int col) {
    // TODO: fix conversion to unique_ptr<uint8_t> ??
    size_t len = sqlite3_column_bytes(stmt_, col);
    const unsigned char *par = sqlite3_column_text(stmt_, col);
    return string(reinterpret_cast<const char*>(par), len);
}
/**
 * @brief SqliteStmt::column_text
 * @return
 */
string SqliteStmt::column_blob(int col) {
    // TODO: fix conversion to unique_ptr<uint8_t> ??
    size_t len = sqlite3_column_bytes(stmt_, col);
    const char *par = reinterpret_cast<const char*>(sqlite3_column_blob(stmt_, col));
    return string(par, len);
}
/**
 * @brief SqliteStmt::column_double
 * @param col
 * @return
 */
double SqliteStmt::column_double(int col) {
    return sqlite3_column_double(stmt_, col);
}
/**
 * @brief SqliteStmt::column_count
 * @return
 */
int SqliteStmt::column_count() {
    return sqlite3_column_count(stmt_);
}
/**
 * @brief SqliteStmt::step
 * @return
 */
bool SqliteStmt::step() {
    int rc = sqlite3_step(stmt_);
    if(rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE) {
        LOGG(Logger::ERROR) << "when stepping: "
                            << sqlite3_errmsg(conn_->getDB()) << Logger::FLUSH;
        return 0;
    }
    if(rc == SQLITE_DONE || rc == SQLITE_OK) {
        reset();
        return 0;
    }
    return 1;
}
/**
 * @brief SqliteStmt::exec
 * @return
 */
bool SqliteStmt::exec() {
    int rc = sqlite3_step(stmt_);
    if(rc != SQLITE_OK && rc != SQLITE_DONE) {
        LOGG(Logger::ERROR) << "when stepping: "
                            << sqlite3_errmsg(conn_->getDB()) << Logger::FLUSH;
        return 0;
    }
    return 1;
}
/**
 * @brief SqliteStmt::version
 * @return
 */
string SqliteStmt::version() {
    return SQLITE_VERSION " " SQLITE_SOURCE_ID;
}
/**
 * @brief SqliteStmt::reset
 * @return
 */
bool SqliteStmt::reset() {
    offset_ = 1;
    if(sqlite3_reset(stmt_) != SQLITE_OK) {
        LOGG(Logger::ERROR) << "couldn't reset the statement: "
                            << sqlite3_errmsg(conn_->getDB()) << Logger::FLUSH;
        return false;
    }
    return true;
}
/**
 * @brief SqliteStmt::explain
 * @return
 */
string SqliteStmt::explain() {
    char *zExplain;
    sqlite3_stmt *pExplain;
    const char *zSql = query_.c_str();

    zSql = sqlite3_sql(stmt_);
    if(zSql == 0) {
        //return SQLITE_ERROR;
        return "";
    }

    zExplain = sqlite3_mprintf("EXPLAIN QUERY PLAN %s", zSql);
    if(zExplain == 0) {
        //return SQLITE_NOMEM;
        return "";
    }

    int rc = sqlite3_prepare_v2(sqlite3_db_handle(stmt_), zExplain, -1, &pExplain, 0);
    sqlite3_free(zExplain);
    if(rc != SQLITE_OK)
        return "";

    std::stringstream ss;
    while(sqlite3_step(pExplain) == SQLITE_ROW) {
        int iSelectid = sqlite3_column_int(pExplain, 0);
        int iOrder = sqlite3_column_int(pExplain, 1);
        int iFrom = sqlite3_column_int(pExplain, 2);
        const char *zDetail = (const char *)sqlite3_column_text(pExplain, 3);

        ss << iSelectid << " " << iOrder << " " << iFrom << " " << zDetail << "\n";
    }

    if(sqlite3_finalize(pExplain) != SQLITE_OK) {
        LOGG(Logger::ERROR) << "Couldn't finalize" << Logger::FLUSH;
    }
    return ss.str();
}
/**
 * @brief SqliteStmt::finalize
 * @return
 */
bool SqliteStmt::finalize() {
    if(!finalized_) {
        finalized_ = true;
        conn_->notifyDestruction(this);
        if(sqlite3_finalize(stmt_) != SQLITE_OK) {
            LOGG(Logger::ERROR) << "couldn't finalize the statement: "
                                << sqlite3_errmsg(conn_->getDB()) << Logger::FLUSH;
            return false;
        }
        stmt_ = 0;
    } else {
        LOGG(Logger::WARNING) << "Already finalized statement is finalized" << Logger::FLUSH;
    }
    return true;
}
//------------------------------------------------------------------------------
// Sqlite connection object
//------------------------------------------------------------------------------
/**
 * @brief SqliteConnection::SqliteConnection
 */
SqliteConn::SqliteConn()
    :  db_(NULL), dbName_(""), create_(false), inmemory_(false), activeTransaction_(false)  {;}
/**
 * @brief SqliteConn::~SqliteConn
 */
SqliteConn::~SqliteConn() {
    close();
}
/**
 * @brief init
 * @param dbname
 * @param createIfMissing
 * @return
 */
bool SqliteConn::open(const URL &url, const Properties &props) {
    // prepare connection
    if(!close())
        return false;

    dbName_ = url.getPath();
    create_ = lexical_cast<int32_t>(props.get("create"));
    inmemory_ = lexical_cast<int32_t>(props.get("memory"));
    readwrite_ = lexical_cast<int32_t>(props.get("readwrite"));

    int safe = sqlite3_threadsafe();
    if(safe == 0) {
        LOGG(Logger::WARNING) << "[SQLITE CONNECTION] Not threadsafe connection" << Logger::FLUSH;
    } else if(safe == 1) {
        LOGG(Logger::WARNING) << "[SQLITE CONNECTION] Serialized level (need reduce to thread safe level?)" << Logger::FLUSH;
    } else if(safe == 2) {
        LOGG(Logger::WARNING) << "[SQLITE CONNECTION] Thread safe" << Logger::FLUSH;
    } else {
        LOGG(Logger::ERROR) << "[SQLITE CONNECTION] Unknown level" << Logger::FLUSH;
    }

    if(dbName_ == "")
        return false;

    if(inmemory_) {
        sqlite3* filedb;
        int rc = SQLITE_OK;
        // create the tmp file database
        if(create_)
            rc = sqlite3_open(dbName_.c_str(), &filedb);
        else {
            if(readwrite_)
                rc = sqlite3_open_v2(dbName_.c_str(), &filedb, SQLITE_OPEN_READWRITE, NULL);
            else
                rc = sqlite3_open_v2(dbName_.c_str(), &filedb, SQLITE_OPEN_READONLY, NULL);
        }
        if(rc != SQLITE_OK) {
            LOGG(Logger::ERROR) << "Couldn't open file database" << sqlite3_errmsg(filedb) << Logger::FLUSH;
            return false;
        }

        // create the memory database
        if(create_)
            rc = sqlite3_open(":memory:", &db_);
        else
            rc = sqlite3_open_v2(":memory:", &db_, SQLITE_OPEN_READWRITE, NULL);
        if(rc != SQLITE_OK) {
            LOGG(Logger::ERROR) << "Couldn't open memory database" << sqlite3_errmsg(filedb) << Logger::FLUSH;
            return false;
        }

        if(rc == SQLITE_OK) {
            // http://www.sqlite.org/backup.html
            sqlite3_backup *backup = sqlite3_backup_init(db_, "main", filedb, "main");
            if(backup) {
                if(sqlite3_backup_step(backup, -1) != SQLITE_DONE) {
                    LOGG(Logger::ERROR) << "Couldn't exec backup" << Logger::FLUSH;
                }
                if(sqlite3_backup_finish(backup) != SQLITE_OK) {
                    LOGG(Logger::ERROR) << "Couldn't finish backup" << Logger::FLUSH;
                }
            }
            if(sqlite3_close(filedb) != SQLITE_OK) {
                LOGG(Logger::ERROR) << "Couldn't close database" << sqlite3_errmsg(filedb) << Logger::FLUSH;
            }
        }
    } else {
        int rc = SQLITE_OK;
        if(create_)
            rc = sqlite3_open(dbName_.c_str(), &db_);
        else {
            if(readwrite_)
                rc = sqlite3_open_v2(dbName_.c_str(), &db_, SQLITE_OPEN_READWRITE, NULL);
            else
                rc = sqlite3_open_v2(dbName_.c_str(), &db_, SQLITE_OPEN_READONLY, NULL);
        }

        if(rc != SQLITE_OK || db_ == 0) {
            LOGG(Logger::ERROR) << "can't open database: " << sqlite3_errmsg(db_)
                                << " " << dbName_ << Logger::FLUSH;
            if(sqlite3_close(db_) != SQLITE_OK) {
                LOGG(Logger::ERROR) << "can't close database: " << sqlite3_errmsg(db_)
                                    << " " << dbName_ << Logger::FLUSH;
            }
            return false;
        }
    }
    return true;
}
/**
 * @brief SqliteConnection::close
 * @return
 */
bool SqliteConn::close() {
    create_ = false;
    inmemory_ = false;
    // iterating on elements of container and deleting
    vector<SqliteStmt*> tmp(stmts_.begin(), stmts_.end());
    for(SqliteStmt *stmt : tmp) {
        // Sqlite statement deletes itself from the
        // set of active connections maintained by SqliteConn
        stmt->reset();
        stmt->finalize();
    }
    releaseMemory(stmts_);
    if(db_ != NULL) {
        if(activeTransaction_ && !commitTransaction())
            return false;
        if(sqlite3_close(db_) != SQLITE_OK) {
            LOGG(Logger::ERROR) << "can't close database: "
                                << sqlite3_errmsg(db_)<< " " << Logger::FLUSH;
            return false;
        }
        db_ = NULL;
    }
    return true;
}
/**
 * @brief prepare
 * @param query
 * @return
 */
bool SqliteConn::prepare(const string &query, unique_ptr<PrepStmt> &stmt) {
    lock_.lock();
    sqlite3_stmt *raw_stmt;
    if(sqlite3_prepare_v2(db_, query.c_str(), -1, &raw_stmt, NULL) != SQLITE_OK) {
        LOGG(Logger::ERROR) << "couldn't prepare statement: "
                            << query << " " << sqlite3_errmsg(db_) << Logger::FLUSH;
        if(sqlite3_finalize(raw_stmt) != SQLITE_OK) {
            LOGG(Logger::ERROR) << "couldn't finalize statement: "
                                 << query << " " << sqlite3_errmsg(db_) << query << Logger::FLUSH;
        }
        stmt.reset(0);
        lock_.unlock();
        return false;
    }

    SqliteStmt *new_stmt = new SqliteStmt(query, raw_stmt, this);
    stmts_.insert(new_stmt);
    stmt.reset(new_stmt);
    lock_.unlock();
    return true;
}
/**
 * @brief existsTable
 * @param table
 * @return
 */
bool SqliteConn::existsTable(const std::string &table) {
    SqlQuery query = SqlQuery::q().select({"name"}).from("sqlite_master").where("type='table' AND name=?");
    std::unique_ptr<PrepStmt> stmt;
    if(prepare(query.toString(), stmt)) {
        if(stmt->bind(table)) {
            if(stmt->step())
                return true;
            else
                return false;
        }
    }
    return false;
}
/**
 * @brief primaryKey
 */
bool SqliteConn::primaryKey(const std::string &table, std::string &pk) {
    if(!existsTable(table))
        return false;

    // check if the type of the index is fts4 or fts3, then the name of the column is docid
    {
        unique_ptr<PrepStmt> stmt;
        string type = "select sql FROM sqlite_master WHERE type='table' and name='"+table+"'";
        if(!prepare(type, stmt))
            return false;

        bool fts = false;
        if(stmt->step()) {
            string sql = stmt->column_text(0);
            if(sql.find("USING fts3") != string::npos || sql.find("USING fts4") != string::npos) {
                pk = "docid";
                fts = true;
            }
        } else {
            LOGG(Logger::ERROR) << "Error extracting primary key" << Logger::FLUSH;
            return false;
        }
        if(fts)
            return true;
    }

    unique_ptr<PrepStmt> stmt;
    string sql = "PRAGMA TABLE_INFO('"+table+"')";    

    if(prepare(sql, stmt)) {
        vector<string> cols;
        vector<string> nonPrimary;
        while(stmt->step()) {
            string col = stmt->column_text(1);
            int type = stmt->column_int(6);
            if(type > 0)
                cols.push_back(col);
            else
                nonPrimary.push_back(col);
        }
        if(cols.size() == 1) {
            pk = cols[0];
        } else if(cols.size() > 1) {
            string key = "("+cols[0];
            for(int i = 0; i < cols.size(); i++) {
                key.append(","+cols[i]);
            }
            key.append(")");
            pk = key;
        } else {
            if(nonPrimary.size() > 0)
                pk = nonPrimary[0];
            else {
                LOGG(Logger::ERROR) << "No columns found for table "+table << Logger::FLUSH;
                return false;
            }
        }
        return true;
    } else {
        LOGG(Logger::ERROR) << "Couldn't prepare statement: " << Logger::FLUSH;
        return false;
    }
}
/**
 * @brief SqliteConnection::exec
 * @param query
 * @return
 */
bool SqliteConn::exec(const std::string &query) {
    char *zErrMsg = NULL;
    int rc = sqlite3_exec(db_, query.c_str(), NULL, 0, &zErrMsg);
    if(rc != SQLITE_OK) {
        LOGG(Logger::ERROR) << zErrMsg << " query: " << query << Logger::FLUSH;
        sqlite3_free(zErrMsg);
        return false;
    }
    sqlite3_free(zErrMsg);
    return true;
}
/**
 * @brief exec
 * @param queries
 * @return
 */
bool SqliteConn::exec(const std::vector<std::string> &queries) {
    for(const string &str : queries)
        if(!exec(str))
            return false;
    return true;
}
/**
 * @brief SqliteStorage::beginTransaction
 */
bool SqliteConn::beginTransaction() {
    if(sqlite3_exec(db_, "BEGIN TRANSACTION", NULL, NULL, NULL) != SQLITE_OK) {
        LOGG(Logger::ERROR) << "can't begin transaction: "
                            << sqlite3_errmsg(db_)<< " " << Logger::FLUSH;
        return false;
    }
    activeTransaction_ = true;
    return true;
}
/**
 * @brief SqliteStorage::finishTransaction
 */
bool SqliteConn::commitTransaction() {
    if(activeTransaction_) {
        if(sqlite3_exec(db_, "COMMIT TRANSACTION", NULL, NULL, NULL) != SQLITE_OK) {
            LOGG(Logger::ERROR) << "can't commit transaction: "
                                << sqlite3_errmsg(db_)<< " " << Logger::FLUSH;
            return false;
        }
    } else {
        LOGG(Logger::WARNING) << "there was no active transaction: "
                              << sqlite3_errmsg(db_)<< " " << Logger::FLUSH;
    }
    activeTransaction_ = false;
    return true;
}
/**
 * @brief SqliteConn::notifyDestruction
 * @param stmt
 * @return
 */
bool SqliteConn::notifyDestruction(SqliteStmt* stmt) {
    if(stmts_.count(stmt) == 0) {
        LOGG(Logger::WARNING) << "non existing statement is being destructed" << Logger::FLUSH;
    } else {
        stmts_.erase(stmt);
    }
    return true;
}
/**
 * @brief SqliteConn::getDB
 * @return
 */
sqlite3* SqliteConn::getDB() {
    return db_;
}
