#pragma once

// http://stackoverflow.com/questions/7497637/c-detect-out-of-range-access
//#define _GLIBCXX_DEBUG
//#define _GLIBXX_DEBUG_PEDANTIC
//#define _GLIBCXX_FULLY_DYNAMIC_STRING

// do not change the order here, ANDROID should come before __linux
#ifdef _WIN64
   //define something for Windows (64-bit)
#elif _WIN32
   //define something for Windows (32-bit)
#elif __APPLE__
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

#elif __linux

#elif __unix // all unices not caught above
    // Unix
#elif __posix
    // POSIX
#endif
