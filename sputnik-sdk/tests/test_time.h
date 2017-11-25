#pragma once

#include "AutoTest.h"

class TestTime : public QObject
{
    Q_OBJECT

private slots:
    void test();
};

DECLARE_TEST(TestTime)