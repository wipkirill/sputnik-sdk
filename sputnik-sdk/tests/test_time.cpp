#include <iostream>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Utils/Timer.h>
#include "test_time.h"

using namespace std;

void TestTime::test() {
	INIT_LOGGING(Logger::INFO);
	
	Time time1("20:44:21");
    QVERIFY(time1.isValid());
    QVERIFY(time1.toString() == "20:44:21");

    Time time2 = time1;
    int carry = time2.incrementBy(38);
    QVERIFY(carry == 0);
    QVERIFY(time2.isValid());
    QVERIFY(time2.toString() == "20:44:59");

    carry = time1.incrementBy(39);
    QVERIFY(carry == 0);
    QVERIFY(time1.isValid());
    QVERIFY(time1.toString() == "20:45:00");

    carry = time1.incrementBy(Time::SEC_IN_HOUR*5);
    QVERIFY(carry == 1);
    QVERIFY(time1.isValid());
    QVERIFY(time1.toString() == "01:45:00");

    DateTime date1("2000-02-28T20:00:00");
    QVERIFY(date1.isValid());
    QVERIFY(date1.toString() == "2000-02-28T20:00:00");

    date1.incrementBy(Time::SEC_IN_HOUR*4);
    QVERIFY(date1.isValid());
    QVERIFY(date1.toString() == "2000-02-29T00:00:00");

    DateTime date2("2013-03-11T15:41:00");
    QVERIFY(date2.isValid());
    QVERIFY(!date2.isLeapYear(date2.getYear()));
    QVERIFY(date2.getWeekDayAsString() == "Mon");
}