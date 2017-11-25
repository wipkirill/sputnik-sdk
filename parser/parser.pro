TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++11 -Wno-sign-compare

SOURCES += \
    OGR/LineString.cpp \
    OGR/Polygon.cpp \
    OGR/OGRTypes.cpp \
    OsmInput.cpp \
    ParserPlugin.cpp \
    GeometryRoutines.cpp \
    TilePlugin.cpp \
    TagPlugin.cpp \
    SearchPlugin.cpp \
    RoutingPlugin.cpp \
    PublicTransPlugin.cpp \
    PassHandler.cpp \
    Parser.cpp \
    ParserOptions.cpp \
    MapInfoPlugin.cpp \
    KdTreeSpatial.cpp \
    Importer.cpp \
    EndPointPlugin.cpp \
    EdgeResolvePlugin.cpp \
    AddressDecoderPlugin.cpp \
    gdal/osm_parser.cpp \
    gdal/cplstringlist.cpp \
    gdal/cplstring.cpp \
    gdal/cpl_vsisimple.cpp \
    gdal/cpl_vsil.cpp \
    gdal/cpl_vsil_unix_stdio_64.cpp \
    gdal/cpl_vsil_tar.cpp \
    gdal/cpl_vsil_subfile.cpp \
    gdal/cpl_vsil_stdout.cpp \
    gdal/cpl_vsil_stdin.cpp \
    gdal/cpl_vsil_sparsefile.cpp \
    gdal/cpl_vsil_gzip.cpp \
    gdal/cpl_vsil_cache.cpp \
    gdal/cpl_vsil_buffered_reader.cpp \
    gdal/cpl_vsil_abstract_archive.cpp \
    gdal/cpl_vsi_mem.cpp \
    gdal/cpl_time.cpp \
    gdal/cpl_strtod.cpp \
    gdal/cpl_string.cpp \
    gdal/cpl_recode.cpp \
    gdal/cpl_recode_stub.cpp \
    gdal/cpl_recode_iconv.cpp \
    gdal/cpl_path.cpp \
    gdal/cpl_multiproc.cpp \
    gdal/cpl_minizip_zip.cpp \
    gdal/cpl_minizip_unzip.cpp \
    gdal/cpl_minizip_ioapi.cpp \
    gdal/cpl_minixml.cpp \
    gdal/cpl_error.cpp \
    gdal/cpl_conv.cpp

OTHER_FILES += \
    parser.pro.user

HEADERS += \
    ParserVersion.h \
    OGR/OGR.h \
    OGR/OGRTypes.h \
    OGR/LineString.h \
    OGR/Polygon.h \
    GeometryRoutines.h \
    TilePlugin.h \
    TagPlugin.h \
    TagFilter.h \
    SearchPlugin.h \
    RoutingPlugin.h \
    PublicTransPlugin.h \
    PassHandler.h \
    ParserPlugin.h \
    ParserOptions.h \
    OsmInput.h \
    MapInfoPlugin.h \
    KdTreeSpatial.h \
    Importer.h \
    EndPointPlugin.h \
    EdgeResolvePlugin.h \
    EdgeFilter.h \
    AddressDecoderPlugin.h \
    gdal/osm_parser.h \
    gdal/gpb.h \
    gdal/cpl_vsi.h \
    gdal/cpl_vsi_virtual.h \
    gdal/cpl_time.h \
    gdal/cpl_string.h \
    gdal/cpl_port.h \
    gdal/cpl_multiproc.h \
    gdal/cpl_minizip_zip.h \
    gdal/cpl_minizip_unzip.h \
    gdal/cpl_minizip_ioapi.h \
    gdal/cpl_minixml.h \
    gdal/cpl_error.h \
    gdal/cpl_conv.h \
    gdal/cpl_config.h

INCLUDEPATH += $$PWD/../sputnik-sdk/
DEPENDPATH += $$PWD/../sputnik-sdk/

INCLUDEPATH += /usr/include/libxml2

unix|win32: LIBS += -lpthread -lz -lxml2 -lgeos

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../sputnik-sdk/build-sdk/ -lsputnik
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../sputnik-sdk/build-sdk/ -lsputnik
else:unix: LIBS += -L$$PWD/../sputnik-sdk/build-sdk/ -lsputnik

unix|win32: LIBS += -L$$PWD/../sputnik-sdk/build-sdk/ -lspatialindex

INCLUDEPATH += $$PWD/../sputnik-sdk
DEPENDPATH += $$PWD/../sputnik-sdk
