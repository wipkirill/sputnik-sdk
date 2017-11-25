#pragma once

#include <map>
#include <set>
#include <queue>
#include <mutex>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/system/system_error.hpp>
namespace fs = boost::filesystem; 
namespace sys = boost::system;

/**
 * @brief The FilePathCache class
 */
class FilePathCache {
private:
    std::mutex lock_;
    std::map<std::string, std::string> cache_;
public:
    bool get(const std::string &file, std::string &path);
    void put(const std::string &file, const std::string &path);
};
/**
 * @brief The FsUtil class
 */
class FsUtil {
public:
    // TODO: pass this with every request to search?
    static std::set<std::string> excludeFromSearch_;
public:
    static void reportError(const sys::system_error& ex);
    static std::string makePath(const std::vector<std::string>& entries);
    static bool currentWorkDir(std::string& workDir);
    static std::string addTrailingSlash(const std::string &workDir);
    static bool search(const std::string& fileName, const std::set<std::string> &searchDirs,
                       std::string &fullPathToFileName);
    static bool getFileContent(const std::string &str, std::string &content);
    static bool fileExists(const std::string &name);
    static bool isDir(const std::string &name);
    static bool writeToFileBinary(const std::string &name, const std::string &content);
    static bool listDirs(const std::set<std::string> &dirs, std::set<std::string> &files);
    static bool extractFileName(const std::string &path, std::string &fileName);
    static bool extractDir(const std::string &path, std::string &dirName, bool mustExist = true);
    static bool getDiskSpace(const std::string& path, int64_t &diskSize, int64_t &totalFreeBytes);
    static bool getFileSize(const std::string &path, int64_t &size);
};
/**
 * @brief The FsFile class
 */
class FsFile {
private:
    fs::path file;
public:
    bool isDir();
    std::string getName();
public:
    friend class FsDirectory;
};
/**
 * @brief The FsDirectory class
 */
class FsDirectory {
private:
    fs::path dir;
    fs::directory_iterator end_iter;
    fs::directory_iterator dir_itr;
public:
    FsDirectory();
    bool open(const std::string& dirName);
    bool readFile(FsFile& f);
    bool hasNext();
    void next();
};
