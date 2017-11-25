#pragma once

#include "ParserPlugin.h"

class OsmInput {
public:
    /**
     * @brief read
     * @param inputFile
     * @param handler
     */
    static bool read(const std::string& inputFile, Plugin& handler);
};
