#include <iostream>
#include <vector>
#include <boost/filesystem.hpp>

#include "test_filesystem.h"
#include <UrbanLabs/Sdk/Utils/FileSystemUtil.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>

using namespace std;
namespace fs = boost::filesystem; 

void TestFilesystem::test() {
	INIT_LOGGING(Logger::INFO);
	// make path test
	char pathSep = fs::path::preferred_separator;
	vector<string> pEntries = {"common", "lib"};
	QVERIFY(FsUtil::makePath(pEntries) == "common" + string(1, pathSep) + "lib");
	pEntries = {"common/", "/lib"};
	QVERIFY(FsUtil::makePath(pEntries) == "common" + string(1, pathSep) + "lib");
	QVERIFY(FsUtil::makePath({}) == "");

	// current workdir
	string wd;
	FsUtil::currentWorkDir(wd);
	LOGG(Logger::INFO) << wd << Logger::FLUSH;
	QVERIFY(wd.find("urbanlabs-projects") != string::npos);

	// extract filename
	string fName, path;
	pEntries = {wd, "Test.pro"};
	path = FsUtil::makePath(pEntries);
	bool res = FsUtil::extractFileName(path, fName);
	QVERIFY(res == true);
	QVERIFY(fName == "Test.pro");

	// non existing file
	res = FsUtil::extractFileName("/home/test/test.txt", fName);
	QVERIFY(res == false);

	// extract parent dir
	string parentDirName;
	path = FsUtil::makePath(pEntries);
	res = FsUtil::extractDir(path, parentDirName);
	QVERIFY(res == true);
	QVERIFY(wd == parentDirName);

	// file exists
	path = FsUtil::makePath(pEntries);
	res = FsUtil::fileExists(path);
	QVERIFY(res == true);

	string nonExistent = "Non-existent";
	res = FsUtil::fileExists(nonExistent);
	QVERIFY(res == false);

	// get disk space
	int64_t diskSize = 0, totalFreeBytes = 0;
	res = FsUtil::getDiskSpace(wd, diskSize, totalFreeBytes);
	QVERIFY(res == true);
	QVERIFY(diskSize > 0);
	QVERIFY(totalFreeBytes > 0);

	// file size
	path = FsUtil::makePath(pEntries);
	int64_t fileSize = 0;
	res = FsUtil::getFileSize(path, fileSize);
	QVERIFY(res == true);
	QVERIFY(fileSize > 0);

	// list dir
	set<string> searchDirs = {wd}, files;
	res = FsUtil::listDirs(searchDirs, files);
	QVERIFY(res == true);
	QVERIFY(files.size() > 0);

	// search
	string fullPath, fileToFind = "Test.pro";
	res = FsUtil::search(fileToFind, searchDirs, fullPath);
	QVERIFY(res == true);
	QVERIFY(fullPath.find("Test.pro") != string::npos);

}