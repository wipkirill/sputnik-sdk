#pragma once

#include <mutex>
#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstring>


#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/Types.h>
#include <UrbanLabs/Sdk/Utils/ThreadSafe.h>

/**
 * @brief The ILogHandler class
 */
class ILogHandler {
public:
    virtual void operator << (std::string record) = 0;
    virtual void setPreTag(std::string tag) = 0;
    virtual void setPostTag(std::string tag) = 0;
    virtual ~ILogHandler() {;}
};
/**
 * @brief The FileLogHandler class
 */
class FileLogHandler : public ILogHandler {
private:
    std::string fileName_;
    std::string preTag_;
    std::string postTag_;
    std::ofstream stream_;
public:
    FileLogHandler(std::string fileName);
    ~ FileLogHandler();
    void operator << (std::string record);
    void setPreTag(std::string tag);
    void setPostTag(std::string tag);
};
/**
 * @brief The StdoutLogHandler class
 */
class StdoutLogHandler : public ILogHandler {
private:
    std::string preTag_;
    std::string postTag_;
public:
    StdoutLogHandler();
    ~ StdoutLogHandler();
    void operator << (std::string record);
    void setPreTag(std::string tag);
    void setPostTag(std::string tag);
};
#ifdef ANDROID
/**
 * @brief The AndroidLogHandler class
 */
class AndroidLogHandler : public ILogHandler {
private:
    std::string preTag_;
    std::string postTag_;
public:
    AndroidLogHandler();
    ~AndroidLogHandler();
    void operator << (std::string record);
    void setPreTag(std::string tag);
    void setPostTag(std::string tag);
};
#endif
/**
 * @brief The Logger class
 */
class Logger {
public:
    enum LEVEL {
        INFO = 0,
        DEBUG = 1,
        WARNING = 2,
        PROGRESS = 3,
        ERROR = 4
    };
    enum CMD {
        FLUSH = 0
    };
private:
    static const std::vector<std::string> levels_;
    // is the logger handler set
    bool isSet_;
    // is set until the next level is set to be higher than the loggers
    // current level
    bool dummyMode_;
    // current logger level, ignores everything lower than the level
    LEVEL level_;
    // preppends record with
    std::string preTag_;
    // appends to the record
    std::string postTag_;
    // log resource handler (file, stdout or any other resource)
    ILogHandler *handler_;
    // log buffer, accumulates until is flushed
    std::stringstream buffer_;
public:
    // mutex lock
    std::mutex lock_;
    Logger &init(Logger::LEVEL lvl, ILogHandler *handler);
    Logger();
    Logger(Logger::LEVEL lvl, ILogHandler *handler);
    ~ Logger();
    template <typename T>
    Logger &operator << (T token);
    Logger &operator << (Logger::LEVEL level);
    Logger &operator << (Logger::CMD cmd);
    Logger &setPreTag(std::string tag);
    Logger &setPostTag(std::string tag);
    ILogHandler* getHandler();
    static std::string getLevel(Logger::LEVEL level);
    static Logger &getInstance();
    bool isSet();
private:
    Logger(const Logger&);
    Logger& operator=(const Logger&) {return *this;}
    static std::unique_ptr<Logger> logger_;
};

template <typename T>
Logger &Logger::operator << (T token) {
    if(!dummyMode_) {
        lock_.lock();
        buffer_ << " " << token;
        lock_.unlock();
    }
    return *this;
}

void init_logging(Logger::LEVEL level);

#define INIT_LOGGING(level) init_logging(level);
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOCATION "file: "+lexical_cast(__FILENAME__)+" function: "+lexical_cast(__FUNCTION__)+" line: "+lexical_cast(__LINE__)
#define LOGG(tag) Logger::getInstance().setPreTag(Logger::getLevel(tag)).setPostTag(std::string("(")+std::string(LOCATION)+std::string(")")) << tag
