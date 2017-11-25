#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>
#include <utility>

#include <QtNetwork/QNetworkAccessManager>
#include <QtCore/QUrl>
#include <QtCore/QEventLoop>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QUrlQuery>

#include "AutoTest.h"

#define TEST(url) \
do {\
    QString resp;\
    QFETCH(QueryArgs, query);\
    QFETCH(QVariantMap, expected);\
    QFETCH(QString, outputFormat);\
    if(NetworkUtil::request(url, query, resp))\
        Validator::validate(expected, resp, outputFormat);\
} while (0)


class NetworkUtil : public QObject {
	Q_OBJECT
public:
  static bool request(const std::string &url, const QueryArgs &args, QString &result);
};

class Sputnik {
public:
	const static std::string HEARTBEAT;
	const static std::string LOADGRAPH;
	const static std::string GETLOADEDGRAPHS;
	const static std::string UNLOADGRAPHS;
	const static std::string NEARESTNEIGHBOR;
	const static std::string ROUTE;
	const static std::string SEARCH;
	const static std::string NEARESTOBJECT;
	const static std::string LISTMAPS;
	const static std::string MAPINFO;
};


class Validator {
public:
	static void validate(const QVariantMap &expected, const QString &response, const QString &format);
	static void checkTagsInResponse(const QList<QVariant> &objectList, int objectId, const QVariantMap &tags);
	static void extractResponse(const QString &output, bool fullObject, const QString &field, const QString &format, QVariantMap &parsed);

};

#endif // UTIL_H