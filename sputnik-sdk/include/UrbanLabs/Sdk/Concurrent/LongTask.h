#pragma once

// do not change the order here, ANDROID should come before __linux
#ifdef _WIN64
//define something for Windows (64-bit)
#elif _WIN32
//define something for Windows (32-bit)
#elif __APPLE__
//#include <curl/curl.h>
//#include <curl/easy.h>
#ifdef TARGET_OS_IPHONE
// iOS
#elif TARGET_IPHONE_SIMULATOR
// iOS Simulator
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
// Unsupported platform
#endif
#elif ANDROID
//#include <curl/curl.h>
//#include <curl/easy.h>
#elif __linux
//#include <curl/curl.h>
//#include <curl/easy.h>
#elif __unix // all unices not caught above
// Unix
#elif __posix
// POSIX
#endif

#include <unordered_map>
#include <string>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/URL.h>

class ThreadPool {
public:
    ThreadPool(size_t);
    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;
    ~ThreadPool();

private:
    // need to keep track of threads so we can join them
    std::vector<std::thread> workers;
    // the task queue
    std::queue<std::function<void()> > tasks;

    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads)
    : stop(false)
{
    for (size_t i = 0; i < threads; ++i)
        workers.emplace_back(
            [this] {
                while(true)
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    while(!this->stop && this->tasks.empty())
                        this->condition.wait(lock);
                    if(this->stop && this->tasks.empty())
                        return;
                    std::function<void()> task(this->tasks.front());
                    this->tasks.pop();
                    lock.unlock();
                    task();
                }
            });
}

// add new work item to the pool
template <class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    typedef typename std::result_of<F(Args...)>::type return_type;

    // don't allow enqueueing after stopping the pool
    if (stop)
        throw std::runtime_error("enqueue on stopped ThreadPool");

    auto task = std::make_shared<std::packaged_task<return_type()> >(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.push([task]() { (*task)(); });
    }
    condition.notify_one();
    return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (size_t i = 0; i < workers.size(); ++i)
        workers[i].join();
}
/**
 * @brief The TaskStatus class
 */
class TaskStatus {
public:
    enum State {
        UNKNOWN,
        INITIALIZING,
        RUNNING,
        SUCCESS,
        FAILED
    };

private:
    int taskId_;
    State taskState_;
    double completePercent_;
    std::string errMessage_;

public:
    TaskStatus(int id)
        : taskId_(id)
        , taskState_(INITIALIZING)
        , completePercent_(0.0)
        , errMessage_()
    {
        ;
    }
    State getState() const
    {
        return taskState_;
    }
    double getPercentage() const
    {
        return completePercent_;
    }
    void setState(State s)
    {
        taskState_ = s;
    }
    void setPercentage(double p)
    {
        completePercent_ = p;
    }
    void setErrorMessage(std::string& message)
    {
        errMessage_ = message;
    }
    std::string getErrorMessage() const
    {
        return errMessage_;
    }
};

//#ifndef ANDROID
///**
 //* @brief The DownloadTask class
 //*/
//class DownloadTask {
//private:
    //static int xferinfo(void* p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t);
    //static int older_progress(void* p, double dltotal, double dlnow, double ultotal, double ulnow);
    //static size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream);
//
//public:
    //static void run(TaskStatus& progress, const URL& downloadUrl, const URL& outputUrl);
//};
//#endif

class DecompressTask {
public:
    static void run(TaskStatus& progress, const URL& inputUrl, const URL& outputUrl);
};

class LongTaskService {
private:
    std::unordered_map<int, TaskStatus*> tasks_;
    std::mutex taskMutex_;
    ThreadPool tPool_;

public:
    LongTaskService()
        : tasks_()
        , tPool_(2)
    {
        ;
    }
    /**
     *
     */
    template <class F, class... Args>
    int submit(F f, Args&... args)
    {
        std::lock_guard<std::mutex> lk(taskMutex_);
        time_t now = time(0);
        TaskStatus* progress = new TaskStatus(now);
        tasks_.insert({ now, progress });
        tPool_.enqueue(f, std::ref(*progress), args...);
        return now;
    }
    /**
     * @brief getPercentage
     * @param taskId
     * @return
     */
    bool getState(int taskId, double& percent, TaskStatus::State& state, std::string& error)
    {
        if (tasks_.count(taskId) > 0) {
            percent = tasks_[taskId]->getPercentage();
            state = tasks_[taskId]->getState();
            error = tasks_[taskId]->getErrorMessage();
            return true;
        }
        return false;
    }
};
