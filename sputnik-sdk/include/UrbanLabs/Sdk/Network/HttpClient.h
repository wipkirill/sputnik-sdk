#pragma once

#include <map>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>

// do not change the order here, ANDROID should come before __linux
//#ifdef _WIN64
////define something for Windows (64-bit)
//#elif _WIN32
////define something for Windows (32-bit)
//#elif __APPLE__
//#include <curl/curl.h>
//#include <curl/easy.h>
//#ifdef TARGET_OS_IPHONE
//// iOS
//#elif TARGET_IPHONE_SIMULATOR
//// iOS Simulator
//#elif TARGET_OS_MAC
//// Other kinds of Mac OS
//#else
//// Unsupported platform
//#endif
//#elif ANDROID
//#include <curl/curl.h>
//#include <curl/easy.h>
//#elif __linux
//#include <curl/curl.h>
//#include <curl/easy.h>
//#elif __unix // all unices not caught above
//// Unix
//#elif __posix
//// POSIX
//#endif
//
//#ifndef ANDROID
///**
 //* @brief The HttpClient class
 //*/
//class HttpClient {
//private:
    //std::string host_;
//public:
    //HttpClient(const std::string &host);
//private:
    //std::string mapToRequest(const std::map <std::string, std::string> &req) const;
    //static size_t writeToString(void *contents, size_t size, size_t nmemb, void *userp);
    //static int waitOnSocket(curl_socket_t sockfd, int for_recv, long int timeout_ms);
    //bool send(CURL *curl, const std::string &request, std::string &response) const;
//public:
    //bool get(const std::string &path, const std::string &request, std::string &response) const;
    //bool get(const std::string &path, const std::map <std::string, std::string> request, std::string &response) const;
    //bool post(const std::string &path, const std::string &request, const std::map <std::string, std::string> options, std::string &response) const;
    //bool post(const std::string &path, const std::map <std::string, std::string> request, const std::map <std::string, std::string> options, std::string &response) const;
//};
//#endif
