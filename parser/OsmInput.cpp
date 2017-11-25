#include <string>
#include "OsmInput.h"


#include <osmium/io/pbf_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>

using namespace std;

/**
 * @brief read
 * @param inputFile
 * @param handler
 */
bool OsmInput::read(const string& inputFile, Plugin& handler) {
    osmium::io::File infile(inputFile);
    osmium::io::Reader reader(infile);
    handler.init();

    osmium::apply(reader, handler);
    handler.finalize();
    reader.close();
    return true;
}