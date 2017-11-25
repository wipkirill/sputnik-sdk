#ifndef TEST_GRAPH_ROUTING_H
#define TEST_GRAPH_ROUTING_H

#include "AutoTest.h"
#include "util.h"

class TestGraphRouting : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void loadGraph();
    void loadGraph_data();
    void getLoadedGraphs();
    void getLoadedGraphs_data();
    void nearestNeighbor();
    void nearestNeighbor_data();
    void route();
    void route_data();
    void unloadGraphs();
    void unloadGraphs_data();
    void listMaps();
    void listMaps_data();
    void mapInfo();
    void mapInfo_data();
    void cleanupTestCase();
};

DECLARE_TEST(TestGraphRouting)

#endif // TEST_GRAPH_ROUTING_H