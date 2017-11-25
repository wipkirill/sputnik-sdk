#include <iostream>
#include <cassert>

#include "test_polyline_encoder.h"
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Utils/GPolyEncode.h>

using namespace std;

// random number from 0 to 1
double getRandom() {
    return rand() / (1.0*numeric_limits<decltype(rand())>::max());
}

void TestPolylineEncoder::test() {
    INIT_LOGGING(Logger::INFO);

    int tests = 1000;
    double minLat = -90.00;
    double maxLat = 90.00; 
    double minLon = -180.00;
    double maxLon = 180.00;     

    for(int t = 0; t < tests; t++) {
        int minLen = 1, maxLen = 100;
        int len = rand()%(maxLen-minLen+1)+minLen;

        vector<Point> line;
        for(int i = 0; i < len; i++) {
            double lat = minLat + (double)(getRandom() * (maxLat - minLat));
            double lon = minLon + (double)(getRandom() * (maxLon - minLon));
            assert(minLat <= lat && lat <= maxLat && minLon <= lon && lon <= maxLon);
            line.emplace_back(lat, lon);
        }

        string encoded = GPolyEncoder::encodePoints(line);
        vector<Point> decoded = GPolyEncoder::decodePoints(encoded.c_str(), encoded.size());

        QCOMPARE((int)line.size(), (int)decoded.size());

        for(size_t i = 0; i < line.size(); i++) {
            QVERIFY(fabs(line[i].lat() - decoded[i].lat()) < GPolyEncoder::PRECISION &&
                    fabs(line[i].lon() - decoded[i].lon()) < GPolyEncoder::PRECISION);
        }
    }
    
    // polygons
    for(int t = 0; t < tests; t++) {
        vector<vector<Point> > polygon;
        int minRings = 1, maxRings = 100;
        int rings = minRings+rand()%(maxRings-minRings+1);
        for(int r = 0; r < rings; r++) {
            int minLen = 1, maxLen = 100;
            int len = rand()%(maxLen-minLen+1)+minLen;
            vector<Point> line;
            for(int i = 0; i < len; i++) {
                double lat = minLat + (double)(getRandom() * (maxLat - minLat));
                double lon = minLon + (double)(getRandom() * (maxLon - minLon));
                assert(minLat <= lat && lat <= maxLat && minLon <= lon && lon <= maxLon);
                line.emplace_back(lat, lon);
            }
            line.emplace_back(line[0]);
            polygon.emplace_back(line);
        }

        string encoded = GPolyEncoder::encodePoints(polygon);
        vector<vector<Point> > decoded = GPolyEncoder::decodePoints2(encoded.c_str(), encoded.size());

        QCOMPARE((int)polygon.size(), (int)decoded.size());

        for(size_t r = 0; r < polygon.size(); ++r) {
            QCOMPARE((int)polygon[r].size(), (int)decoded[r].size());
            for(size_t i = 0; i < polygon[r].size(); i++) {
                QVERIFY(fabs(polygon[r][i].lat() - decoded[r][i].lat()) < GPolyEncoder::PRECISION &&
                        fabs(polygon[r][i].lon() - decoded[r][i].lon()) < GPolyEncoder::PRECISION);
            }
        }
    }
}