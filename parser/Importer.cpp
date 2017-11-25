#include "Importer.h"
#include <UrbanLabs/Sdk/Storage/SqlConsts.h>

using namespace std;
/**
 * @brief SqliteBulkImporter::SqliteBulkImporter
 */
SqliteBulkImporter::SqliteBulkImporter(const std::string &workingDir)
    : sql_(), sqlFile_(), sqlite3Command_(), icuTestCmd_(),storageFile_(),
      sqlStream_(), tableSQL_(), tables_(), gtfsTableSQL_(), otherCmd_()
{
    string fullPathToSqlite;
    if(!FsUtil::search("sqlite3", {workingDir}, fullPathToSqlite)) {
        LOGG(Logger::ERROR) << "Couldnt find sqlite3 binary" << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }
    // need to escape spaces
    sqlite3Command_ = "\"" + fullPathToSqlite + "\"";
    if(!FsUtil::extractDir(fullPathToSqlite, exportPath_)) {
        LOGG(Logger::ERROR) << "Couldnt determine sqlite3 directory" << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }
    // lib folder is level up from binaries
    exportPath_ = FsUtil::makePath({exportPath_, "..", "lib"});
    exportPath_ = "export LD_LIBRARY_PATH=\""+exportPath_ +"\"";
    icuTestCmd_ = "echo \"select 'ы' LIKE 'Ы';\" | " + sqlite3Command_;
    checkSqliteIcu();
    initTables();
}
/**
 * @brief SqliteBulkImporter::checkSqliteIcu
 */
void SqliteBulkImporter::checkSqliteIcu() const {
    string icuTest = getStdoutFromCommand(exportPath_ + " && " + icuTestCmd_);
    if (icuTest.find("1") == string::npos) {
        LOGG(Logger::ERROR) << "[SQLITE IMPORTER] ICU library is not included in sqlite3 command. Test command with path: " << exportPath_ << " returned: " << icuTest << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }
}
/**
 * @brief SqliteBulkImporter::init
 * @param storageFile
 * @param createIfMissing
 */
bool SqliteBulkImporter::init(const string &storageFile, bool /*createIfMissing*/) {
    string pt = StringConsts::PT;
    for(const string& pragma : SqlConsts::PRAGMAS)
        sql_ += pragma + "\n";
    // create all required tables
    for(const string& tableSql : tableSQL_)
        sql_ += tableSql+"\n";
    sql_ +=".separator '"+StringConsts::SEPARATOR+"'\n";
    // write import stmts
    for(const string& table : tables_)
        sql_ += ".import "+storageFile+pt+table+" "+table+"\n";
    for(const string& fileToRead : filesToRead_) {
        sql_ += ".read "+fileToRead + "\n";
    }
    // other commands(index etc)
    for(const string& other : otherCmd_) {
        sql_ += other+"\n";
    }
    sql_ += "ANALYZE;\nVACUUM;\nREINDEX;\n";

    storageFile_ = storageFile;
    sqlFile_ = storageFile+".sql";
    sqlStream_.open(sqlFile_);
    if(sqlStream_.good()) {
        sqlStream_ << sql_ << endl;
        sqlStream_.close();
    } else {
        LOGG(Logger::ERROR) << "[SQLITE IMPORTER] Error while creating sql script" << Logger::FLUSH;
        return false;
    }
    return true;
}
/**
 * @brief SqliteBulkImporter::import
 */
bool SqliteBulkImporter::import() {
    string command = exportPath_ + " && " + sqlite3Command_+" "+storageFile_+" < "+sqlFile_;
    int code = system(command.c_str());
    if(code != 0) {
        LOGG(Logger::ERROR) << "[SQLITE IMPORTER] Error while executing sql script" << Logger::FLUSH;
        return false;
    }
    return true;
}
/**
 * @brief SqliteBulkImporter::tearDown
 */
void SqliteBulkImporter::tearDown() {
    std::remove(sqlFile_.c_str());
}
/**
 * @brief SqliteBulkImporter::initTables
 */
void SqliteBulkImporter::initTables() {
    // put mapfeature stuff by default
    tableSQL_ = {};
    tables_ = {};
    otherCmd_ = {};
}
