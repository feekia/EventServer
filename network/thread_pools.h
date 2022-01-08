
#ifndef _THREAD_POOLS_H_
#define _THREAD_POOLS_H_
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace es {
class ThreadPool {
public:
    ThreadPool(size_t);
    bool enqueue(std::function<void()> &&, int idx = 0);
    ~ThreadPool();
    size_t getSize() { return size; }

private:
    size_t size;
    // need to keep track of threads so we can join them
    std::vector<std::thread> workers;
    // the task queue
    std::vector<std::queue<std::function<void()>>> tasks;
    // synchronization
    std::vector<std::mutex *>              queue_mutex;
    std::vector<std::condition_variable *> condition;
    std::atomic<bool>                      stop;
};

} // namespace es
#endif
