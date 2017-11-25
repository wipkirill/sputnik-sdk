#include <sstream>

#include <boost/foreach.hpp>

#include <UrbanLabs/Sdk/Utils/LicenseManager.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Utils/SslTool.h>
#include <UrbanLabs/Sdk/Utils/StringUtils.h>
#include <UrbanLabs/Sdk/Utils/FileSystemUtil.h>

using namespace std;

bool LicenseManager::load(const std::string& licenseKey, const std::string &licenseSign) {
	try {
        if(!validateData(licenseKey, licenseSign))
        	return false;
        string data = StringUtils::base64_decode(licenseKey);
        if(!parse(data)) 
        	return false;
    } catch (std::runtime_error& e) {
        LOGG(Logger::ERROR) << e.what() << Logger::FLUSH;
        return false;
    }
    return true;
}

bool LicenseManager::load(const std::string& path) {
	if(!FsUtil::fileExists(path)) {
		LOGG(Logger::ERROR) << "License is not valid: file does not exists" << path << Logger::FLUSH;
		return false;
	}
	string fileContent;
	if(!FsUtil::getFileContent(path, fileContent)) {
		LOGG(Logger::ERROR) << "License is not valid: failed to read file" << path << Logger::FLUSH;
		return false;
	}

	stringstream ss;
	ss << fileContent;
	pt::ptree json;
    try {
        pt::read_json(ss, json);
        string data = json.get<string>("data");
        string signature = json.get<string>("signature");
        if(!load(data, signature))
        	return false;
    }
    catch (pt::json_parser_error &e) {
        LOGG(Logger::ERROR) << e.what() << Logger::FLUSH;
        return false;
    } catch (std::runtime_error& e) {
        LOGG(Logger::ERROR) << e.what() << Logger::FLUSH;
        return false;
    }
    return true;
}

bool LicenseManager::parse(const std::string& licenseData) {
	stringstream ss;
	ss << licenseData;
	LOGG(Logger::DEBUG) << licenseData << Logger::FLUSH;
    try {
        pt::read_json(ss, licenseProps_);
    } catch (pt::json_parser_error &e) {
        LOGG(Logger::ERROR) << e.what() << Logger::FLUSH;
        return false;
    } catch (std::runtime_error& e) {
        LOGG(Logger::ERROR) << e.what() << Logger::FLUSH;
        return false;
    }
	return true;
}

bool LicenseManager::getInt(const std::string& key, int& value) {
	try {
		value = licenseProps_.get<int>(key);
	} catch (pt::json_parser_error &e) {
		LOGG(Logger::ERROR) << e.what() << Logger::FLUSH;
        return false;
	} catch(const std::exception &e) {
		LOGG(Logger::ERROR) << e.what() << Logger::FLUSH;
        return false;
	}
	return true;
}

bool LicenseManager::getString(const std::string& key, std::string& value) {
	try {
		value = licenseProps_.get<std::string>(key);
	} catch (pt::json_parser_error &e) {
		LOGG(Logger::ERROR) << e.what() << Logger::FLUSH;
        return false;
	} catch(const std::exception &e) {
		LOGG(Logger::ERROR) << e.what() << Logger::FLUSH;
        return false;
	}
	return true;
}

bool LicenseManager::getStringSet(const std::string& key, std::set<std::string>& value) {
	try {
		BOOST_FOREACH(pt::ptree::value_type &v, licenseProps_.get_child(key)) {
	        // The data function is used to access the data stored in a node.
	        value.insert(v.second.data());
	    }	
	} catch (pt::json_parser_error &e) {
		LOGG(Logger::ERROR) << e.what() << Logger::FLUSH;
        return false;
	} catch(const std::exception &e) {
		LOGG(Logger::ERROR) << e.what() << Logger::FLUSH;
        return false;
	}
	
	return true;
}

bool LicenseManager::validateData(const string& data, const string& signature) {
	SslTool sTool;
	sTool.publicKey(SslTool::PUBLIC_KEY);

	string decodedData = StringUtils::base64_decode(data);
	string decodedSignature = StringUtils::base64_decode(signature);

	if(!sTool.verify(decodedData, decodedSignature)) {
		LOGG(Logger::ERROR) << "License is not valid: data does not match signature" << Logger::FLUSH;
		return false;
	}

	return true;
}

bool LicenseManager::expired(const DateTime& dueDate) const {
	DateTime current(currentDateTime());
	return dueDate < current;
}