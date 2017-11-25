#pragma once

#include "AutoTest.h"

class TestMathUtils : public QObject
{
    Q_OBJECT

private slots:
    void test();
};

DECLARE_TEST(TestMathUtils)