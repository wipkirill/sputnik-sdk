// Timer.cpp
//
#include <cassert>
#include <UrbanLabs/Sdk/Utils/Timer.h>

using namespace std;

#define DAYS_IN_MONTH {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
#define DAYS {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"}
#define MONTHS {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}

const DateTime DateTime::invalid_ = DateTime(DateTime::INF, DateTime::INF, DateTime::INF, Time("00:00:00"));
const string DateTime::dayName_[] = DAYS;
const string DateTime::monthName_[] = MONTHS;
const int DateTime::daysInMonth_[] = DAYS_IN_MONTH;

/**
 * Get current date/time, format is YYYY-MM-DDTHH:mm:ss
 **/
const string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%X", &tstruct);
    return buf;
}
/**
 * Get current date/time, format is HH:mm:ss
 **/
const string currentTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "[%X]", &tstruct);
    return buf;
}
/**
 * @brief Timer::Timer
 */
Timer::Timer() : t1(), t2() {
    start();
}
/**
 * @brief Timer::start
 */
void Timer::start() {
#ifdef _WIN64
    //define something for Windows (64-bit)
#error "not defined yet"
#elif _WIN32
    //define something for Windows (32-bit)
#error "not defined yet"
#elif __APPLE__
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    t1.tv_sec = mts.tv_sec;
    t1.tv_nsec = mts.tv_nsec;
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
    clock_gettime(CLOCK_REALTIME, &t1);
#elif __linux
    clock_gettime(CLOCK_REALTIME, &t1);
#elif __unix // all unices not caught above
    // Unix
#error "not defined yet"
#elif __posix
    // POSIX
#error "not defined yet"
#endif

}
/**
 * @brief Timer::stop
 */
void Timer::stop() {
#ifdef _WIN64
    //define something for Windows (64-bit)
#error "not defined yet"
#elif _WIN32
    //define something for Windows (32-bit)
#error "not defined yet"
#elif __APPLE__
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    t2.tv_sec = mts.tv_sec;
    t2.tv_nsec = mts.tv_nsec;
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
    clock_gettime(CLOCK_REALTIME, &t2);
#elif __linux
    clock_gettime(CLOCK_REALTIME, &t2);
#elif __unix // all unices not caught above
    // Unix
#error "not defined yet"
#elif __posix
    // POSIX
#error "not defined yet"
#endif
}
double Timer::getElapsedTimeSec() {
#ifdef _WIN64
    //define something for Windows (64-bit)
#error "not defined yet"
#elif _WIN32
    //define something for Windows (32-bit)
#error "not defined yet"
#elif __APPLE__
    return (t2.tv_sec - t1.tv_sec)+(double)(t2.tv_nsec - t1.tv_nsec)/1e9;
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
    return (t2.tv_sec - t1.tv_sec)+(double)(t2.tv_nsec - t1.tv_nsec)/1e9;
#elif __linux
    return (t2.tv_sec - t1.tv_sec)+(double)(t2.tv_nsec - t1.tv_nsec)/1e9;
#elif __unix // all unices not caught above
    // Unix
#error "not defined yet"
#elif __posix
    // POSIX
#error "not defined yet"
#endif
}
/**
 * @brief Time::Time
 */
Time::Time()
    : hour_(0), minute_(0), second_(0), valid_(false) {
    ;
}
/**
 * @brief Time::Time
 * @param time
 */
Time::Time(string time)
    : hour_(0), minute_(0), second_(0), valid_(false) {
    vector<int> colons(1, -1);
    for(size_t i = 0; i < time.size(); i++) {
        if(time[i] == ':')
            colons.push_back(i);
    }
    colons.push_back(time.size());
    if(colons.size() == 4) {
        hour_ = lexical_cast<int>(time.substr(colons[0]+1, colons[1]-colons[0]-1));
        minute_ = lexical_cast<int>(time.substr(colons[1]+1, colons[2]-colons[1]-1));
        second_ = lexical_cast<int>(time.substr(colons[2]+1, colons[3]-colons[2]-1));
        valid_ = true;
    } else if(colons.size() == 3) {
        hour_ = lexical_cast<int>(time.substr(colons[0]+1, colons[1]-colons[0]-1));
        minute_ = lexical_cast<int>(time.substr(colons[1]+1, colons[2]-colons[1]-1));
        valid_ = true;
    } else {
        valid_ = false;
    }
}
/**
 * @brief Time::Time
 * @param h
 * @param m
 * @param s
 */
Time::Time(int h, int m, int s)
    : hour_(h), minute_(m), second_(s), valid_(false) {
    valid_ = isValid();
}
/**
 * @brief Time::incrementBy
 * @param timeInSeconds
 * @return
 */
int Time::incrementBy(int timeInSeconds) {
    // increment seconds
    second_ += timeInSeconds;
    timeInSeconds = second_-second_ % SECS;
    second_ %= SECS;

    // increment minutes
    minute_ += timeInSeconds/SEC_IN_MIN;
    timeInSeconds = minute_*SEC_IN_MIN-(minute_ % MINS)*SEC_IN_MIN;
    minute_ %= MINS;

    // increment hours
    hour_ += timeInSeconds/SEC_IN_HOUR;
    int carry = hour_/HOURS;
    hour_ %= HOURS;

    return carry;
}
/**
 * @brief Time::getHour
 * @return
 */
int Time::getHour() const {
    return hour_;
}
/**
 * @brief Time::getMinute
 * @return
 */
int Time::getMinute() const {
    return minute_;
}
/**
 * @brief Time::getSecond
 * @return
 */
int Time::getSecond() const {
    return second_;
}
/**
 * @brief Time::getDiff
 * @param time
 * @return
 */
int Time::getDiff(const Time &time) const {
    return (hour_-time.hour_)*SEC_IN_HOUR+(minute_-time.minute_)*SEC_IN_MIN+(second_-time.second_);
}
/**
 * @brief Time::isValid
 * @return
 */
bool Time::isValid() const {
    return (hour_ <= 23 && hour_ >= 0) &&
           (minute_ >= 0 && minute_ <= 59) &&
           (second_ >= 0 && second_ <= 59) && valid_;
}
/**
 * @brief Time::toString
 * @return
 */
string Time::toString() const {
    // assert(isValid());
    char str[9];
    str[8] = '\0';
    sprintf(str, "%02d:%02d:%02d", hour_, minute_, second_);
    return string(str);
}
/**
 * @brief Time::operator <<
 * @param oss
 * @return
 */
ostream &Time::operator << (ostream &oss) {
    oss << toString();
    return oss;
}
/**
 * @brief Time::operator <
 * @param time
 * @return
 */
bool Time::operator < (const Time &time) const {
    if(hour_ != time.hour_)
        return hour_ < time.hour_;
    if(minute_ != time.minute_) {
        return second_ < time.second_;
    }
    return second_ < time.second_;
}
/**
 * @brief Time::operator <=
 * @param time
 * @return
 */
bool Time::operator <= (const Time &time) const {
    if(hour_ != time.hour_)
        return hour_ <= time.hour_;
    if(minute_ != time.minute_) {
        return second_ <= time.second_;
    }
    return second_ <= time.second_;
}
/**
 * @brief Time::operator ==
 * @param time
 * @return
 */
bool Time::operator == (const Time &time) const {
    return hour_ == time.hour_ && minute_ == time.minute_ && second_ == time.second_;
}
/**
 * @brief DateTime::DateTime
 */
DateTime::DateTime() {
    ;
}
/**
 * @brief DateTime::DateTime
 * @param time
 * @param start
 */
DateTime::DateTime(string time, bool start) {
    if(time.find("T") == string::npos) {
        if(start)
            time_ = Time("00:00:00");
        else
            time_ = Time("23:59:59");
        year_ = lexical_cast<int>(time.substr(0, 4));
        month_ = lexical_cast<int>(time.substr(4, 2));
        day_ = lexical_cast<int>(time.substr(6, 2));
    } else {
        time_ = Time(time.substr(time.find("T")+1));
        year_ = lexical_cast<int>(time.substr(0, 4));
        month_ = lexical_cast<int>(time.substr(5, 2));
        day_ = lexical_cast<int>(time.substr(8, 2));
    }
}
/**
 * @brief DateTime::DateTime
 * @param y
 * @param m
 * @param d
 * @param t
 */
DateTime::DateTime(int y, int m, int d, Time t)
    : year_(y), month_(m), day_(d), time_(t) {
    ;
}
/**
 * @brief DateTime::setTime
 * @param time
 */
void DateTime::setTime(const Time &time) {
    time_ = time;
}
/**
 * @brief DateTime::getTime
 * @return
 */
Time DateTime::getTime() const {
    return time_;
}
/**
 * @brief DateTime::getYear
 * @return
 */
int DateTime::getYear() const {
    return year_;
}
/**
 * @brief DateTime::getMonth
 * @return
 */
int DateTime::getMonth() const {
    return month_;
}
/**
 * @brief DateTime::getDay
 * @return
 */
int DateTime::getDay() const {
    return day_;
}
/**
 * @brief DateTime::isLeapYear
 * @param year
 * @return
 */
bool DateTime::isLeapYear(int year) const {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}
/**
 * @brief DateTime::numDaysInMonth
 * @param year
 * @param month
 * @return
 */
int DateTime::numDaysInMonth(int year, int month) const {
    if(month == 2 && isLeapYear(year))
        return daysInMonth_[month-1]+1;
    return daysInMonth_[month-1];
}
/**
 * @brief DateTime::isValid
 * @return
 */
bool DateTime::isValid() const {
    if(numDaysInMonth(year_, month_) >= day_ && month_ >= 1 && month_ <= 12)
        return true;
    return false;
}
/**
* Returns day of the week for a date. Valid for all Gregorian calendar dates
* http://en.wikipedia.org/wiki/Calculating_the_day_of_the_week
* 0 = Monday, 1 = Tuesday...
* @brief getWeekDay
* @return
*/
int DateTime::getWeekDay() const {
    int y = year_, m = month_, d = day_;
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    y -= m < 3;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d+6) % 7;
}
/**
 * @brief DateTime::getMonthAsString
 * @return
 */
string DateTime::getMonthAsString() const {
    return monthName_[month_-1];
}
/**
 * @brief DateTime::getWeekDayAsString
 * @return
 */
string DateTime::getWeekDayAsString() const {
    return dayName_[getWeekDay()];
}
/**
 * @brief DateTime::incrementBy
 * @param timeInSeconds
 */
void DateTime::incrementBy(int timeInSeconds) {
    int days = time_.incrementBy(timeInSeconds);

    // update by number of days
    for(int left = numDaysInMonth(year_, month_)-day_; left < days;) {
        day_ = 1;
        days -= left;

        month_++;
        if(month_ == 12) {
            month_ = 1;
            year_++;
        }

        left = numDaysInMonth(year_, month_);
    }
    if(days > 0) {
        day_ += days;
    }
}
/**
 * @brief DateTime::toString
 * @return
 */
string DateTime::toString() {
    char str[11];
    str[10] = '\0';
    sprintf(str, "%04d-%02d-%02d", year_, month_, day_);
    return string(str)+"T"+time_.toString();
}
/**
 * @brief DateTime::operator <
 * @param date
 * @return
 */
bool DateTime::operator < (const DateTime &date) const {
    if(year_ != date.year_)
        return year_ < date.year_;
    if(month_ != date.month_)
        return month_ < date.month_;
    if(day_ != date.day_)
        return day_ < date.day_;
    return time_ < date.time_;
}
/**
 * @brief DateTime::operator <=
 * @param date
 * @return
 */
bool DateTime::operator <= (const DateTime &date) const {
    if(year_ != date.year_)
        return year_ <= date.year_;
    if(month_ != date.month_)
        return month_ <= date.month_;
    if(day_ != date.day_)
        return day_ <= date.day_;
    return time_ <= date.time_;
}
/**
 * @brief DateTime::operator ==
 * @param date
 * @return
 */
bool DateTime::operator == (const DateTime &date) const {
    return year_ == date.year_ && month_ == date.month_ && day_ == date.day_ && time_ == date.time_;
}
/**
 * @brief DateTime::operator !=
 * @param date
 * @return
 */
bool DateTime::operator != (const DateTime &date) const {
    return !(operator ==(date));
}

#undef LZZ_INLINE
