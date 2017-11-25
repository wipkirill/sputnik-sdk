#pragma once

#include <UrbanLabs/Sdk/Platform/Stdafx.h>

/**
 * @brief drawProgressBar
 * @param processed
 * @param total
 */
void drawProgressBar(size_t processed, size_t total);
/**
 * @brief getStdoutFromCommand
 * @param cmd
 * @return
 */
std::string getStdoutFromCommand(const std::string& cmd);
