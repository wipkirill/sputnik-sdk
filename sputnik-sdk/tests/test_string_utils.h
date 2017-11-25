#pragma once

#include "AutoTest.h"

class TestStringUtils : public QObject
{
    Q_OBJECT

private slots:
    void test();
};

DECLARE_TEST(TestStringUtils)