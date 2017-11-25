/****************************************************************************
** Meta object code from reading C++ file 'test_graph_routing.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.2.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "test_graph_routing.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'test_graph_routing.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.2.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_TestGraphRouting_t {
    QByteArrayData data[18];
    char stringdata[239];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_TestGraphRouting_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_TestGraphRouting_t qt_meta_stringdata_TestGraphRouting = {
    {
QT_MOC_LITERAL(0, 0, 16),
QT_MOC_LITERAL(1, 17, 12),
QT_MOC_LITERAL(2, 30, 0),
QT_MOC_LITERAL(3, 31, 9),
QT_MOC_LITERAL(4, 41, 14),
QT_MOC_LITERAL(5, 56, 15),
QT_MOC_LITERAL(6, 72, 20),
QT_MOC_LITERAL(7, 93, 15),
QT_MOC_LITERAL(8, 109, 20),
QT_MOC_LITERAL(9, 130, 5),
QT_MOC_LITERAL(10, 136, 10),
QT_MOC_LITERAL(11, 147, 12),
QT_MOC_LITERAL(12, 160, 17),
QT_MOC_LITERAL(13, 178, 8),
QT_MOC_LITERAL(14, 187, 13),
QT_MOC_LITERAL(15, 201, 7),
QT_MOC_LITERAL(16, 209, 12),
QT_MOC_LITERAL(17, 222, 15)
    },
    "TestGraphRouting\0initTestCase\0\0loadGraph\0"
    "loadGraph_data\0getLoadedGraphs\0"
    "getLoadedGraphs_data\0nearestNeighbor\0"
    "nearestNeighbor_data\0route\0route_data\0"
    "unloadGraphs\0unloadGraphs_data\0listMaps\0"
    "listMaps_data\0mapInfo\0mapInfo_data\0"
    "cleanupTestCase\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TestGraphRouting[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   94,    2, 0x08,
       3,    0,   95,    2, 0x08,
       4,    0,   96,    2, 0x08,
       5,    0,   97,    2, 0x08,
       6,    0,   98,    2, 0x08,
       7,    0,   99,    2, 0x08,
       8,    0,  100,    2, 0x08,
       9,    0,  101,    2, 0x08,
      10,    0,  102,    2, 0x08,
      11,    0,  103,    2, 0x08,
      12,    0,  104,    2, 0x08,
      13,    0,  105,    2, 0x08,
      14,    0,  106,    2, 0x08,
      15,    0,  107,    2, 0x08,
      16,    0,  108,    2, 0x08,
      17,    0,  109,    2, 0x08,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void TestGraphRouting::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        TestGraphRouting *_t = static_cast<TestGraphRouting *>(_o);
        switch (_id) {
        case 0: _t->initTestCase(); break;
        case 1: _t->loadGraph(); break;
        case 2: _t->loadGraph_data(); break;
        case 3: _t->getLoadedGraphs(); break;
        case 4: _t->getLoadedGraphs_data(); break;
        case 5: _t->nearestNeighbor(); break;
        case 6: _t->nearestNeighbor_data(); break;
        case 7: _t->route(); break;
        case 8: _t->route_data(); break;
        case 9: _t->unloadGraphs(); break;
        case 10: _t->unloadGraphs_data(); break;
        case 11: _t->listMaps(); break;
        case 12: _t->listMaps_data(); break;
        case 13: _t->mapInfo(); break;
        case 14: _t->mapInfo_data(); break;
        case 15: _t->cleanupTestCase(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject TestGraphRouting::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_TestGraphRouting.data,
      qt_meta_data_TestGraphRouting,  qt_static_metacall, 0, 0}
};


const QMetaObject *TestGraphRouting::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TestGraphRouting::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_TestGraphRouting.stringdata))
        return static_cast<void*>(const_cast< TestGraphRouting*>(this));
    return QObject::qt_metacast(_clname);
}

int TestGraphRouting::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 16;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
