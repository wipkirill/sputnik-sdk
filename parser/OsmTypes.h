#pragma once

#include <string>
#include <vector>

#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>

struct OSMTag
{
    std::string key;
    std::string value;
    void readOsmium(const osmium::Tag& tag) {
        key = tag.key();
        value = tag.value();
    }
};

struct OSMNode
{
    Vertex::VertexId nID;
    double       dfLat;
    double       dfLon;
    unsigned int nTags;
    std::vector<OSMTag> tags;
    void readOsmium(osmium::Node& osmiumNode) {
        nID = osmiumNode.id();
        dfLat = osmiumNode.location().lat();
        dfLon = osmiumNode.location().lon();
        nTags = osmiumNode.tags().size();
        for (const osmium::Tag& tag : osmiumNode.tags()) {
            OSMTag osmTag;
            osmTag.readOsmium(tag);
            tags.push_back(osmTag);
        }
    }
};

struct OSMWay
{
    Vertex::VertexId nID;
    unsigned int nTags;
    unsigned int nRefs;
    std::vector<Vertex::VertexId> nodeRefs;
    std::vector<OSMTag> tags;
    void readOsmium(osmium::Way& osmiumWay) {
        nID = osmiumWay.id();
        nTags = osmiumWay.tags().size();
        for (const osmium::Tag& tag : osmiumWay.tags()) {
            OSMTag osmTag;
            osmTag.readOsmium(tag);
            tags.push_back(osmTag);
        }
        nRefs = osmiumWay.nodes().size();
        for (const osmium::NodeRef& nr : osmiumWay.nodes()) {
		    nodeRefs.push_back(nr.ref());
		}
    }
};

typedef enum
{
    MEMBER_NODE = 0,
    MEMBER_WAY = 1,
    MEMBER_RELATION = 2
} OSMMemberType;

struct OSMMember
{
    Vertex::VertexId nID;
    std::string role; 
    OSMMemberType  eType;
};

struct OSMRelation
{
    Vertex::VertexId nID;
    unsigned int nTags;
    unsigned int nMembers;
    std::vector<OSMTag> tags;
    std::vector<OSMMember> members;
    void readOsmium(osmium::Relation& osmiumRel) {
        nID = osmiumRel.id();
        nTags = osmiumRel.tags().size();
        for (const osmium::Tag& tag : osmiumRel.tags()) {
            OSMTag osmTag;
            osmTag.readOsmium(tag);
            tags.push_back(osmTag);
        }
        nMembers = osmiumRel.members().size();
        for (osmium::RelationMember& rm : osmiumRel.members()) {
        	OSMMember member;
        	member.nID = rm.ref();
        	member.role = rm.role();
		   
		    switch(rm.type()) {
		    	case osmium::item_type::node :
		    		member.eType = OSMMemberType::MEMBER_NODE;
		    		break;
		    	case osmium::item_type::way :
		    		member.eType = OSMMemberType::MEMBER_WAY;
		    		break;
		    	case osmium::item_type::relation :
		    		member.eType = OSMMemberType::MEMBER_RELATION;
		    		break;
		    }
		    members.push_back(member);
		}
    }
};