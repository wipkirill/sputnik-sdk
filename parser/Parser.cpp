#include <memory>
#include <stdlib.h>

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include "OsmTypes.h"
#include "ParserPlugin.h"
#include "ParserOptions.h"
#include "EndPointPlugin.h"
#include "EdgeResolvePlugin.h"
#include "PassHandler.h"
#include "EdgeFilter.h"
#include "RoutingPlugin.h"
#include "SearchPlugin.h"
#include "TagPlugin.h"
#include "PublicTransPlugin.h"
#include "MapInfoPlugin.h"
#include "TilePlugin.h"
#include "AddressDecoderPlugin.h"
#include "OsmInput.h"
#include "Importer.h"

#include <UrbanLabs/Sdk/Utils/Timer.h>
#include <UrbanLabs/Sdk/Utils/StringUtils.h>
#include <UrbanLabs/Sdk/Utils/FileSystemUtil.h>
#include <UrbanLabs/Sdk/Utils/LicenseManager.h>
#include <UrbanLabs/Sdk/Config/XmlConfig.h>
#include <UrbanLabs/Sdk/OSM/TagFilter.h>

using std::fstream;
using std::string;
using std::vector;
using std::ifstream;

int main(int argc, char *argv[]) { 
    ParserOption pOptions;
    // init parser options
    if(!pOptions.init(argc, argv)) {
        std::cout << pOptions.help(true) << std::endl;
        exit(EXIT_FAILURE);
    }

    // setup the logger level TODO avoid crush if INIT_LOGGING not called!
    if(pOptions.has(ParserOption::DEBUG)) {
        INIT_LOGGING(Logger::INFO);
    } else {
        INIT_LOGGING(Logger::PROGRESS);        
    }

    // determine parser binary workdir to configure importer and read license
    char* sdkHome = getenv("SPUTNIK_SDK_HOME");
    string parserBinaryworkdir;
    if (sdkHome != NULL) {
        parserBinaryworkdir = string(sdkHome);
    } else {
        if(!FsUtil::currentWorkDir(parserBinaryworkdir)) {
            LOGG(Logger::ERROR) << "Cannot determine current workdir" << Logger::FLUSH;
            exit(EXIT_FAILURE);
        }
    }

    LicenseManager lMgr;
    vector<string> licPathEntries = {parserBinaryworkdir, "config", "license.key"};
    string licPath = FsUtil::makePath(licPathEntries);
    if(!lMgr.load(licPath)) {
        exit(EXIT_FAILURE);
    }

    // print help 
    if(pOptions.has(ParserOption::HELP)) {
        std::cout << pOptions.help(true) << std::endl;
        exit(EXIT_SUCCESS);
    }
    // print version
    if(pOptions.has(ParserOption::VERSION)) {
        std::cout << ParserOption::version() << std::endl;
        exit(EXIT_SUCCESS);
    }
    // validate options
    int parseOptionResult = pOptions.validate();
    switch(parseOptionResult) {
        case ParserOption::ERROR_NOTHING_TO_DO:
            std::cout << "No parsing options provided, nothing to do" << std::endl;
            std::cout << pOptions.help(true) << std::endl;
            exit(EXIT_FAILURE);
            break;
        case ParserOption::ERROR_IN_COMMAND_LINE:
            std::cout << "An error encountered in command line arguments" << std::endl;
            std::cout << pOptions.help(true) << std::endl;
            exit(EXIT_FAILURE);
            break;
        case ParserOption::ERROR_UNHANDLED_EXCEPTION:
            std::cout << "An exception thown" << std::endl;
            std::cout << pOptions.help(true) << std::endl;
            exit(EXIT_FAILURE);
            break;
        default:
            break;
    }

    string expirationDateString;
    if(!lMgr.getString("products.parser.expirationDate", expirationDateString)) {
        LOGG(Logger::ERROR) <<  "Failed to read license data" << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }
    DateTime expirationDate(expirationDateString);

    if(!expirationDate.isValid() || lMgr.expired(expirationDate)) {
        LOGG(Logger::ERROR) <<  "Demo period expired. Please extend your license on the website http://urbanlabs.in" << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }

    int maxFileSizeMB = 0;
    if(!lMgr.getInt("products.parser.maxInputFileSize", maxFileSizeMB)) {
        LOGG(Logger::ERROR) <<  "Failed to load data from settings" << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }

    // check if input is valid
    string inputFile = pOptions.get<string>(ParserOption::INPUT);
    if(!FsUtil::fileExists(inputFile)) {
        // the inputFile file does not exist
        LOGG(Logger::ERROR) << "Input file" << inputFile << "does not exist" << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }

    int64_t inFileSize;
    if(!FsUtil::getFileSize(inputFile, inFileSize)) {
        LOGG(Logger::ERROR) << "Failed to read size of" << inputFile << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }

    // if(inFileSize / (1024 * 1024) > maxFileSizeMB) {
    //     LOGG(Logger::ERROR) << "Cannot parse file which size exceeds" << maxFileSizeMB << "MB in demo mode" << Logger::FLUSH;
    //     exit(EXIT_FAILURE);
    // }

    // check if output is valid
    string outputFile = pOptions.get<string>(ParserOption::OUTPUT);
    if(FsUtil::fileExists(outputFile)) {
        // the output file file exists
        LOGG(Logger::ERROR) << "Output file" << outputFile << "already exists" << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }

    // determine current working directory for temp files and output
    // try to extract a dir from output file path
    string workdir;
    if(FsUtil::extractDir(outputFile, workdir, false)) {
        LOGG(Logger::DEBUG) << "[PARSER] Determined workdir from output path" << workdir << Logger::FLUSH;
        if(!FsUtil::fileExists(workdir) || !FsUtil::isDir(workdir)) {
            LOGG(Logger::ERROR) << "Output folder" << workdir << "does not exists" << Logger::FLUSH;
            exit(EXIT_FAILURE);
        }
    } else if(!FsUtil::currentWorkDir(workdir)) {
        LOGG(Logger::ERROR) << "Cannot determine current workdir" << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }

    Plugin::setWorkDir(workdir);

    // setup tag filter for edges
    std::unique_ptr<EdgeFilter> edgeFilter(new HighWayFilter());
    std::unique_ptr<EdgeFilter> edgeFilterEndpoints(new HighWayFilter());

    if(pOptions.has(ParserOption::PEDESTRIAN)) {
        edgeFilter.reset(new PedestrianFilter());
        edgeFilterEndpoints.reset(new PedestrianFilter());
    }

    // find tile configuration file
    string tileConfigFile = FsUtil::makePath({parserBinaryworkdir, "config", "tileConfig.xml"});
    if(!FsUtil::fileExists(tileConfigFile)) {
        LOGG(Logger::ERROR) << "No tileconfig file in config/ folder" << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }
    // find tag config file
    string tagConfigFile = FsUtil::makePath({parserBinaryworkdir, "config", "tagConfig.xml"});
    if(!FsUtil::fileExists(tagConfigFile)) {
        LOGG(Logger::ERROR) << "No tagconfig file in config/ folder" << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }

    if(!TagFilter::readConfig(tagConfigFile)) {
        LOGG(Logger::ERROR) << "Failed to read tagconfig file" << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }

    // start timer
    Timer timer;
    // TODO move this to avoid file reading when no option specified
    // process end points
    EndPointPlugin eh(outputFile, edgeFilterEndpoints.get());
    OsmInput::read(inputFile, eh);

    // resolve ways
    EdgeResolvePlugin erh(outputFile, edgeFilterEndpoints.get());
    erh.setBadEdges(eh.getBadEdgeSet());
    erh.setEndPoints(eh.getEndPoints());
    erh.setVertices(eh.getVertices());
    OsmInput::read(inputFile, erh);

    bool compression = !pOptions.has(ParserOption::NO_DATA_COMPRESSION);
    if(!compression)
        LOGG(Logger::DEBUG) << "No compression will be applied to the output!" << Logger::FLUSH;

    set<string> mapFeatures;
    SqliteBulkImporter importer(parserBinaryworkdir);
    PassHandler ph(outputFile);
    // initialize plugins
    // register routing plugin
    if (pOptions.has(ParserOption::ROUTING)) {
        std::unique_ptr<RoutingPlugin> rp(new RoutingPlugin(outputFile, edgeFilter.get(), compression));
        if(pOptions.has(ParserOption::TURN_RESTRICTIONS)) {
            // register turn restrictions
            mapFeatures.insert(ParserOption::TURN_RESTRICTIONS);
            rp->setTurnRestrictions(eh.getTurnRestrictions());
        }
        rp->setOsmValidator(&eh);
        importer.registerPlugin(rp.get());
        ph.registerPlugin(std::move(rp));
        mapFeatures.insert(ParserOption::ROUTING);
    }
    // register search plugin
    if (pOptions.has(ParserOption::SEARCH)) {
        std::unique_ptr<SearchPlugin> sp(new SearchPlugin(outputFile));
        sp->setOsmValidator(&eh);
        importer.registerPlugin(sp.get());
        ph.registerPlugin(std::move(sp));
        mapFeatures.insert(ParserOption::SEARCH);

        // tags
        {
            std::unique_ptr<TagPlugin> tagPlugin(new TagPlugin(outputFile));
            tagPlugin->setOsmValidator(&eh);
            importer.registerPlugin(tagPlugin.get());
            ph.registerPlugin(std::move(tagPlugin));
            mapFeatures.insert(ParserOption::TAGS);
        }
    }
    // register gtfs plugin
    if (pOptions.has(ParserOption::GTFS)) {
        std::unique_ptr<PublicTransPlugin> pt(new PublicTransPlugin(outputFile));
        pt->setOsmValidator(&eh);
        importer.registerPlugin(pt.get());
        ph.registerPlugin(std::move(pt));
        mapFeatures.insert(ParserOption::GTFS);
    }
    // register tiles plugin
    if (pOptions.has(ParserOption::TILES)) {
        std::unique_ptr<TilePlugin> tilePlugin(new TilePlugin(outputFile, tileConfigFile, compression));
        tilePlugin->setOsmValidator(&eh);
        tilePlugin->setWorldWaterPath(FsUtil::makePath({parserBinaryworkdir, "config", "data", "world_water.sqlite"}));
        importer.registerPlugin(tilePlugin.get());
        ph.registerPlugin(std::move(tilePlugin));
        mapFeatures.insert(ParserOption::TILES);
    }
    // register address decoder plugin
    if (pOptions.has(ParserOption::ADDRESS_DECODER)) {
        std::unique_ptr<AddressDecoderPlugin> addrPlugin(new AddressDecoderPlugin(outputFile));
        addrPlugin->setOsmValidator(&eh);
        importer.registerPlugin(addrPlugin.get());
        ph.registerPlugin(std::move(addrPlugin));
        mapFeatures.insert(ParserOption::ADDRESS_DECODER);
    }

    std::unique_ptr<MapInfoPlugin> mapInf(new MapInfoPlugin(outputFile, mapFeatures));
    // extract areaname
    string areaName = pOptions.get<string>(ParserOption::AREA_NAME);
    mapInf->setAreaName(areaName);
    // extract country codes
    string countryCodes = pOptions.get<string>(ParserOption::COUNTRY_CODES);
    mapInf->setCountryCodes(countryCodes);
    // extract comment
    string comment = pOptions.get<string>(ParserOption::COMMENT);
    mapInf->setComment(comment);
    importer.registerPlugin(mapInf.get());
    ph.registerPlugin(std::move(mapInf));

    // check if some plugins need more than one pass
    int maxPassNum = ph.passNum();
    LOGG(Logger::PROGRESS) << "[PARSER] Start of parsing step" << Logger::FLUSH;
    
    for(int i=0;i<maxPassNum;++i) {
        ph.notifyPassNumber(i+1);
        OsmInput::read(inputFile, ph);
    }
    LOGG(Logger::PROGRESS) << "[PARSER] End of parsing step" << Logger::FLUSH;
    ph.notifyEndParsing();
    //bulk load all temp data from txt files to sqlite
    if(!importer.init(outputFile, true)){
        LOGG(Logger::ERROR) <<"Can't initialize sqlite importer"<< Logger::FLUSH;
        exit(EXIT_FAILURE);
    }
    LOGG(Logger::PROGRESS) << "[PARSER] Importing step" << Logger::FLUSH;
    if(!importer.import()) {
        LOGG(Logger::ERROR) <<"Can't import tables"<< Logger::FLUSH;
        exit(EXIT_FAILURE);
    }
    importer.tearDown();

    ph.afterImport();

    timer.stop();
    LOGG(Logger::PROGRESS) << "[TOTAL PARSE TIME]" << timer.getElapsedTimeSec() << "sec" << Logger::FLUSH;
    // cleanup 
    erh.cleanupHandler();
    eh.cleanupHandler();

    return EXIT_SUCCESS;
}

