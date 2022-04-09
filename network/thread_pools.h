#pragma once

#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

namespace es {
class ThreadPool {
public:
    ThreadPool(size_t);
    ~ThreadPool() {
        exit = true;
        condition.notify_all();
        for (size_t i = 0; i < size; i++) {
            workers[i].join();
        }
    }

    void enqueue(std::function<void()> &&task) {
        {
            std::unique_lock<std::mutex> lk(mtx);
            tasks.emplace_back(task);
        }

        condition.notify_one();
    }

    void quit() { exit = true; }

private:
    size_t                           size;
    std::vector<std::thread>         workers;
    std::list<std::function<void()>> tasks;
    std::mutex                       mtx;
    std::condition_variable          condition;
    std::atomic<bool>                exit;
};

ThreadPool::ThreadPool(size_t threads) : size(threads), exit(false) {
    for (size_t i = 0; i < threads; ++i) {
        workers[i] = thread([this]() {
            while (!exit) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lk(mtx);
                    condition.wait(lk, [this] { return this->exit || !this->tasks.empty(); });
                    if (this->exit && this->tasks.empty()) return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task();
            }
        });
    }
}
} // namespace es
