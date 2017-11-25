#pragma once

#include "AutoTest.h"

class TestObjectIdStorage : public QObject
{
    Q_OBJECT

private slots:
    void test();
};

DECLARE_TEST(TestObjectIdStorage)