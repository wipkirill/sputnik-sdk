#pragma once

//
// This is an implementation of read/write lock
// Example:
//
// class Object : public ReadWriteLock {
// public:
//     void read() {
//         ReadWriteLock::ReadLock lock(this);
//         // do some reading (no modification of the class)
//         // if you modify the class -> you are fucked!
//         // ideally you should access only const methods here
//     }
//
//     void write() {
//         ReadWriteLock::WriteLock lock(this);
//         // do some exlusive writing (modification of the class)
//     }
// };

// int main() {
//     Object obj;
//     std::thread read(&Object::read, &obj);
//     std::thread write(&Object::write, &obj);
//     read.join();
//     write.join();
//     return 0;
// }
//
// no further synchronization is necessary

#include <mutex>
#include <atomic>
#include <thread>
#include <cassert>
#include <condition_variable>

namespace detail {
class WriteScopeGuard;
class ReadScopeGuard;
} // namespace detail

class ReadWriteLock {
    friend class detail::ReadScopeGuard;
    friend class detail::WriteScopeGuard;

private:
    mutable std::mutex requestLock_;
    std::atomic<int> numRequests_;
    std::condition_variable hasNoRequests_;

    std::function<void()> startRead_;
    std::function<void()> finishRead_;

    std::function<std::unique_lock<std::mutex>()> startWrite_;
    std::function<void(std::unique_lock<std::mutex>&&)> finishWrite_;

private:
    void init();

public:
    typedef decltype(startRead_) StartRead;
    typedef decltype(finishRead_) FinishRead;
    typedef decltype(startWrite_) StartWrite;
    typedef decltype(finishWrite_) FinishWrite;

    typedef detail::ReadScopeGuard ReadLock;
    typedef detail::WriteScopeGuard WriteLock;

public:
    ReadWriteLock();
    ReadWriteLock(const ReadWriteLock &lock);
    ReadWriteLock(ReadWriteLock&& lock);

public:
    ReadWriteLock& operator = (const ReadWriteLock& lock) = delete;
    ReadWriteLock&& operator = (ReadWriteLock&& lock) = delete;
};

namespace detail {
class WriteScopeGuard {
public:
    WriteScopeGuard(ReadWriteLock* lock);
    ~WriteScopeGuard();
    void release();

private:
    typename ReadWriteLock::StartWrite start_;
    typename ReadWriteLock::FinishWrite finish_;
    bool engaged_;
    std::unique_lock<std::mutex> result_;
};

class ReadScopeGuard {
public:
    ReadScopeGuard(ReadWriteLock* lock);
    ~ReadScopeGuard();
    void release();

private:
    typename ReadWriteLock::StartRead start_;
    typename ReadWriteLock::FinishRead finish_;
    bool engaged_;
};
} // namespace detail