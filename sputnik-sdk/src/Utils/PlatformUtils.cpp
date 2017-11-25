// Utility.cpp
//

#include <string>
#include <UrbanLabs/Sdk/Utils/PlatformUtils.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Utils/Types.h>

using namespace std;

/**
 * @brief drawProgressBar
 * @param processed
 * @param total
 */
void drawProgressBar(size_t processed, size_t total) {
    int len = 60;
    double percent = (double)processed/(double)total;
    LOGG(Logger::INFO) << "\x1B[2K"; // Erase the entire current line.
    LOGG(Logger::INFO) << "\x1B[0E"; // Move to the beginning of the current line.
    string progress;
    for(int i = 0; i < len; ++i) {
        if(i < static_cast<int>(len * percent)) {
            progress += "=";
        } else {
            progress += " ";
        }
    }
    LOGG(Logger::INFO) << "[" << progress << "] " << (static_cast<int>(100 * percent)) << Logger::FLUSH;
    //flush(LOGG(Logger::INFO)); // Required.
}

/**
 * @brief getStdoutFromCommand
 * @param cmd
 * @return
 */
string getStdoutFromCommand(const string& cmd) {
    string data;
    FILE * stream;
    const int maxBufferLen = 256;
    char buffer[maxBufferLen+1];
    string cmdCopy = cmd;

    // ok, apparently this is portable
    cmdCopy.append(" 2>&1");

    stream = popen(cmdCopy.c_str(), "r");
    if (stream) {
        while (!feof(stream)) {
            if (fgets(buffer, maxBufferLen, stream) != NULL) {
                data.append(buffer);
            }
        }
        pclose(stream);
    }
    LOGG(Logger::DEBUG) << "Running command" << cmd << "output:" << data<< "%";

    return data;
}

