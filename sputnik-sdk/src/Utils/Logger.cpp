// Logger.cpp
//

#include <iostream>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/Types.h>
#include <UrbanLabs/Sdk/Utils/Timer.h>

using namespace std;

const std::vector<std::string> Logger::levels_ =
    {"[INFO]", "[DEBUG]", "[WARNING]", "[PROGRESS]", "[ERROR]"};

std::unique_ptr<Logger> Logger::logger_ = nullptr;

/* Example usage:
 *   INIT_LOGGING(Logger::INFO);
 *   LOGG(Logger::INFO) << "Strings" << 5 << 5.6 << "Numbers" << Logger::FLUSH;
 */
#ifdef ANDROID
#include <android/log.h>
/**
 * @brief AndroidLogHandler::AndroidLogHandler
 */
AndroidLogHandler::AndroidLogHandler() : preTag_(""), postTag_("") {
    ;
}
/**
 * @brief AndroidLogHandler::~AndroidLogHandler
 */
AndroidLogHandler::~AndroidLogHandler() {
    preTag_ = "";
    postTag_ = "";
}
/**
 * @brief AndroidLogHandler::operator <<
 * @param record
 */
void AndroidLogHandler::operator << (string record) {
    __android_log_write(ANDROID_LOG_ERROR, preTag_.c_str(), (currentTime()+record+" "+postTag_).c_str());
}
/**
 * @brief AndroidLogHandler::setPreTag
 * @param tag
 */
void AndroidLogHandler::setPreTag(string tag) {
    preTag_ = tag;
}
/**
 * @brief AndroidLogHandler::setPostTag
 * @param tag
 */
void AndroidLogHandler::setPostTag(string tag) {
    postTag_ = tag;
}
#endif // ANDROID
/**
 * @brief FileLogHandler::FileLogHandler
 * @param fileName
 */
FileLogHandler::FileLogHandler(string fileName)
    : fileName_(fileName), preTag_(""), postTag_("") {
    stream_.open(fileName, ios_base::app);
}
/**
 * @brief FileLogHandler::~FileLogHandler
 */
FileLogHandler::~ FileLogHandler() {
    stream_.close();
}
/**
 * @brief FileLogHandler::operator <<
 * @param record
 */
void FileLogHandler::operator << (string record) {
    stream_ << currentTime() << preTag_ << " " << record << " " << postTag_ << endl;
}
/**
 * @brief FileLogHandler::setPreTag
 * @param tag
 */
void FileLogHandler::setPreTag(string tag) {
    preTag_ = tag;
}
/**
 * @brief FileLogHandler::setPostTag
 * @param tag
 */
void FileLogHandler::setPostTag(string tag) {
    postTag_ = tag;
}
/**
 * @brief StdoutLogHandler::StdoutLogHandler
 */
StdoutLogHandler::StdoutLogHandler()
    : preTag_(""), postTag_("") {
    ;
}
/**
 * @brief StdoutLogHandler::~StdoutLogHandler
 */
StdoutLogHandler::~ StdoutLogHandler() {
    preTag_ = "";
    postTag_ = "";
}
/**
 * @brief StdoutLogHandler::operator <<
 * @param record
 */
void StdoutLogHandler::operator << (string record) {
    cout << currentTime() << preTag_ << " " << record << " " << postTag_ << endl;
}
/**
 * @brief StdoutLogHandler::setPreTag
 * @param tag
 */
void StdoutLogHandler::setPreTag(string tag) {
    preTag_ = tag;
}
/**
 * @brief StdoutLogHandler::setPostTag
 * @param tag
 */
void StdoutLogHandler::setPostTag(string tag) {
    postTag_ = tag;
}
/**
 * @brief Logger::init
 * @param lvl
 * @param handler
 * @return
 */
Logger &Logger::init(Logger::LEVEL lvl, ILogHandler *handler) {
    isSet_ = true;
    dummyMode_ = true;

    level_ = lvl;
    handler_ = handler;
    preTag_ = "";
    postTag_ = "";
    return *this;
}
/**
 * @brief Logger::Logger
 */
Logger::Logger()
    : isSet_(false), dummyMode_(false), level_(Logger::ERROR), preTag_(""), postTag_(""), handler_(0) {
    ;
}
Logger::Logger(const Logger&) :
    isSet_(false), dummyMode_(false), level_(Logger::ERROR), preTag_(""), postTag_(""), handler_(0){
    ;
}
/**
 * @brief Logger::Logger
 * @param lvl
 * @param handler
 */
Logger::Logger(Logger::LEVEL lvl, ILogHandler *handler)
    : isSet_(true), dummyMode_(true), level_(lvl), preTag_(""), postTag_(""), handler_(handler) {
    init(lvl, handler);
}
/**
 * @brief Logger::~Logger
 */
Logger::~Logger() {
    if(handler_ != 0)
        delete handler_;
}
/**
 * @brief Logger::operator <<
 * @param level
 * @return
 */
Logger &Logger::operator << (Logger::LEVEL level) {
    std::lock_guard<std::mutex> guard(Logger::getInstance().lock_);
    if(level < level_) {
        dummyMode_ = true;
    } else {
        dummyMode_ = false;
    }
    return *this;
}
/**
 * @brief Logger::operator <<
 * @param cmd
 * @return
 */
Logger &Logger::operator << (Logger::CMD cmd) {
    std::lock_guard<std::mutex> guard(Logger::getInstance().lock_);
    switch(cmd) {
    case Logger::FLUSH:
        if(!dummyMode_) {
            getHandler()->operator << (lexical_cast(buffer_.str()));
        }
        buffer_.str("");
        buffer_.clear();
        break;
    }
    return *this;
}
/**
 * @brief Logger::setPreTag
 * @param tag
 * @return
 */
Logger &Logger::setPreTag(string tag) {
    std::lock_guard<std::mutex> guard(Logger::getInstance().lock_);
    preTag_ = tag;
    getHandler()->setPreTag(preTag_);
    return *this;
}
/**
 * @brief Logger::setPostTag
 * @param tag
 * @return
 */
Logger &Logger::setPostTag(string tag) {
    std::lock_guard<std::mutex> guard(Logger::getInstance().lock_);
    postTag_ = tag;
    getHandler()->setPostTag(postTag_);
    return *this;
}
/**
 * @brief Logger::isSet
 * @return
 */
bool Logger::isSet() {
    return isSet_;
}
/**
 * @brief init_logging
 * @param level
 * @return
 */
void init_logging(Logger::LEVEL level) {
    std::lock_guard<std::mutex> guard(Logger::getInstance().lock_);
    if(!Logger::getInstance().isSet()) {
#if ANDROID
        Logger::getInstance().init(level, new AndroidLogHandler());
#else
        Logger::getInstance().init(level, new StdoutLogHandler());
#endif
    }
}
/**
 * @brief Logger::getHandler
 */
ILogHandler* Logger::getHandler() {
    if(handler_ == nullptr) {
        throw std::runtime_error("logger has no handler");
    }

    return handler_;
}
/**
 * @brief Logger::getLevel
 */
std::string Logger::getLevel(Logger::LEVEL level) {
    return Logger::levels_[level];
}
/**
 * @brief Logger::getInstance
 */
Logger &Logger::getInstance() {
    if(!Logger::logger_) {
        Logger::logger_.reset(new Logger());
    }

    return *logger_;
}
