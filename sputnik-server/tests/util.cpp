#include "util.h"
using namespace std;

const string Sputnik::HEARTBEAT = "heartbeat";
const string Sputnik::LOADGRAPH = "graph/load";
const string Sputnik::GETLOADEDGRAPHS = "graph/list";
const string Sputnik::UNLOADGRAPHS = "graph/unload";
const string Sputnik::NEARESTNEIGHBOR = "graph/nearest";
const string Sputnik::ROUTE = "graph/route";
const string Sputnik::SEARCH = "search/query";
const string Sputnik::NEARESTOBJECT = "search/nearest";
const string Sputnik::LISTMAPS = "listmaps";
const string Sputnik::MAPINFO = "mapinfo";

#define COMPARE(actual, expected, message) \
do {\
    if (!QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__)) {\
        qDebug() << message;\
        return;\
    }\
} while (0)
/**
*
*/
bool NetworkUtil::request(const std::string &url, const QueryArgs &args, QString &result) {
      // create custom temporary event loop on stack
      QEventLoop eventLoop;

      // "quit()" the eventl-loop, when the network request "finished()"
      QNetworkAccessManager mgr;
      QObject::connect(&mgr, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));

      // the HTTP request
      QUrl rUrl("http://localhost:8080/" + QString::fromUtf8(url.c_str()));
      if(args.size() > 0) {
          QUrlQuery query;
          for(int i=0; i < args.size();++i) 
              query.addQueryItem(args[i].first, args[i].second);
          rUrl.setQuery(query);
      }

      QNetworkRequest req(rUrl);
      qDebug() << rUrl.toString();
      QNetworkReply *reply = mgr.get(req);
      eventLoop.exec(); // blocks stack until "finished()" has been called

      if (reply->error() == QNetworkReply::NoError) {
          QByteArray bytes = reply->readAll();  // bytes
          result = QString(bytes);
          return true;
      } 
      QTest::qFail("Couldn't connect to server", __FILE__, __LINE__);
      return false;
}
/**
*
*/
void Validator::extractResponse(const QString &output, bool fullObject, const QString &field, const QString &format, QVariantMap &parsed) {
    if(format == "json") {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8(), &err);
        if(err.error != QJsonParseError::NoError) {
            qDebug() << "Error while parsing JSON" << err.errorString() << output;
            QFAIL("Error while parsing JSON");
        }
        QVariantMap fullMap = doc.object().toVariantMap();
        if(fullObject) {
            parsed = fullMap;
        } else {
            parsed = fullMap[field].toMap();
        }
    } else {
        QString msg = "Output format " + format + " not implemented";
        QFAIL(msg.toStdString().c_str());
    }
}
/**
*
*/
void Validator::validate(const QVariantMap &expected, const QString &response, const QString &format) {
    if(format == "json") {
        QJsonObject jsExp = QJsonObject::fromVariantMap(expected);
        QJsonDocument exp(jsExp);
        QVariantMap actual;
        extractResponse(response, true, "", format, actual);
        // validate contents
        auto iter = expected.begin();
        for(; iter != expected.end(); ++iter) {
            if(actual.find(iter.key()) == actual.end()) {
                qDebug() << "No field " << iter.key() << "in response"; 
                QFAIL("Some field not found in response");
            }

            QString msg = "Values are different for key :" + iter.key() + "\n" +
                          "Response: " + response + "\n" + "Expected: " + exp.toJson();
            COMPARE(actual[iter.key()], expected[iter.key()], msg); 
        }
    } else {
        QString msg = "Output format " + format + " not implemented";
        QFAIL(msg.toStdString().c_str());
    }
}
/**
*
*/
void Validator::checkTagsInResponse(const QList<QVariant> &objectList, int objectId, const QVariantMap &tags) {
    if(objectId > objectList.size()-1)
        QFAIL("No such objectId in response field");

    for(int i = 0; i < objectList.size();++i) {
        if(objectId != -1 && i != objectId)
            continue;
        QVariantMap q = objectList.at(i).toMap();
        if(q.find("tags") == q.end())
            QFAIL("No tags field in the object");
        QVariantMap tagList = q["tags"].toMap();
        auto iter = tags.begin();
        for(; iter != tags.end(); ++iter) {
            if(tagList.find(iter.key()) == tagList.end()) {
                qDebug() << "No field " << iter.key() << "in response"; 
                QFAIL("Some field not found in tags");
            }
            QString msg = "Values are different for key :" + iter.key() + "\n" +
                              "Actual: " + tagList[iter.key()].toString() + "\n" + "Expected: " + iter.value().toString();
            if(iter.key() == "name") {
                if(tags[iter.key()].toString().indexOf(iter.value().toString()) == -1)
                    QFAIL(msg.toStdString().c_str());
            } else
                COMPARE(tagList[iter.key()], tags[iter.key()], msg); 
        }
    }
}