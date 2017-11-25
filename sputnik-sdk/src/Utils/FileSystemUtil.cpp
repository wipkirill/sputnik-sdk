#include <cerrno>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <UrbanLabs/Sdk/Utils/FileSystemUtil.h>
#include <UrbanLabs/Sdk/Utils/StringUtils.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>

using namespace std;

const string Folder::COMMON       = "common";
const string Folder::DATA         = "data";
const string Folder::FONTS        = "fonts";
const string Folder::PLUGIN_INPUT = "input";
const string Folder::TILES        = "tiles";
const string Folder::RESOURCE     = "res";
const string Folder::IMG          = "img";
const string FileName::INDEX_HTML = "index.html";
const string FileName::MENU_HTML  = "menu.html";
const string FileName::ERROR_HTML = "error.html";
const string FileName::MAPNIK_XML = "OSMBrightCustom.xml";
const string FileName::LABELS_XML = "labels.xml"; 

// TODO: fix this
set<string> FsUtil::excludeFromSearch_ = {Folder::COMMON, Folder::TILES, ".", ".."};

//---------------------------------------------------------------------------------
// FilePathCache
//---------------------------------------------------------------------------------
/**
 * @brief FilePathCache::get
 * @param file
 * @param path
 * @return
 */
bool FilePathCache::get(const string &file, string &path) {
    lock_.lock();
    bool ret = false;
    if(cache_.find(file) != cache_.end()) {
        path = cache_[file];
        if(FsUtil::fileExists(path))
            ret = true;
        else {
            cache_.erase(file);
            path = "";
        }
    }
    lock_.unlock();
    return ret;
}
/**
 * @brief FilePathCache::put
 * @param file
 * @param path
 */
void FilePathCache::put(const string &file, const string &path) {
    lock_.lock();
    cache_[file] = path;
    lock_.unlock();
}
//---------------------------------------------------------------------------------
// FsUtil
//---------------------------------------------------------------------------------
/**
 * @brief FsUtil::makePath
 * @param entries
 * @return
 */
string FsUtil::makePath(const vector<string> &entries) {
    if (entries.size() == 0)
        return "";
    string res = entries[0];
    char pathSep = fs::path::preferred_separator;
    for (int i=1; i < (int)entries.size();++i) {
        string entry = entries[i];
        // remove leading slash in path entry
        if (entry.find(pathSep) == 0)
            entry = entry.substr(1, entry.size()-1);
        // if last char in curr path is path separator
        if (res.find_last_of(pathSep) == res.size()-1)
            res += entry;
        else
            res += pathSep + entry;
    }
    return res;
}
/**
*
**/ 
void FsUtil::reportError(const sys::system_error& ex) {
    LOGG(Logger::ERROR)  << "threw filesystem_error exception:" <<
         "ex.code().value() is " << ex.code().value() << 
         "ex.code().category().name() is " << ex.code().category().name()  <<
         "ex.what() is " << ex.what()  << Logger::FLUSH;
}
/**
 * @brief FsUtil::currentWorkDir
 * @param workDir
 * @return
 */
bool FsUtil::currentWorkDir(string &workDir) {
    try {
        workDir = fs::canonical(fs::current_path()).string();
        return true;
    } catch (const sys::system_error& ex) {
        reportError(ex);
    }
    return false;
}
/**
 * @brief FsUtil::fixLastSlash
 * @param workDir
 * @return
 */
string FsUtil::addTrailingSlash(const string &workDir) {
    char pathSep = fs::path::preferred_separator;
    string res = workDir;
    if (res.find_last_of(pathSep) != res.size()-1)
        res += pathSep;
    return res;
}
/**
 * @brief FsUtil::search
 * @param fileName
 * @param excludeDirs
 * @return
 */
bool FsUtil::search(const string& fileName, const set<string> &searchDirs, string& fullPathToFileName) {
    queue<string> pathQueue;
    for (const string& searchDir: searchDirs)
        pathQueue.push(searchDir);
    while(!pathQueue.empty()) {
        string path = pathQueue.front();
        pathQueue.pop();
        FsDirectory dir;
        if (!dir.open(path)) {
            LOGG(Logger::ERROR) << "Cannot open dir " << path << Logger::FLUSH;
            return false;
        }
        LOGG(Logger::DEBUG) << "Searching in " << path << Logger::FLUSH;
        while (dir.hasNext()) {
            FsFile file;
            if (!dir.readFile(file)) {
                LOGG(Logger::ERROR) << "Cannot open file " << file.getName() << Logger::FLUSH;
                return false;
            }
            if (file.isDir() && excludeFromSearch_.find(file.getName()) == excludeFromSearch_.end()) {
                string dirName = makePath({path, file.getName()});
                pathQueue.push(dirName);
            } else {
                if (fileName == file.getName()) {
                    LOGG(Logger::DEBUG) << "Found " << fileName << " in " << path << Logger::FLUSH;
                    vector<string> foundPath = {path, file.getName()};
                    fullPathToFileName = makePath(foundPath);
                    return true;
                }
            }
            dir.next();
        }    
    }
    return false;
}
/**
 * @brief FsUtil::listDirs
 * @param dirs
 * @param files
 * @return
 */
bool FsUtil::listDirs(const set<string> &searchDirs, set<string> &files) {
    queue<string> pathQueue;
    for (const string& searchDir: searchDirs)
        pathQueue.push(searchDir);
    while (!pathQueue.empty()) {
        string path = pathQueue.front();
        pathQueue.pop();
        FsDirectory dir;
        if (!dir.open(path)) {
            LOGG(Logger::ERROR) << "Cannot open dir " << path << Logger::FLUSH;
            return false;
        }
        LOGG(Logger::DEBUG) << "Searching in " << path << Logger::FLUSH;
        while (dir.hasNext()) {
            FsFile file;
            if (!dir.readFile(file)) {
                LOGG(Logger::ERROR) << "Cannot open file " << file.getName() << Logger::FLUSH;
                return false;
            }
            if (file.isDir() && excludeFromSearch_.find(file.getName()) == excludeFromSearch_.end()) {
                string dirName = makePath({path, file.getName()});
                pathQueue.push(dirName);
            } else {
                if (excludeFromSearch_.find(file.getName()) == excludeFromSearch_.end()) {
                    vector<string> foundPath = {path, file.getName()};
                    files.insert(makePath(foundPath));
                }
            }
            dir.next();
        }
    }
    return true;
}
/**
 * @brief FsUtil::extractFileName
 * @param path
 * @param fileName
 * @return
 */
bool FsUtil::extractFileName(const string &path, string &fileName) {
    if(path.size() > 0) {
        try {
            fs::path p(path);
            if(fs::exists(p) && fs::is_regular_file(p)) {
                fileName = p.filename().string();
                return true;
            }
        } catch(const sys::system_error& ex) {
            reportError(ex);
        }
    }
    return false;
}
/**
 * @brief FsUtil::extractDir
 * Get parent path of a given path
 * @param path
 * @param dirName
 * @return
 */
bool FsUtil::extractDir(const std::string &path, std::string &dirName, bool mustExist) {
    if(path.size() > 0) {
        try {
            fs::path p(path);
            if(mustExist) {
                if(fs::exists(p)) {
                    dirName = p.parent_path().string();
                    return true;
                }
            } else {
                dirName = p.parent_path().string();
                if(dirName.empty())
                    return false;
                return true;
            }
        } catch(const sys::system_error& ex) {
            reportError(ex);
        }
    }
    return false;
}
/**
 * @brief getFileContent
 * @param content
 * @return
 */
bool FsUtil::getFileContent(const string &name, string &content) {
    ifstream t(name);
    if(t.is_open() && t.good()) {
        // read the file contents
        t.seekg(0, ios::end);
        size_t size = t.tellg();
        string buffer(size, ' ');
        t.seekg(0); t.read(&buffer[0], size);
        content.swap(buffer);
        return true;
    }
    return false;
}
/**
 * @brief fileExists
 * @param name
 * @return
 */
bool FsUtil::fileExists(const string &name) {
    try {
        fs::path p(name);
        return fs::exists(p);
    } catch(const sys::system_error& ex) {
        reportError(ex);
    }
    return false;
}
/**
 * @brief isDir
 * @param name
 * @return
 */
bool FsUtil::isDir(const std::string &name) {
    try {
        fs::path p(name);
        if(fs::exists(p) && fs::is_directory(p)) 
            return true;
    } catch(const sys::system_error& ex) {
        reportError(ex);
    }
    return false;
}
/**
 * @brief FsUtil::writeToFile
 * @param name
 * @param content
 * @return
 */
bool FsUtil::writeToFileBinary(const string &name, const string &content) {
    if(!fileExists(name)) {
        ofstream str(name, ios_base::out);
        if(str.is_open()) {
            size_t before = str.tellp();
            if(str.write(content.c_str(), content.size())) {
                size_t numberOfBytesWritten = (size_t)str.tellp() - before;
                if(numberOfBytesWritten != content.size()) {
                    LOGG(Logger::ERROR) << "[FsUtil]Cannot write to file " << name << Logger::FLUSH;
                    str.close();
                    return false;
                }
                str.flush();
                str.close();
                return true;
            } else {
                return false;
            }
        }
    }
    return false;
}
/**
 * @brief FsUtil::checkDiskSpace
 * @param path
 * @param diskSize
 * @param totalFreeBytes
 * @return
 */
bool FsUtil::getDiskSpace(const string& path, int64_t &diskSize, int64_t &totalFreeBytes) {
    try {
        fs::space_info s = fs::space(fs::path(path));
        totalFreeBytes = s.free;
        diskSize = s.available;
        return true;
    } catch(const sys::system_error& ex) {
        reportError(ex);
    }
    return false;
}
/**
 * @brief FsUtil::getFileSize
 * @param path
 * @param size
 * @return
 */
bool FsUtil::getFileSize(const string &path, int64_t &size) {
    try {
        size = fs::file_size(fs::path(path));
        return true;
    } catch(const sys::system_error& ex) {
        reportError(ex);
    }
    return false;
}
/**
 * @brief FsFile::getName
 * @return
 */
string FsFile::getName() {
    return file.filename().string();
}
/**
 * @brief FsFile::isDir
 * @return
 */
bool FsFile::isDir() {
    return fs::is_directory(file);
}
/**
*
*/
FsDirectory::FsDirectory() {
    ;
}
/**
 * @brief FsDirectory::open
 * @param dirName
 * @return
 */
bool FsDirectory::open(const string &dirName) {
    try {
        dir = fs::path(dirName);
        if(fs::exists(dir) && fs::is_directory(dir)) {
            dir_itr = fs::directory_iterator(dir);
            return true;    
        }
    } catch(const sys::system_error& ex) {
        LOGG(Logger::ERROR)  << ex.what() << Logger::FLUSH;
    }
    return false;
}
/**
 * @brief FsDirectory::hasNext
 * @return
 */
bool FsDirectory::hasNext() {
    return dir_itr != end_iter;
}
/**
 * @brief FsDirectory::readFile
 * @param file
 * @return
 */
bool FsDirectory::readFile(FsFile &f) {
    try {
        f.file = fs::path(dir_itr->path());
        if(fs::exists(f.file)) {
            return true;
        }
    } catch(const sys::system_error& ex) {
         LOGG(Logger::ERROR)  << ex.what() << Logger::FLUSH;
    }
    return false;
}
/**
 * @brief FsDirectory::next
 */
void FsDirectory::next() {
    ++dir_itr;
}
