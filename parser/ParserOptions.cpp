#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <iostream>

#include "ParserOptions.h"
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Platform/LibSputnikVersion.h>
#include "ParserVersion.h"

using namespace std;

const string ParserOption::AREA_NAME           = "area-name";
const string ParserOption::COUNTRY_CODES       = "country-codes";
const string ParserOption::COMMENT             = "comment";
const string ParserOption::TAGS                = "tags";
const string ParserOption::ROUTING             = "routing";
const string ParserOption::SEARCH              = "search";
const string ParserOption::GTFS                = "gtfs";
const string ParserOption::TILES               = "tiles";
const string ParserOption::TURN_RESTRICTIONS   = "turn-restrictions";
const string ParserOption::ADDRESS_DECODER     = "address-decoder";
// use this option to disable data compression in output file
const string ParserOption::NO_DATA_COMPRESSION = "nocompression";
// tag filter
const string ParserOption::PEDESTRIAN          = "pedestrian";
const string ParserOption::HIGHWAY             = "highway";
// turns on debug information
const string ParserOption::DEBUG               = "debug";
const string ParserOption::HELP                = "help";
const string ParserOption::VERSION             = "version";
const string ParserOption::INPUT               = "input";
const string ParserOption::OUTPUT              = "output";

  
ParserOption::ParserOption(): desc_("Allowed options") {
    // main option configuration
    desc_.add_options()
        (INPUT.c_str(), po::value<string>()->required(), "INPUT is the name of an OpenStreetMap(c) PBF file")
        (OUTPUT.c_str(), po::value<string>()->required(), "A new SQLite database is created in OUTPUT")
        
        (AREA_NAME.c_str(), po::value<string>(), "human-friendly and recognizable name of area represented by this map file, i.e Prague, Berlin etc")
        (COUNTRY_CODES.c_str(), po::value<string>(), "international country code(s) of the INPUT_FILENAME region separated by commas, i.e  RU,BY or DE,FR")
        (COMMENT.c_str(), po::value<string>(), "put here any machine or human readable notes to be included in map meta data")
        
        (ROUTING.c_str(), po::bool_switch()->default_value(false), "write routing information, without turn restrictions")
        (PEDESTRIAN.c_str(), po::bool_switch()->default_value(false), "pedestrian edge filter (only pedestrian edges to be parsed)")
        (HIGHWAY.c_str(), po::bool_switch()->default_value(false), "highway edge filter (only highway edges to be parsed)")

        (SEARCH.c_str(), po::bool_switch()->default_value(false), "export information needed for searching osm objects")
        
        (TURN_RESTRICTIONS.c_str(), po::bool_switch()->default_value(false), "this option is only valid with routing enabled. it forces to create edges that obey turn restrictions")
        (TILES.c_str(), po::bool_switch()->default_value(false), "dump information needed for rendering map tiles")
        (ADDRESS_DECODER.c_str(), po::bool_switch()->default_value(false), "preprocess addresses of object in order to give approximate addresses and autocompletion when giving partial addresses")
        (GTFS.c_str(), po::bool_switch()->default_value(false), "incubating public transport information/routing")
        (DEBUG.c_str(), po::bool_switch()->default_value(false), "increase logging level")
        (VERSION.c_str(), po::bool_switch()->default_value(false), "parser version")
        (HELP.c_str(), po::bool_switch()->default_value(false), "print this message");

    // positional args for input and output files
    ioFileOption_.add(INPUT.c_str(), 1);
    ioFileOption_.add(OUTPUT.c_str(), 1);        
}

/**
*
*/
bool ParserOption::init(int argc, char *argv[]) {
    try {
        po::store(po::command_line_parser(argc, argv).
            options(desc_).positional(ioFileOption_).run(), varMap_);
    } catch(std::exception& e) {  
        return false; 
    } 
    return true;
}
/**
 * @brief ParserOption::validateOptions
 * @param argc
 * @param argv
 * @param invalid
 * @return
 */
int ParserOption::validate() {
    try {
        po::notify(varMap_);
        // if user provided just input and output, without plugins
        // there is nothing to do
        std::vector<string> parsingOptions = {ROUTING, SEARCH, TILES, ADDRESS_DECODER};
        bool nothingToDo = true;
        for(const string pOpt:parsingOptions) 
            if(varMap_.count(pOpt) && !varMap_[pOpt].defaulted())
                nothingToDo = false;
        if(nothingToDo) 
            return ERROR_NOTHING_TO_DO;

        conflicting_options(HIGHWAY, PEDESTRIAN);
        
    } catch(po::required_option& e) { 
        return ERROR_IN_COMMAND_LINE; 
    } catch(boost::program_options::error& e) {  
        return ERROR_IN_COMMAND_LINE; 
    } catch(std::exception& e) {  
        std::cout << e.what() << std::endl;
        return ERROR_UNHANDLED_EXCEPTION; 
    }
    return SUCCESS;
}
/**
 * @brief findArg
 * @param argValue
 * @param argc
 * @param argv
 * @return
 */
bool ParserOption::has(const string& argName) {
    return varMap_.count(argName) ? varMap_[argName].as<bool>() : false;
}

/**
 *
 */
string ParserOption::version() {
    return "Parser by UrbanLabs, version " + parser_version() + ", source id " + parser_source_id() + "\n" 
           "libsputnik version " + libsputnik_version()+", source id " + libsputnik_source_id();
}
/**
 *
 */
string ParserOption::help(bool showDetails = false) {
    stringstream out;
    out << "Usage: parser INPUT_FILENAME OUTPUT_FILENAME [OPTIONS]\n"  
      "INPUT_FILENAME is the name of an OpenStreetMap(c) PBF file. A new SQLite database is created\n"
      "in OUTPUT_FILENAME. Use --help for details\n";
    if(showDetails) {
        out << desc_ ;
    }
    return out.str();
}

void ParserOption::conflicting_options(const string& opt1, const string& opt2) {
    if (varMap_.count(opt1) && !varMap_[opt1].defaulted() && 
            varMap_.count(opt2) && !varMap_[opt2].defaulted())
        throw logic_error(string("Conflicting options '") 
                          + opt1 + "' and '" + opt2 + "'.");
}