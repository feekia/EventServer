#pragma once
#include <chrono>
#include <mutex>
namespace es {
class RealLock {
    using MS = std::chrono::milliseconds;

private:
    /* data */
    std::timed_mutex mtx;

public:
    RealLock(/* args */) {}
    ~RealLock() {}
    bool try_lock(int timeoutms) { return mtx.try_lock_for(MS(timeoutms)); }
    void lock() { mtx.lock(); }
    void unlock() { mtx.unlock(); }
};

} // namespace es
