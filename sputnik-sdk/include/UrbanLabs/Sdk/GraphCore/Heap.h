#pragma once

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/Timer.h>

#include <iostream>

//#define HEAPDEBUG
//#define HEAP_USAGE_STATISTICS

template<typename Key, typename Val, typename KeyPosMap = std::vector<unsigned> >
class Heap {
private:
    /**
     * @brief The HeapEl class
     */
    class HeapEl {
    public:
        Key key;
        Val val;

        HeapEl() {
            ;
        }

        HeapEl(const Key &k, const Val &v) {
            key = k;
            val = v;
        }
    };
public:
    size_t heapSize_;
    // key -> position map
    KeyPosMap keyPosMap_;
    // position key map
    std::vector<HeapEl> heap_;
    // signifies if an element is in heap
    std::vector<bool> inHeap_;
#ifdef HEAP_USAGE_STATISTICS
    // heap usage statistics
    static size_t maxElems_;
    static size_t totalElements_;
#endif
public:
    /**
     * @brief Heap
     */
    Heap() {
        heapSize_ = 0;
#ifdef HEAP_USAGE_STATISTICS
        maxElems_ = 0;
        totalElements_ = 0;
#endif
    }

    ~Heap() {
#ifdef HEAP_USAGE_STATISTICS
        std::cout << "[HEAP DESTRUCTOR] max elements: " << maxElems_ << std::endl;
        std::cout << "[HEAP DESTRUCTOR] total elements: " << totalElements_ << std::endl;
#endif
    }

    /**
     * @brief getKey
     * @param pr
     * @return
     */
    Key getKey(const HeapEl&pr) {
        return pr.key;
    }

    /**
     * @brief getValue
     * @param pr
     * @return
     */
    Val getValue(const HeapEl&pr) {
        return pr.val;
    }

    /**
     * @brief setValue
     * @param val
     * @param pr
     */
    void setValue(const Val &val, HeapEl&pr) {
        pr.val = val;
    }

    /**
     * @brief setValue
     * @param val
     * @param pr
     */
    void setKey(const Key &val, HeapEl&pr) {
        pr.key = val;
    }

    /**
     * @brief printHeap
     */
    void printHeap() {
        std::cout << "Heap size: " << heapSize_ << std::endl;
        for(size_t i = 1; i <= heapSize_; i++) {
            std::cout << getKey(heap_[i]) << " " << getValue(heap_[i]) << std::endl;
        }
        std::cout << std::flush;
    }

    /**
     * @brief size
     * @return
     */
    size_t size() const {
        return heapSize_;
    }

    /**
     * @brief empty
     * @return
     */
    bool empty() const {
        return heapSize_ == 0;
    }

    /**
     * sets the size to 0
     */
    void setEmpty() {
        heapSize_ = 0;
    }

    /**
     * @brief topHeap
     * @return
     */
    std::pair<Key, Val> topHeap() const {
#ifdef HEAPDEBUG
        assert(heapSize_ != 0);
#endif
        return std::make_pair(heap_[1].key, heap_[1].val);
    }

    /**
     * @brief existsInHeap
     * @param key
     * @return
     */
    bool existsInHeap(Key key) {
        std::cout << "[ERROR] method existsInHeap not supported yet" << std::endl;
        abort();
        return inHeap_[key];
    }

    /**
     * TODO: think about better initialization
     *
     * @brief initHeap
     * @param elems
     */
    void initHeap(size_t elems) {
        abort();
        keyPosMap_.resize(elems);
        heap_.resize(2000);
    }

    /**
     * @brief initResizeHeap
     * @param elems
     */
    void initResizeHeap(size_t elems) {
        keyPosMap_.resize(elems);
        heap_.resize(2000);
    }

    /**
     * @brief initGoogle
     */
    void initGoogle() {
        keyPosMap_.set_empty_key((Key)-1);
        keyPosMap_.set_deleted_key((Key)-2);
        heap_.resize(200);
    }

    /**
     * TODO: think about better initialization
     *
     * @brief initHeap
     * @param elems
     */
    void initHeapLocal(size_t elems) {
        keyPosMap_.resize(elems);
        heap_.resize(10);
    }

    /**
     * remove the top of the heap
     */
    void popHeap() {
#ifdef HEAPDEBUG
        assert(verifyHeap());
#endif
        if(heapSize_ > 1) {
            keyPosMap_[getKey(heap_[heapSize_])] = 1;
            std::swap(heap_[1], heap_[heapSize_]);
        }
        heapSize_--;

        maxHeapify(1);
#ifdef HEAPDEBUG
        assert(verifyHeap());
#endif
    }

    /**
     * remove the top of the heap using Floyd's improvement
     */
    void popHeapFloyd() {
        unsigned i = 1;
        while(true) {
            unsigned left = i << 1, right = left+1, smallest = i;
            if(left <= heapSize_) {
                smallest = left;
            }
            if(right <= heapSize_ && getValue(heap_[left]) > getValue(heap_[right])) {
                smallest = right;
            }

            if(smallest != i) {
                // modify the iterators
                keyPosMap_[getKey(heap_[smallest])] = i;
                heap_[i] = heap_[smallest];
                i = smallest;
            } else
                break;
        }

        // if we created a hole
        if(i != heapSize_) {
            keyPosMap_[getKey(heap_[heapSize_])] = i;
            heap_[i] = heap_[heapSize_];
            heapSize_--;

            siftUp(i);
        } else {
            heapSize_--;
        }

#ifdef HEAPDEBUG
        if(!verifyHeap()) {
            printHeap();
            assert(false);
        }
#endif
    }

    /**
     * a more efficient version of pop and push at the same time
     */
    void popPushHeap(const Key &key, const Val &val) {
        setKey(key, heap_[1]);
        setValue(val, heap_[1]);
        keyPosMap_[key] = 1;
        maxHeapify(1);
    }

    /**
     * sift the element up while it is larger than its parents
     * TODO: cache the memory access values
     *
     * @brief siftUp
     * @param currPos
     */
    void siftUp(unsigned currPos) {
        Key currKey = getKey(heap_[currPos]);
        Val currValue = getValue(heap_[currPos]);

        while(currPos > 1) {
            unsigned parent = currPos >> 1;
            Key parentKey = getKey(heap_[parent]);
            Val parentValue = getValue(heap_[parent]);

            if(parentValue <= currValue)
                break;

#ifdef HEAPDEBUG
            assert(getKey(heap_[parent]) == parentKey);
            assert(getKey(heap_[currPos]) == currKey);
#endif

            std::swap(heap_[parent], heap_[currPos]);
            keyPosMap_[currKey] = parent;
            keyPosMap_[parentKey] = currPos;

            currPos = parent;
        }
#ifdef HEAPDEBUG
        assert(verifyHeap());
#endif
    }

    /**
     * insert a new element into the heap
     */
    void pushHeap(const Key &key, const Val &val) {
#ifdef HEAP_USAGE_STATISTICS
        totalElements_++;
        maxElems_ = max(maxElems_, heapSize_+1);
#endif
#ifdef HEAPDEBUG
        assert(verifyHeap());
#endif
        // insert the element
        // indexing starts at 1
        unsigned currPos = heapSize_+1;

        // increase the heap size
        heapSize_++;

        // resize appropriately, do not trust stl
        if(heapSize_+1 > heap_.capacity()) {
            //assert(false);
            heap_.resize(2*(heapSize_+1));
            //keyPosMap_.resize(2*(heapSize_+1));
        }

        // set the new key value
        heap_[currPos] = HeapEl(key, val);

        // set the keyPosMap_
        keyPosMap_[key] = currPos;

        // sift the element up
        siftUp(currPos);
#ifdef HEAPDEBUG
        assert(verifyHeap());
#endif
    }

    /**
     * decrease the value of an existing key in the heap
     *
     * @brief decreaseKey
     * @param key
     * @param val
     */
    void decreaseKey(const Key &key, const Val &val) {
#ifdef HEAPDEBUG
        assert(verifyHeap());
#endif
        unsigned currPos = keyPosMap_[key];

        // the node should be contained in the heap
#ifdef HEAPDEBUG
        assert(getKey(heap_[currPos]) == key);
#endif

        // initialize the value
        if(val < getValue(heap_[currPos])) {
            setValue(val, heap_[currPos]);

            // sift the element up
            siftUp(currPos);
        }
#ifdef HEAPDEBUG
        assert(verifyHeap());
#endif
    }

    /**
     * increase the value of an existing key in the heap
     * @brief decreaseKey
     * @param key
     * @param val
     */
    void increaseKey(const Key &key, const Val &val) {
#ifdef HEAPDEBUG
        assert(verifyHeap());
#endif
        unsigned currPos = keyPosMap_[key];

        // the node should be contained in the heap
#ifdef HEAPDEBUG
        assert(getKey(heap_[currPos]) == key);
#endif

        // initialize the value
        if(val > getValue(heap_[currPos])) {
            setValue(val, heap_[currPos]);

            // sift the element down
            maxHeapify(currPos);
        }
#ifdef HEAPDEBUG
        assert(verifyHeap());
#endif
    }

    /**
     * changes the value of an element in the heap
     *
     * @brief changeKey
     * @param key
     * @param val
     */
    void changeKey(const Key &key, const Val &val) {
#ifdef HEAPDEBUG
        assert(verifyHeap());
#endif
        unsigned currPos = keyPosMap_[key];

        // initialize the value
        if(val > getValue(heap_[currPos])) {
            increaseKey(key, val);
        } else if(val < getValue(heap_[currPos])) {
            decreaseKey(key, val);
        }
#ifdef HEAPDEBUG
        assert(verifyHeap());
#endif
    }

    /**
     * assuming the left and right are valid heaps, drag down the i element
     */
    void maxHeapify(unsigned i) {
        while(true) {
            unsigned smallest = i;
            unsigned left = i << 1, right = left+1;
            if(left <= heapSize_ && getValue(heap_[left]) < getValue(heap_[i])) {
                smallest = left;
            }

            if(right <= heapSize_ && getValue(heap_[right]) < getValue(heap_[smallest])) {
                smallest = right;
            }

            if(smallest != i) {
                // modify the iterators
                keyPosMap_[getKey(heap_[smallest])] = i;
                keyPosMap_[getKey(heap_[i])] = smallest;
                std::swap(heap_[smallest], heap_[i]);

                i = smallest;
            } else
                break;
        }
#ifdef HEAPDEBUG
        assert(verifyHeap());
#endif
    }

    /**
     * construct the heap
     */
    void buildHeap() {
        for(size_t i = 1+heapSize_/2; i >= 1; i--)
            maxHeapify(i);
    }

    /**
     * sort the heap
     */
    void heapSort() {
        buildHeap();

        for(size_t i = heapSize_; i >= 1; i--) {
            std::swap(heap_[1], heap_[i]);
            heapSize_--;
            maxHeapify(1);
        }
    }

    /**
     * @brief verifyHeap
     * @return
     */
    bool verifyHeap() {
        if(heapSize_ > 0)
            return verifyHeapFrom(1);
        else
            return true;
    }

    /**
     * @brief verifyHeapFrom
     * @param from
     * @return
     */
    bool verifyHeapFrom(unsigned from) {
        bool trueHeap = true;
        unsigned left = from << 1, right = left+1;

        assert(keyPosMap_[getKey(heap_[from])] == from);

        // verify the left side
        if(left <= heapSize_) {
            if(getValue(heap_[from]) <= getValue(heap_[left]))
                trueHeap &= verifyHeapFrom(left);
            else
                return false;
        }

        // verify the right side
        if(right <= heapSize_) {
            if(getValue(heap_[from]) <= getValue(heap_[right]))
                trueHeap &= verifyHeapFrom(right);
            else
                return false;
        }

        return trueHeap;
    }

    /**
     * @brief testHeap
     */
    static void test() {
#define HEAPDEBUG
        std::cout << "testing basic operations" << std::endl;
        Heap<int, double> heap;
        heap.initHeap(20);

        for(int i = 0; i < 10; i++)
            heap.pushHeap(i, i);

        heap.popHeap();
        heap.printHeap();

        heap.popHeap();
        heap.printHeap();

        heap.pushHeap(0, 11);
        heap.printHeap();

        heap.decreaseKey(0, 0);
        heap.printHeap();

        for(int i = 0; i < 9; i++)
            heap.popHeap();

        heap.printHeap();

        srand(time(NULL));

        std::vector<int> randomEls;
        for(int i = 0; i < 500000; i++) {
            randomEls.push_back(rand());
        }

        Timer t;

        // custom heap
        Heap<int, int> heapCustom;
        heapCustom.initHeap(500000);
        for(int i = 0; i < randomEls.size(); i++) {
            heapCustom.pushHeap(i, randomEls[i]);
        }
        for(int i = 0; i < randomEls.size(); i++) {
            heapCustom.popHeap();
        }
        t.stop();
        std::cout << "custom: " << t.getElapsedTimeSec() << std::endl;
#undef HEAPDEBUG
     }
};

#ifdef HEAP_USAGE_STATISTICS
template<typename Key, typename Value, typename KeyPosMap = vector<unsigned> >
size_t Heap<Key, Value>::maxElems_ = 0;

template<typename Key, typename Value, typename KeyPosMap = vector<unsigned> >
size_t Heap<Key, Value>::totalElements_ = 0;
#endif
