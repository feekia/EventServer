
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

#include "thread_pools.h"

namespace es {
// the constructor just launches some amount of workers
ThreadPool::ThreadPool(size_t threads) : size(threads), stop(false) {
    for (size_t i = 0; i < threads; ++i) {
        queue_mutex.emplace_back(new std::mutex());
        tasks.emplace_back(std::queue<std::function<void()>>());
        condition.emplace_back(new std::condition_variable());
        workers.emplace_back([this, i]() {
            std::function<void()> task;
            for (;;) {
                {
                    std::unique_lock<std::mutex> guard(*this->queue_mutex[i]);
                    this->condition[i]->wait(guard, [this, i] { return this->stop || !this->tasks[i].empty(); });
                    if (this->stop && this->tasks[i].empty()) return;
                    task = std::move(this->tasks[i].front());
                    this->tasks[i].pop();
                }
                task();
            }
        });
    }
}

// add new work item to the pool
bool ThreadPool::enqueue(std::function<void()> &&task, int idx) {
    std::unique_lock<std::mutex> guard(*queue_mutex[idx]);
    tasks[idx].emplace(task);
    condition[idx]->notify_one();
    return true;
}

// the destructor joins all threads
ThreadPool::~ThreadPool() {
    for (size_t i = 0; i < size; i++) {
        stop = true;
        condition[i]->notify_all();
        workers[i].join();
        delete condition[i];
        condition[i] = nullptr;
        delete queue_mutex[i];
        queue_mutex[i] = nullptr;
    }
}

} // namespace es
