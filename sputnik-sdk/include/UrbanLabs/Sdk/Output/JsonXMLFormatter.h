#pragma once

#include <string>
#include <vector>
#include <ostream>
#include <iomanip>
#include <sstream>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>

/**
 * @brief The FormatterNode class
 */
class FormatterNode {
protected:
    std::string key_;
    std::string value_;
    std::vector <FormatterNode> nodes_;
public:
    int level_;
    bool recursive_;
public:
    FormatterNode();
    virtual ~FormatterNode();
    FormatterNode(std::string key);
    template <typename T>
    FormatterNode(std::string key, T value);
    template <typename T>
    std::string escape(T t) const;
    template <typename TT = void *>
    std::string openRecursive(const std::string &key) const;
    template <typename TT = void *>
    std::string closeRecursive(const std::string &key) const;
    template <typename TT = void *>
    std::string openNonRecursive(const std::string &key) const;
    template <typename TT = void *>
    std::string closeNonRecursive(const std::string &key) const;
    template <typename TT = void *>
    std::string separateValues() const;
    template <typename TT = void *>
    std::string separateNodes() const;
    template <typename T>
    std::ostream &streamValues(std::ostream &oss, T values) const;
    template <typename T>
    std::ostream &streamValues(std::ostream &oss, std::vector <T> values) const;
    void add(std::vector <FormatterNode> nodes);
    template <typename TT = void *>
    friend std::ostream &operator << (std::ostream &oss, const FormatterNode &nd) {
        if(!nd.recursive_) {
            oss << nd.openNonRecursive(nd.key_);
            nd.streamValues(oss, nd.value_);
            oss << std::endl;
            oss << nd.closeNonRecursive(nd.key_);
        } else {
            oss << nd.openRecursive(nd.key_);
            for(int i = 0; i < (int)nd.nodes_.size(); i++) {
                oss  << nd.nodes_[i];
                if(i+1 != (int)nd.nodes_.size()) oss << nd.separateNodes();
            }
            oss << nd.closeRecursive(nd.key_);
        }
        return oss;
    }
};

// DO NOT TOUCH THIS, IT WILL BREAK RELEASE BUILD!!!!!
// http://stackoverflow.com/questions/16392885/why-is-my-specialized-template-function-invoked-only-in-debug-builds
template <>
std::string FormatterNode::escape <std::string> (std::string str) const;
template <>
std::string FormatterNode::escape <char *> (char *str) const;

/**
 * @brief The JSONFormatterNode class
 */
class JSONFormatterNode : public FormatterNode {
public:
    typedef JSONFormatterNode Node;
    typedef std::vector <Node> Nodes;
private:
    std::vector <JSONFormatterNode> nodes_;
public:
    JSONFormatterNode();
    JSONFormatterNode(std::string key);
    template <typename T>
    JSONFormatterNode(std::string key, T value);
    void add(std::vector <JSONFormatterNode> nodes);
    friend std::ostream &operator << (std::ostream &oss, const JSONFormatterNode &nd) {
        if(!nd.recursive_) {
            oss << nd.openNonRecursive(nd.key_);
            nd.streamValues(oss, nd.value_);
            oss << std::endl;
            oss << nd.closeNonRecursive(nd.key_);
        } else {
            oss << nd.openRecursive(nd.key_);
            for(int i = 0; i < (int)nd.nodes_.size(); i++) {
                oss  << nd.nodes_[i];
                if(i+1 != (int)nd.nodes_.size()) oss << nd.separateNodes();
            }
            oss << nd.closeRecursive(nd.key_);
        }
        return oss;
    }
    std::string toString() const;
    std::vector <JSONFormatterNode> nodes();
};
/**
 * @brief The XMLFormatterNode class
 */
class XMLFormatterNode : public FormatterNode {
public:
    std::vector <XMLFormatterNode> nodes_;
public:
    XMLFormatterNode();
    XMLFormatterNode(std::string key);
    template <typename T>
    XMLFormatterNode(std::string key, T value);
    template <typename T>
    std::string escape(T t);
    std::string openRecursive(const std::string &key) const;
    std::string closeRecursive(const std::string &key) const;
    std::string openNonRecursive(const std::string &key) const;
    std::string closeNonRecursive(const std::string &key) const;
    std::string separateValues() const;
    std::string separateNodes() const;
    template <typename T>
    std::ostream &streamValues(std::ostream &oss, T values) const;
    template <typename T>
    std::ostream &streamValues(std::ostream &oss, std::vector <T> values) const;
    void add(std::vector <XMLFormatterNode> nodes);
    friend std::ostream &operator << (std::ostream &oss, const XMLFormatterNode &nd) {
        if(!nd.recursive_) {
            oss << nd.openNonRecursive(nd.key_);
            nd.streamValues(oss, nd.value_);
            oss << std::endl;
            oss << nd.closeNonRecursive(nd.key_);
        } else {
            oss << nd.openRecursive(nd.key_);
            for(int i = 0; i < (int)nd.nodes_.size(); i++) {
                oss  << nd.nodes_[i];
                if(i+1 != (int)nd.nodes_.size()) oss << nd.separateNodes();
            }
            oss << nd.closeRecursive(nd.key_);
        }
        return oss;
    }
};
/**
 *
 */
template <>
XMLFormatterNode::XMLFormatterNode(std::string key, XMLFormatterNode value);
template <typename T>
FormatterNode::FormatterNode(std::string key, T value)
    : key_(key), value_(escape(value)), level_(0), recursive_(false) {
    ;
}
/**
 *
 */
template <typename TT>
std::string FormatterNode::openRecursive(const std::string &key) const {
    if(key != "")
        return escape(key)+":{";
    return "{";
}
/**
 *
 */
template <typename TT>
std::string FormatterNode::closeRecursive(const std::string &/*key*/) const {
    return "}";
}
/**
 *
 */
template <typename TT>
std::string FormatterNode::openNonRecursive(const std::string &key) const {
    return escape(key)+":";
}
/**
 *
 */
template <typename TT>
std::string FormatterNode::closeNonRecursive(const std::string &/*key*/) const {
    return " ";
}
/**
 *
 */
template <typename TT>
std::string FormatterNode::separateValues() const {
    return ",";
}
/**
 *
 */
template <typename TT>
std::string FormatterNode::separateNodes() const {
    return ",";
}
/**
 *
 */
template <typename T>
std::ostream &FormatterNode::streamValues(std::ostream &oss, T values) const {
    oss << values;
    return oss;
}
/**
 *
 */
template <typename T>
std::ostream &FormatterNode::streamValues(std::ostream &oss, std::vector <T> values) const {
    oss << "[";
    for(int i = 0; i < (int)values.size(); i++) {
        oss << escape(values[i]);
        if(i+1 != (int)values.size()) oss << separateValues();
    }
    oss << "]";
    return oss;
}
/**
 *
 */
template <typename T>
std::string FormatterNode::escape(T t) const {
    std::stringstream ss;
    ss << std::setprecision(9);
    streamValues(ss, t);
    return ss.str();
}
/**
 *
 */
template <typename T>
JSONFormatterNode::JSONFormatterNode(std::string key, T value) {
    this->key_ = key;
    this->value_ = escape(value);
    this->level_ = 0;
    this->recursive_ = false;
}
/**
 *
 */
template <typename T>
XMLFormatterNode::XMLFormatterNode(std::string key, T value) {
    this->key_ = key;
    this->value_ = escape(value);
    this->level_ = 0;
    this->recursive_ = false;
}
/**
 *
 */
template <typename T>
std::string XMLFormatterNode::escape(T t) {
    std::stringstream ss;
    ss << std::setprecision(9);
    streamValues(ss, t);
    return ss.str();
}
template <typename T>
std::ostream &XMLFormatterNode::streamValues(std::ostream &oss, T values) const {
    oss << "<v>" << values << "</v>";
    return oss;
}
/**
 *
 */
template <typename T>
std::ostream &XMLFormatterNode::streamValues(std::ostream &oss, std::vector <T> values) const {
    oss << "<vs>";
    for(int i = 0; i < values.size(); i++) {
        oss << escape(values[i]);
        if(i+1 != values.size()) oss << separateValues();
    }
    oss << "</vs>";
    return oss;
}
