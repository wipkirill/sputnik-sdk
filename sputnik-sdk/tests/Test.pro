# -------------------------------------------------
# Project created by QtCreator 2010-04-28T20:25:30
# -------------------------------------------------
QT = core testlib
QMAKE_CXXFLAGS += -std=c++11 -O2
SOURCES += main.cpp \
           test_storage.cpp \
           test_math_utils.cpp \
           test_time.cpp \
           test_string_utils.cpp \
           test_polyline_encoder.cpp \
           test_filesystem.cpp

HEADERS += AutoTest.h \
           test_storage.h \
           test_math_utils.h \
           test_time.h \
           test_string_utils.h \
           test_polyline_encoder.h \
           test_filesystem.h

CONFIG-=app_bundle
          
QMAKE_LFLAGS += -Wl,-rpath,#commonLibsPath
INCLUDEPATH += #commonIncludePath
DEPENDPATH += #commonIncludePath
LIBS += -L#commonLibsPath -lsputnik -lsqlite3
