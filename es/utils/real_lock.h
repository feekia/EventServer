#pragma once
#include <mutex>
namespace es {
class RealLock {
private:
    /* data */
    std::mutex                   mtx;
    std::unique_lock<std::mutex> lk;

public:
    RealLock(/* args */) { lk = {mtx, std::defer_lock}; }
    ~RealLock() {}
    bool try_lock(){return lk.try_lock();}
    void lock() { lk.lock(); }
    void unlock() { lk.unlock(); }
};

} // namespace es
