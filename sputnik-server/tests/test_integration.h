#ifndef TEST_INTEGRATION_H
#define TEST_INTEGRATION_H

#include "AutoTest.h"
#include "util.h"

class TestServer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void heartBeat();
    void heartBeat_data();
    void cleanupTestCase();
};

DECLARE_TEST(TestServer)

#endif // TEST_INTEGRATION_H