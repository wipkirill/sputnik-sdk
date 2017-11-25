// JsonXMLFormatter.cpp
//
#include <UrbanLabs/Sdk/Output/JsonXMLFormatter.h>

using namespace std;

/**
 * @brief FormatterNode::FormatterNode
 */
FormatterNode::FormatterNode()
    : key_(""), value_(""), level_(0), recursive_(false) {
    ;
}
/**
 * @brief FormatterNode::FormatterNode
 * @param key
 */
FormatterNode::FormatterNode(std::string key)
    : key_(key), value_(""), level_(0), recursive_(false) {
    ;
}
/**
 * @brief FormatterNode::~FormatterNode
 */
FormatterNode::~FormatterNode(){
    ;
}
/**
 * @brief FormatterNode::add
 * @param nodes
 */
void FormatterNode::add(std::vector <FormatterNode> nodes) {
    for(int i = 0; i < nodes.size(); i++) {
        nodes[i].level_ = level_+1;
    }
    nodes_.insert(nodes_.end(), nodes.begin(), nodes.end());
    recursive_ = true;
}
/**
 *
 */
template <>
std::string FormatterNode::escape <std::string> (std::string str) const {
    return "\""+str+"\"";
}
/**
 *
 */
template <>
std::string FormatterNode::escape <char *> (char *str) const {
    return "\""+std::string(str)+"\"";
}
/**
 *
 */
template <>
FormatterNode::FormatterNode(std::string key, FormatterNode value)
    : key_(key), level_(0), recursive_(false) {
    value_ = "{"+escape(value)+"}";
    nodes_.clear();
}
/**
 * @brief JSONFormatterNode::JSONFormatterNode
 */
JSONFormatterNode::JSONFormatterNode() {
    this->key_ = "";
    this->value_ = "";
    this->level_ = 0;
    this->recursive_ = false;
}
/**
 * @brief JSONFormatterNode::JSONFormatterNode
 * @param key
 */
JSONFormatterNode::JSONFormatterNode(std::string key) {
    this->key_ = key;
    this->value_ = "";
    this->level_ = 0;
    this->recursive_ = false;
}
/**
 * @brief JSONFormatterNode::add
 * @param nodes
 */
void JSONFormatterNode::add(std::vector <JSONFormatterNode> nds) {
    nodes_.insert(nodes_.end(), nds.begin(), nds.end());

    int ndSz = nodes_.size();
    for(int i = (ndSz > 0 ? ndSz-1 : 0), c = ndSz; c > 0; c--, i--) {
        nodes_[i].level_ = level_+1;
    }
    recursive_ = true;
}
/**
 * @brief JSONFormatterNode::toString
 * @return
 */
std::string JSONFormatterNode::toString() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}
/**
 * @brief JSONFormatterNode::nodes
 * @return
 */
std::vector <JSONFormatterNode> JSONFormatterNode::nodes() {
    return nodes_;
}
/**
 * @brief XMLFormatterNode::XMLFormatterNode
 */
XMLFormatterNode::XMLFormatterNode() {
    this->key_ = "";
    this->value_ = "";
    this->level_ = 0;
    this->recursive_ = false;
}
/**
 * @brief XMLFormatterNode::XMLFormatterNode
 * @param key
 */
XMLFormatterNode::XMLFormatterNode(std::string key) {
    this->key_ = key;
    this->value_ = "";
    this->level_ = 0;
    this->recursive_ = false;
}
/**
 * @brief XMLFormatterNode::openRecursive
 * @param key
 * @return
 */
std::string XMLFormatterNode::openRecursive(const std::string &key) const {
    if(key == "")
        return "<root>";
    return "<"+key+">";
}
/**
 * @brief XMLFormatterNode::closeRecursive
 * @param key
 * @return
 */
std::string XMLFormatterNode::closeRecursive(const std::string &key) const {
    if(key == "")
        return "</root>";
    return "</"+key+">";
}
/**
 * @brief XMLFormatterNode::openNonRecursive
 * @param key
 * @return
 */
std::string XMLFormatterNode::openNonRecursive(const std::string &key) const {
    return "<"+key+">";
}
/**
 * @brief XMLFormatterNode::closeNonRecursive
 * @param key
 * @return
 */
std::string XMLFormatterNode::closeNonRecursive(const std::string &key) const {
    return "</"+key+">";
}
/**
 * @brief XMLFormatterNode::separateValues
 * @return
 */
std::string XMLFormatterNode::separateValues() const {
    return "";
}
/**
 * @brief XMLFormatterNode::separateNodes
 * @return
 */
std::string XMLFormatterNode::separateNodes() const {
    return "";
}
/**
 * @brief XMLFormatterNode::add
 * @param nodes
 */
void XMLFormatterNode::add(std::vector <XMLFormatterNode> nodes) {
    for(int i = 0; i < nodes.size(); i++) {
        nodes[i].level_ = level_+1;
    }
    nodes_.insert(nodes_.end(), nodes.begin(), nodes.end());
    recursive_ = true;
}
/**
 *
 */
template <>
XMLFormatterNode::XMLFormatterNode(std::string key, XMLFormatterNode value) {
    this->key_ = key;
    this->level_ = 0;
    this->recursive_ = false;
    value_ = escape(value);
    nodes_.clear();
}
#undef LZZ_INLINE
