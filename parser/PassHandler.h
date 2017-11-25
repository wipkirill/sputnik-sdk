#pragma once

#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <stddef.h>
#include <sqlite3.h>
#include <google/sparse_hash_map>
#include <google/sparse_hash_set>

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include "ParserPlugin.h"

using std::hash;
using std::reverse;
using std::vector;
using std::string;
using std::unique_ptr;

#define VP_SQLITE
// parsing graph with osmium
class PassHandler : public Plugin {
private:
    // file for storing vertex and edge info
    string outputFileName_;
    vector<unique_ptr<Plugin> > plugins_;
    int currPassId_;
public:
public:
    PassHandler();
    PassHandler(const PassHandler&);
    PassHandler& operator=(const PassHandler&){return *this;}
    virtual ~PassHandler();
    PassHandler(const string& outputFile);
public:
    template <typename P>
    void registerPlugin(std::unique_ptr<P> plugin) {
        plugins_.push_back(std::move(plugin));
    }
    int passNum() const;
    virtual void init();
    virtual void notifyPassNumber(const int currPassId);
    virtual void notifyNode(OSMNode* n);
    virtual void notifyWay(OSMWay* w);
    virtual void notifyRelation(OSMRelation* rel);
    virtual void finalize();
    virtual void notifyEndParsing();
    virtual void afterImport();
};
