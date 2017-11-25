#pragma once
#include <string>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

class ServerOption {
public:
	static const std::string PORT;
	static const std::string HELP;
    static const std::string VERSION;
    static const std::string DEBUG;
public:
    static const int ERROR_IN_COMMAND_LINE = 1; 
    static const int SUCCESS = 0; 
    static const int ERROR_UNHANDLED_EXCEPTION = 2;
public:
	ServerOption();
	bool init(int argc, char *argv[]);
	int validate();
    bool has(const std::string& argName);
    template<typename T>
    T get(const std::string &optionName) {
        return varMap_.count(optionName) ? varMap_[optionName].as<T>() : T();
    }
    std::string help(bool showDetails);
    static std::string version();
    std::string version_detailed();
private:
    po::options_description desc_;
    po::variables_map varMap_;
    po::positional_options_description portOption_;
};
