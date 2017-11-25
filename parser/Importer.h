#pragma once

#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/Utils/FileSystemUtil.h>

/**
 * @brief The SqliteBulkImporter imports osm and routing data to sqlite tables class
 */
class SqliteBulkImporter {
private:
    std::string sql_;
    std::string sqlFile_;
    std::string sqlite3Command_;
    std::string icuTestCmd_;
    std::string storageFile_;
    std::ofstream sqlStream_;
    std::vector<std::string> tableSQL_;
    std::vector<std::string> tables_;
    std::vector<std::string> gtfsTableSQL_;
    std::vector<std::string> gtfsObjFiles_;
    std::vector<std::string> gtfsGroupFiles_;
    std::vector<std::string> otherCmd_;
    std::vector<std::string> filesToRead_;
    std::string exportPath_;
public:
    SqliteBulkImporter(const std::string &workingDir);
    bool init(const std::string &storageFile, bool /*createIfMissing*/);
    bool import();
    void checkSqlite() const;
    void checkSqliteIcu() const;
    void tearDown();
    void initTables();
    /**
     * @brief SqliteBulkImporter::registerPlugin
     * @param plugin
     */
    template <typename P>
    void registerPlugin(const P *plugin) {
        std::vector<std::string> tSql = plugin->getSqlToCreateTables();
        tableSQL_.insert(tableSQL_.end(),tSql.begin(), tSql.end());
        std::vector<std::string> tableNames = plugin->getTableNamesToImport();
        tables_.insert(tables_.end(), tableNames.begin(),tableNames.end());
        std::vector<std::string> otherCmds = plugin->getOtherSqlCommands();
        otherCmd_.insert(otherCmd_.end(), otherCmds.begin(), otherCmds.end());
        std::vector<std::string> readFiles = plugin->getFileNamesToRead();
        filesToRead_.insert(filesToRead_.end(), readFiles.begin(), readFiles.end());
    }
};
