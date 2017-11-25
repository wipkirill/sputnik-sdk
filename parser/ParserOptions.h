#pragma once 

#include <string>
#include <UrbanLabs/Sdk/Utils/Types.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

/**
 * @brief The ParserOption class
 */
class ParserOption {
public:
    static const std::string AREA_NAME;
    static const std::string COUNTRY_CODES;
    static const std::string COMMENT;
    static const std::string PEDESTRIAN;
    static const std::string HIGHWAY;
    static const std::string TAGS;
    static const std::string ROUTING;
    static const std::string SEARCH;
    static const std::string GTFS;
    static const std::string TILES;
    static const std::string TURN_RESTRICTIONS;
    static const std::string ADDRESS_DECODER;
    static const std::string NO_DATA_COMPRESSION;
    static const std::string DEBUG;
    static const std::string HELP;
    static const std::string VERSION;
    static const std::string INPUT;
    static const std::string OUTPUT;
public:
    static const int SUCCESS = 0;
    static const int ERROR_IN_COMMAND_LINE = 1;  
    static const int ERROR_NOTHING_TO_DO = 2;
    static const int ERROR_UNHANDLED_EXCEPTION = 3;
public:
    ParserOption();
    bool init(int argc, char *argv[]);
    int validate();
    bool has(const std::string& argName);
    template<typename T>
    T get(const std::string &optionName) {
        return varMap_.count(optionName) ? varMap_[optionName].as<T>() : T();
    }
    static std::string version();
    std::string help(bool showDetails);
private:
    po::options_description desc_;
    po::variables_map varMap_;
    po::positional_options_description ioFileOption_;
private:
    void conflicting_options(const std::string& opt1, const std::string& opt2);
};
 