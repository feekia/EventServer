
#ifndef _POOL_H
#define _POOL_H
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>

class threadpools {
public:
    threadpools(size_t);
    bool enqueue(std::function<void()>&&, int idx = 0);
    ~threadpools();
    size_t getSize(){
        return size;
    }
private:
    size_t size;
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;
    // the task queue
    std::vector< std::queue<std::function<void()>>> tasks;
    // synchronization
    std::vector<std::mutex *> queue_mutex;
    std::vector< std::condition_variable *> condition;
    bool stop;
};
#endif
