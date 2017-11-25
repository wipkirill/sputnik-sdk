#pragma once

#include <string>
#include <set>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <UrbanLabs/Sdk/Utils/Timer.h>

namespace pt = boost::property_tree;

class LicenseManager {
private:
	pt::ptree licenseProps_;
public:
	bool load(const std::string& licenseKey, const std::string &licenseSign);
	bool load(const std::string& path);
	bool getInt(const std::string& key, int& value);
	bool getString(const std::string& key, std::string& value);
	bool getStringSet(const std::string& key, std::set<std::string>& value);
	bool expired(const DateTime& dueDate) const;
private:
	bool parse(const std::string& licenseData);
	bool validateData(const std::string& data, const std::string& signature);
};