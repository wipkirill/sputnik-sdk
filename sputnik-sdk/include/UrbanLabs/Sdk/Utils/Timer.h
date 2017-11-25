#pragma once

#include <vector>

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/Types.h>

// do not change the order here, ANDROID should come before __linux
#ifdef _WIN64
   //define something for Windows (64-bit)
#elif _WIN32
   //define something for Windows (32-bit)
#elif __APPLE__
    #include "TargetConditionals.h"
    #include <sys/time.h>
    #include <time.h>
    #include <mach/clock.h>
    #include <mach/mach.h>
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
    #include <sys/time.h>
    #include <time.h>
#elif __linux
    #include <sys/time.h>
    #include <time.h>
#elif __unix // all unices not caught above
    // Unix
#elif __posix
    // POSIX
#endif

using std::ostream;

std::string const currentDateTime();
std::string const currentTime();

class Timer {
private:
#ifdef _WIN64
    #error "not defined"
   //define something for Windows (64-bit)
#elif _WIN32
    #error "not defined"
   //define something for Windows (32-bit)
#elif __APPLE__
    timespec t1;
    timespec t2;
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
    timespec t1;
    timespec t2;
#elif __linux
    timespec t1;
    timespec t2;
#elif __unix // all unices not caught above
    // Unix
    #error "not defined"
#elif __posix
    #error "not defined"
    // POSIX
#endif
public:
    Timer();
    void start();
    void stop();
    double getElapsedTimeSec();
};

class Time {
private:
    int hour_;
    int minute_;
    int second_;
    bool valid_;
public:
    typedef int TimeDiff;
    static const int SECS = 60;
    static const int MINS = 60;
    static const int HOURS = 24;
    static const int SEC_IN_HOUR = 60*60;
    static const int SEC_IN_MIN = 60;
public:
    Time();
    Time(std::string time);
    Time(int h, int m, int s);
    int incrementBy(int timeInSeconds);
    int getHour() const;
    int getMinute() const;
    int getSecond() const;
    int getDiff(const Time &time) const;
    bool isValid() const;
    std::string toString() const;
    std::ostream &operator << (std::ostream &oss);
    friend std::ostream &operator << (std::ostream &os, const Time &time) {
        os << time.toString();
        return os;
    }
    bool operator < (const Time &time) const;
    bool operator <= (const Time &time) const;
    bool operator == (const Time &time) const;
};

class DateTime {
private:
    int year_;
    int month_;
    int day_;
    Time time_;
private:
    static std::string const(dayName_) [];
    static std::string const(monthName_) [];
    static int const(daysInMonth_) [];
public:
    const static int INF = (1 << 30);
    const static DateTime invalid_;
    const static int SECONDS_IN_DAY = Time::SEC_IN_HOUR*24;
public:
    DateTime();
    DateTime(std::string time, bool start = true);
    DateTime(int y, int m, int d, Time t);
    void setTime(const Time &time);
    Time getTime() const;
    int getYear() const;
    int getMonth() const;
    int getDay() const;
    bool isLeapYear(int year) const;
    int numDaysInMonth(int year, int month) const;
    bool isValid() const;
    int getWeekDay() const;
    std::string getMonthAsString() const;
    std::string getWeekDayAsString() const;
    void incrementBy(int timeInSeconds);
    std::string toString();
    bool operator < (const DateTime &date) const;
    bool operator <= (const DateTime &date) const;
    bool operator == (const DateTime &date) const;
    bool operator != (const DateTime &date) const;
};
