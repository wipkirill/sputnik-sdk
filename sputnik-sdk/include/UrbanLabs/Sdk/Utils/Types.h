#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <cstdlib>

#include <UrbanLabs/Sdk/Platform/Stdafx.h>

//------------------------------------------------------------------------------
// lexical casting (converting string to some integral type)
// - There should only be standard types here, no int, long long and others
//------------------------------------------------------------------------------
template<typename T>
inline T lexical_cast(const std::string &str);

template<>
inline std::string lexical_cast<std::string>(const std::string &str) {
    return str;
}
template<>
inline int8_t lexical_cast<int8_t>(const std::string &str) {
    return atoi(str.c_str());
}

template<>
inline uint8_t lexical_cast<uint8_t>(const std::string &str) {
    return atoi(str.c_str());
}

template<>
inline int32_t lexical_cast<int32_t>(const std::string &str) {
    return atoi(str.c_str());
}

template<>
inline uint32_t lexical_cast<uint32_t>(const std::string &str) {
    return atol(str.c_str());
}

template<>
inline int64_t lexical_cast<int64_t>(const std::string &str) {
    return atoll(str.c_str());
}

template<>
inline uint64_t lexical_cast<uint64_t>(const std::string &str) {
    return atoll(str.c_str());
}

template<>
inline float lexical_cast<float>(const std::string &str) {
    return atof(str.c_str());
}

template<>
inline double lexical_cast<double>(const std::string &str) {
    return atof(str.c_str());
}

template<typename T>
inline std::string lexical_cast(const T &t) {
    std::stringstream ss;
    ss << std::setprecision(16);
    ss << t;
    return ss.str();
}
//------------------------------------------------------------------------------
// type specification for printf
//------------------------------------------------------------------------------
template<typename T>
inline std::string type_spec();

template<>
inline std::string type_spec<int32_t>() {
    return "%d";
}

template<>
inline std::string type_spec<uint32_t>() {
    return "%u";
}

template<>
inline std::string type_spec<int64_t>() {
#ifdef _WIN64
   //define something for Windows (64-bit)
   #error "not implemented yet"
#elif _WIN32
   //define something for Windows (32-bit)
   #error "not implemented yet"
#elif __APPLE__
    return "%lld";
    #include "TargetConditionals.h"
    #ifdef TARGET_OS_IPHONE
         // iOS
    #elif TARGET_IPHONE_SIMULATOR
        // iOS Simulator
    #elif TARGET_OS_MAC
        // Other kinds of Mac OS
    #else
        // Unsupported platform
    #endif
#elif ANDROID
    return "%lld";
#elif __linux
    return "%lld";
#elif __unix // all unices not caught above
    // Unix
    #error "not implemented yet"
#elif __posix
    // POSIX
    #error "not implemented yet"
#endif
}

template<>
inline std::string type_spec<uint64_t>() {
#ifdef _WIN64
   //define something for Windows (64-bit)
   #error "not implemented yet"
#elif _WIN32
   //define something for Windows (32-bit)
   #error "not implemented yet"
#elif __APPLE__
    return "%llu";
    #include "TargetConditionals.h"
    #ifdef TARGET_OS_IPHONE
         // iOS
    #elif TARGET_IPHONE_SIMULATOR
        // iOS Simulator
    #elif TARGET_OS_MAC
        // Other kinds of Mac OS
    #else
        // Unsupported platform
    #endif
#elif ANDROID
    return "%llu";
#elif __linux
    return "%llu";
#elif __unix // all unices not caught above
    // Unix
    #error "not implemented yet"
#elif __posix
    // POSIX
    #error "not implemented yet"
#endif
}

template<>
inline std::string type_spec<int8_t>() {
    return "%c";
}

template<>
inline std::string type_spec<uint8_t>() {
    return "%u";
}

template<>
inline std::string type_spec<float>() {
    return "%.9f";
}

template<>
inline std::string type_spec<double>() {
    return "%.9lf";
}

template<>
inline std::string type_spec<std::string>() {
    return "%s";
}
