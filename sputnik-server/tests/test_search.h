#ifndef TEST_SEARCH_H
#define TEST_SEARCH_H

#include "AutoTest.h"
#include "util.h"

class TestSearch : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void simple();
    void simple_data();
    void testMultipleObjects();
    void testMultipleObjects_data();
    void testSearchByIds();
    void testSearchByIds_data();
    void testNearestObjectBasic();
    void testNearestObjectBasic_data();
    void testNearestObject();
    void testNearestObject_data();
    void cleanupTestCase();
};

DECLARE_TEST(TestSearch)

#endif // TEST_SEARCH_H