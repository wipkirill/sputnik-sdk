#pragma once

#include <string>
#include <google/sparse_hash_map>
#include <google/sparse_hash_set>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>
#include <UrbanLabs/Sdk/GraphCore/Edges.h>
#include <UrbanLabs/Sdk/Utils/Types.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include "OsmTypes.h"

/**
 * @brief die
 * @param pluginName
 * @param error
 */
#define die(pluginName, error) { \
    LOGG(Logger::ERROR) << "["<< pluginName<<"] " << error << Logger::FLUSH; \
    exit(EXIT_FAILURE); \
}

class OsmValidator {
public:
    typedef Vertex::VertexId VertexId;
    typedef google::sparse_hash_set<VertexId, std::hash<Vertex> > VertexSet;
public:
    virtual Edge::EdgeId maxEdgeId() const = 0;
    virtual Vertex::VertexId maxVertexId() const = 0;
    virtual bool isValidNode(OSMNode* node) const = 0;
    virtual bool isValidWay(OSMWay* way) const = 0;
    virtual bool isValidRelation(OSMRelation* relation) const = 0;
    virtual Vertex findVertex(VertexId id) const = 0;
    virtual ~OsmValidator(){}
};

class Plugin : public osmium::handler::Handler {
private:
    static std::string WORK_DIRECTORY;
protected:
    std::string pluginId_;
    OsmValidator *osmValidator_;
public:
    virtual void init(){}
    virtual void notifyNode(OSMNode* /*n*/){}
    virtual void notifyWay(OSMWay* /*w*/){}
    virtual void notifyRelation(OSMRelation* /*rel*/){}
    virtual void node(osmium::Node&);
    virtual void way(osmium::Way&);
    virtual void relation(osmium::Relation&);
    virtual void finalize(){}
    virtual void validate(){}
    virtual void notifyEndParsing(){}
    virtual void afterImport(){}
    virtual void cleanUp(){}
    virtual ~Plugin(){}
public:
    /**
     * @brief isValidWay
     * @param way
     * @return
     */
    virtual bool isValidWay(OSMWay* w);

    /**
     * Each plugin can subscribe for N parser passes
     * Default is 1 pass, when all of the methods
     * notifyNode(), notifyWay, notifyRelation
     * are called. Can be a situation when it should be done
     * N times.
     * @brief getPassNumber
     * @return
     */
    virtual int getPassNumber();
    /**
     * This method is called with id of current parser pass only if
     * a Plugin requires more than one round
     * @brief notifyPassNumber
     * @param currPassId
     */
    virtual void notifyPassNumber(const int /*currPassId*/);

    /**
     * @brief setOsmValidator
     * @param v
     */
    virtual void setOsmValidator(OsmValidator *v);
    /**
     * @brief getTableNamesToImport
     * @return
     */
    virtual std::vector<std::string> getTableNamesToImport() const;
    /**
     * @brief getSqlToCreateTables
     * @return
     */
    virtual std::vector<std::string> getSqlToCreateTables() const;
    /**
     * @brief getOtherSqlCommands
     * @return
     */
    virtual std::vector<std::string> getOtherSqlCommands() const;
    /**
     * @brief getFileNamesToRead is used to .read files from shell
     * this was done because sometimes we need to import BLOB
     * data which cannot be .imported from a CSV file
     * @return
     */
    virtual std::vector<std::string> getFileNamesToRead() const;
    /**
     * @brief Sets working directory to static variable
     * for global usage among all plugins
     * @param dirName
     */
    static void setWorkDir(const std::string &dirName);
    /**
     * @brief Read current working directory(backslashed) for plugin
     * Each plugin can override this method and set up its own path.
     * By default returns WORK_DIRECTORY
     * @return
     */
    virtual std::string getWorkDir() const;
};
