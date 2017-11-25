#include <WebService/ServerVersion.h>
#include <WebService/ServerOption.h>
#include <UrbanLabs/Sdk/Platform/LibSputnikVersion.h>

using namespace std;

const string ServerOption::PORT    = "port";
const string ServerOption::HELP    = "help";
const string ServerOption::VERSION = "version";
const string ServerOption::DEBUG   = "debug";


ServerOption::ServerOption() {
    // main option configuration
    desc_.add_options()
        (PORT.c_str(), po::value<int>()->required(), "bind server to PORT")
        (HELP.c_str(), po::bool_switch()->default_value(false), "print this message")
        (DEBUG.c_str(), po::bool_switch()->default_value(false), "increase logging level")
        (VERSION.c_str(), po::bool_switch()->default_value(false), "Server version"); 

    portOption_.add(PORT.c_str(), 1);
}
/**
*
*/
bool ServerOption::init(int argc, char *argv[]) {
    try {
        po::store(po::command_line_parser(argc, argv).
            options(desc_).positional(portOption_).run(), varMap_);
    } catch(std::exception& e) {  
        return false; 
    } 
    return true;
}

/**
 * @brief ServerOption::validateOptions
 * @param argc
 * @param argv
 * @param invalid
 * @return
 */
int ServerOption::validate() {
    try {
        po::notify(varMap_);
    } catch(po::required_option& e) { 
        return ERROR_IN_COMMAND_LINE; 
    } catch(boost::program_options::error& e) { 
        return ERROR_IN_COMMAND_LINE; 
    } catch(std::exception& e) {  
        return ERROR_UNHANDLED_EXCEPTION; 
    } 
    return SUCCESS;
}
/**
 * @brief has
 * @param argValue
 * @param argc
 * @param argv
 * @return
 */
bool ServerOption::has(const string& argName) {
    return varMap_.count(argName) ? varMap_[argName].as<bool>() : false;
}

/**
 *
 */
string ServerOption::version() {
    return server_version();
}
/**
 *
 */
string ServerOption::help(bool showDetails = false) {
    stringstream out;
    out << "Usage: server PORT\n"  
      "The Server will be listening to on the PORT.\n";
    if(showDetails) {
        out << desc_;
    }
    return out.str();
}
/**
 *
 */
string ServerOption::version_detailed() {
    return "Server by UrbanLabs, version " + server_version() + ", source id " + server_source_id() + "\n" 
           "libsputnik version " + libsputnik_version()+", source id " + libsputnik_source_id();
}