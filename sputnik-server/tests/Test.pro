# -------------------------------------------------
# Project created by QtCreator 2010-04-28T20:25:30
# -------------------------------------------------
QT = core testlib network
QMAKE_CXXFLAGS += -std=c++11
SOURCES += main.cpp \
	util.cpp \
    test1.cpp \
    test_integration.cpp \
    test_graph_routing.cpp \
    test_search.cpp
HEADERS += AutoTest.h \
	util.h \
    test1.h \
    test_integration.h \
    test_graph_routing.h \
    test_search.h
