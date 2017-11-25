#include <algorithm>

#include "PassHandler.h"
/**
 * @brief PassHandler
 * @param outputFile
 * @param edgFilter
 */
PassHandler::PassHandler(const string& outputFile):
    Plugin(),outputFileName_(outputFile), plugins_(), currPassId_(0) {
}
/**
 *
 */
PassHandler::~PassHandler() {
    ;
}
/**
 *
 */
int PassHandler::passNum() const {
    int maxPassNum = 1;
    for(int i=0;i<(int)plugins_.size();++i)
        maxPassNum = std::max(maxPassNum,plugins_[i]->getPassNumber());
    return maxPassNum;
}
/**
 * @brief PassHandler::init
 */
void PassHandler::init(){
    if(currPassId_ > 1)
        return;
    for (int i = 0 ; i < (int)plugins_.size();++i) {
        plugins_[i]->init();
    }
}
/**
 * @brief PassHandler::notifyPassNumber
 * @param currPassId
 */
void PassHandler::notifyPassNumber(const int currPassId) {
    currPassId_ = currPassId;
    for (int i = 0 ; i < (int)plugins_.size();++i) {
        if(plugins_[i]->getPassNumber() >= currPassId)
            plugins_[i]->notifyPassNumber(currPassId);
    }
}
/**
 * @brief PassHandler::notifyNode
 * @param n
 */
void PassHandler::notifyNode(OSMNode* n) {
    for (int i = 0 ; i < (int)plugins_.size();++i) {
        if(plugins_[i]->getPassNumber() >= currPassId_)
            plugins_[i]->notifyNode(n);
    }
}
/**
 * @brief PassHandler::notifyWay
 * @param w
 */
void PassHandler::notifyWay(OSMWay* w) {
    for (int i = 0 ; i < (int)plugins_.size();++i) {
        if(plugins_[i]->getPassNumber() >= currPassId_)
            if (plugins_[i]->isValidWay(w))
                plugins_[i]->notifyWay(w);
    }
}
/**
 * @brief notifyRelation
 * @param rel
 */
void PassHandler::notifyRelation(OSMRelation* rel) {
    for (int i = 0 ; i < (int)plugins_.size();++i) {
        if(plugins_[i]->getPassNumber() >= currPassId_)
            plugins_[i]->notifyRelation(rel);
    }
}
/**
 * @brief PassHandler::finalize
 */
void PassHandler::finalize() {
    if(currPassId_ == 1)
        for (int i = 0 ; i < (int)plugins_.size();++i) {
            plugins_[i]->finalize();
        }
}
void PassHandler::notifyEndParsing() {
    for (int i = 0 ; i < (int)plugins_.size();++i) {
        plugins_[i]->notifyEndParsing();
    }
}

void PassHandler::afterImport() {
    LOGG(Logger::PROGRESS) << "Validation step" << Logger::FLUSH;
    // validation
    for(int i = 0 ; i < (int)plugins_.size();++i) {
        plugins_[i]->validate();
    }

    LOGG(Logger::PROGRESS) << "After import step" << Logger::FLUSH;
    // after import step
    for(int i = 0 ; i < (int)plugins_.size();++i) {
        plugins_[i]->afterImport();
    }

    LOGG(Logger::PROGRESS) << "Cleanup step" << Logger::FLUSH;
    // remove all temp text files
    for(int i = 0 ; i < (int)plugins_.size();++i) {
        plugins_[i]->cleanUp();
    }
}

