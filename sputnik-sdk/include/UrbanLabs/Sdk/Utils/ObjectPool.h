#pragma once

#include <mutex>

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/ThreadSafe.h>

/**
 * @brief The GraphSelector class
 */
template<typename T, typename I, typename D>
class ObjectPool {
private:
    static const int MAX_OBJ = 10;
private:
    std::mutex lock_;
private:
    int sz_;
    std::queue <int> ids_;
    std::map <std::string, int> idMap_;
    T obj_[MAX_OBJ];
public:
    ObjectPool();
    bool addObj(const std::string &name, const I &init);
    bool existsObj(const std::string &name);
    bool removeObj(const std::string &name);
    T &getObj(const std::string &name);
    std::vector <std::string> getLoadedObj();
};

/**
 * @brief ObjectPool::ObjectPool
 */
template<typename T, typename I, typename D>
ObjectPool<T, I, D>::ObjectPool() : lock_(), sz_(0), ids_(), idMap_() {
    // TODO: initialize locking
}

/**
 * @brief ObjectPool::existsObj
 * @param name
 * @return
 */
template<typename T, typename I, typename D>
bool ObjectPool<T, I, D>::existsObj(const std::string &name) {
    lock_.lock();
    bool ret = idMap_.find(name) != idMap_.end();
    lock_.unlock();
    return ret;
}
/**
 * @brief ObjectPool::addObj
 * @param name
 * @return
 */
template<typename T, typename I, typename D>
bool ObjectPool<T, I, D>::addObj(const std::string &name, const I &init) {
    lock_.lock();
    // object already exists
    if(idMap_.find(name) != idMap_.end()) {
        LOGG(Logger::INFO) << "object already exists" << Logger::FLUSH;
        lock_.unlock();
        return false;
    }

    // get a new id for the object
    int id;
    if(ids_.empty()) {
        if(sz_ == MAX_OBJ) {
            lock_.unlock();
            return false;
        }
        id = sz_;
    } else {
        id = ids_.front();
        ids_.pop();
    }
    // try to parse object
    if(!init.init(obj_[id])) {
        lock_.unlock();
        return false;
    }

    idMap_[name] = id;
    if(ids_.empty())
        sz_++;

    lock_.unlock();
    return true;
}
/**
 * @brief ObjectPool::removeObj
 * @param name
 * @return
 */
template<typename T, typename I, typename D>
bool ObjectPool<T, I, D>::removeObj(const std::string &name) {
    lock_.lock();
    bool ret = false;
    if(idMap_.find(name) != idMap_.end()) {
        int id = idMap_[name];
        D::release(obj_[id]);
        idMap_.erase(name);
        ids_.push(id);
        ret = true;
    } else {
        LOGG(Logger::INFO) << "object not found" << Logger::FLUSH;
    }
    lock_.unlock();
    return ret;
}
/**
 * @brief ObjectPool::getOsmGraph
 * @param name
 * @return
 */
template<typename T, typename I, typename D>
T &ObjectPool<T, I, D>::getObj(const std::string &name) {
    return obj_[idMap_[name]];
}

/**
 * @brief ObjectPool::getLoadedOsm
 * @return
 */
template<typename T, typename I, typename D>
std::vector<std::string> ObjectPool<T, I, D>::getLoadedObj() {
    lock_.lock();
    std::vector<std::string> names;
    for(const auto pr : idMap_) {
        names.push_back(pr.first);
    }
    lock_.unlock();
    return names;
}
