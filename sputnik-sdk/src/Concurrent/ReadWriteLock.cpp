#include <UrbanLabs/Sdk/Concurrent/ReadWriteLock.h>

namespace detail {

WriteScopeGuard::WriteScopeGuard(ReadWriteLock* lock)
    : start_(lock->startWrite_)
    , finish_(lock->finishWrite_)
    , engaged_(true)
    , result_(std::move(start_()))
{
    ;
}

WriteScopeGuard::~WriteScopeGuard()
{
    if (engaged_) {
        std::bind(finish_, std::move(result_));
    }
}
void WriteScopeGuard::release()
{
    engaged_ = false;
}

ReadScopeGuard::ReadScopeGuard(ReadWriteLock* lock)
    : start_(lock->startRead_)
    , finish_(lock->finishRead_)
    , engaged_(true)
{
    start_();
}

ReadScopeGuard::~ReadScopeGuard()
{
    if (engaged_) {
        finish_();
    }
}
void ReadScopeGuard::release()
{
    engaged_ = false;
}

} // namespace detail

void ReadWriteLock::init()
{
    startRead_ = [this]() {
        std::lock_guard<std::mutex> guard(requestLock_);
        numRequests_++;
    };

    finishRead_ = [this]() {
        {
            std::lock_guard<std::mutex> guard(requestLock_);
            numRequests_--;
        }
        // notify all potential writers that read is over
        // notifying one will not work as he might not be able to aquire the lock
        hasNoRequests_.notify_all();
    };

    startWrite_ = [this]() {
        std::unique_lock<std::mutex> guard(requestLock_);
        hasNoRequests_.wait(guard, [this]() {return numRequests_ == 0; });
        assert(numRequests_ == 0);
        return guard;
    };

    finishWrite_ = [this](std::unique_lock<std::mutex>&& guard) {
        guard.unlock();
        // wake up only 1 writer
        hasNoRequests_.notify_one();
    };
}

ReadWriteLock::ReadWriteLock()
    : numRequests_(0)
{
    init();
}

ReadWriteLock::ReadWriteLock(const ReadWriteLock& lock)
    : numRequests_(0)
{
    init();
}

ReadWriteLock::ReadWriteLock(ReadWriteLock&& lock)
    : numRequests_(0)
{
    init();
}
