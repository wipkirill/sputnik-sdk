#include <iostream>
#include <QDebug>

#include "test_string_utils.h"
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Utils/StringUtils.h>

using namespace std;

void TestStringUtils::test() {
    INIT_LOGGING(Logger::INFO);
    SimpleTokenator st("    dsfs   sdfds sdsf     dsf  ", ' ', '\"', true);
    int count = st.countTokens();
    QVERIFY(4 == count);

    string tokens [] = {"dsfs", "sdfds", "sdsf", "dsf"};
    for(int i = 0; i < 4; i++) {
        QVERIFY(st.nextToken() == tokens[i]);
    }

    SimpleTokenator st2("    a b   bc  def a b", ' ', '\"', true);
    int count2 = st2.countTokens();
    QVERIFY(6 == count2);

    string tokens2 [] = {"a", "b", "bc", "def", "a", "b"};
    for(int i = 0; i < 6; i++) {
        QVERIFY(st2.nextToken() == tokens2[i]);
    }

    SimpleTokenator st22("a b   bc  def a b   ", ' ', '\"', true);
    int count22 = st22.countTokens();
    QVERIFY(6 == count22);

    string tokens22 [] = {"a", "b", "bc", "def", "a", "b"};
    for(int i = 0; i < 6; i++) {
        QVERIFY(st22.nextToken() == tokens22[i]);
    }

    SimpleTokenator st3("abc,,abc", ',', '\"', false);
    int count3 = st3.countTokens();
    QVERIFY(count3 == 3);
    string tokens3 [] = {"abc", "", "abc"};
    for(int i = 0; i < 3; i++) {
        QVERIFY(st3.nextToken() == tokens3[i]);
    }

    SimpleTokenator st4(",abc,,abc,", ',', '\"', false);
    int count4 = st4.countTokens();
    QVERIFY(count4 == 5);
    string tokens4 [] = {"", "abc", "", "abc",""};
    for(int i = 0; i < 5; i++) {
        QVERIFY(st4.nextToken() == tokens4[i]);
    }

    SimpleTokenator st5(",\"abc\",", ',', '\"', false);
    int count5 = st5.countTokens();
    QVERIFY(count5 == 3);
    string tokens5 [] = {"", "\"abc\"", ""};
    for(int i = 0; i < 3; i++) {
        QVERIFY(st5.nextToken() == tokens5[i]);
    }

    SimpleTokenator st6(",\"abc,def\",\"abc, dsf sfdd ,def\",", ',', '\"', false);
    int count6 = st6.countTokens();
    QVERIFY(count6 == 4);
    string tokens6 [] = {"", "\"abc,def\"", "\"abc, dsf sfdd ,def\"", ""};
    for(int i = 0; i < 4; i++) {
        QVERIFY(st6.nextToken() == tokens6[i]);
    }

    SimpleTokenator st7("a b", ',', '\"', false);
    int count7 = st7.countTokens();
    QVERIFY(count7 == 1);
    string tokens7 [] = {"a b"};
    for(int i = 0; i < 1; i++) {
        QVERIFY(st7.nextToken() == tokens7[i]);
    }

    // base64 tests
    string testMsg = "Test message";
    string b64TestMsg = "VGVzdCBtZXNzYWdl";
    QVERIFY(StringUtils::base64_encode(testMsg.c_str(), testMsg.size()) == b64TestMsg);
    QVERIFY(StringUtils::base64_decode(b64TestMsg) == testMsg);

    string b64TestMsgTampered = b64TestMsg + "CbX";
    try {
        StringUtils::base64_decode(b64TestMsgTampered);
        QFAIL("Exception not thrown for tampered input");
    } catch(const std::runtime_error &e) {

    }

    // replace a character from another alphabet
    b64TestMsgTampered = "vGVzdCBtZXNzYWdKЮ";
    try {
        string result = "Result " + StringUtils::base64_decode(b64TestMsgTampered);
        qDebug() << QString::fromUtf8(result.c_str());;
        QFAIL("Exception not thrown for tampered input");
    } catch(const std::runtime_error &e) {

    }
}