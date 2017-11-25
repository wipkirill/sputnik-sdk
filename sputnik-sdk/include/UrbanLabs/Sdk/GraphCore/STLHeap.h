#pragma once

#include <vector>
#include <utility>
#include <unordered_map>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/GraphCore/Heap.h>

#ifdef __linux
//#include <ext/pb_ds/priority_queue.hpp>//

//template<typename Key, typename Val>
//class STLHeap {
//public:
//    // The value type of the priority queue.
//    // The first entry is the node's id, and the second is the distance.
//    typedef std::pair<Key, Val> pq_value;//

//    // Comparison functor used to compare priority-queue value types.
//    struct pq_value_cmp : public std::binary_function<pq_value, pq_value, bool> {
//        inline bool
//        operator()(const pq_value& r_lhs, const pq_value& r_rhs) const {
//            // Note that a value is considered smaller than a different value
//            // if its distance is* larger*. This is because by STL
//            // conventions, "larger" entries are nearer the top of the
//            // priority queue.
//            return r_rhs.second < r_lhs.second;
//        }
//    };//

//    typedef __gnu_pbds::priority_queue< pq_value, pq_value_cmp> pq_t;//

//private:
//    size_t heapSize_;
//    // mapping to the positions inside the heap
//    std::unordered_map<Key, typename pq_t::point_iterator> a_it;
//    // The priority queue object.
//    pq_t p;
//public://

//    /**
//     * @brief STLHeap
//     */
//    STLHeap() : heapSize_(0) {
//        ;
//    }//

//    /**
//     * @brief empty
//     * @return
//     */
//    bool empty() {
//        return heapSize_ == 0;
//    }//

//    /**
//     * insert a new element into the heap
//     */
//    void pushHeap(Key key, Val val) {
//        heapSize_++;
//        a_it[key] = p.push(pq_value(key, val));
//    }//

//    /**
//     * decrease the value of an existing key in the heap
//     */
//    void decreaseKey(Key key, Val val) {
//        p.modify(a_it[key], pq_value(key, val));
//    }//

//    /**
//     * return top of the heap
//     */
//    std::pair<Key, Val> topHeap() {
//        return p.top();
//    }//

//    /**
//     * remove the top of the heap
//     */
//    void popHeap() {
//        heapSize_--;
//        p.pop();
//    }//

//    /**
//     * @brief initHeap
//     * @param elems
//     */
//    void initHeap(size_t elems) {
//        ;
//    }//

//    /**
//     * @brief initGoogle
//     */
//    void initGoogle() {
//        ;
//    }
//};//

//template <typename Key, typename Val, typename KeyPosMap = std::vector<unsigned> >
//struct STLHeapWrapper {
//    typedef STLHeap<Key, Val> type;
//};
//#else
//template <typename Key, typename Val, typename KeyPosMap>
//struct STLHeapWrapper {
//    typedef Heap<Key, Val, KeyPosMap> type;
//};
#endif
