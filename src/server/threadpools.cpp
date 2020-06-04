
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

#include "threadpools.h"

// the constructor just launches some amount of workers
threadpools::threadpools(size_t threads) : stop(false)
{
    for (size_t i = 0; i < threads; ++i)
    {
        workers.emplace_back(
            [this]() {
                for (;;)
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock,
                                         [this] { return this->stop || !this->tasks.empty(); });
                    if (this->stop && this->tasks.empty())
                        return;
                    auto task = std::move(this->tasks.front());
                    this->tasks.pop();
                    task();
                }
            });
    }
}

// add new work item to the pool
bool threadpools::enqueue(std::function<void()> task)
{
    std::unique_lock<std::mutex> lock(queue_mutex);
    tasks.emplace(task);
    condition.notify_one();
    return true;
}

// the destructor joins all threads
threadpools::~threadpools()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers)
        worker.join();
}
