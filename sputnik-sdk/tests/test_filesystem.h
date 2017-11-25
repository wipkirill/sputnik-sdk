#pragma once

#include "AutoTest.h"

class TestFilesystem : public QObject
{
    Q_OBJECT

private slots:
    void test();
};

DECLARE_TEST(TestFilesystem)