#pragma once
#include <atomic>
#include <functional>
#include <inttypes.h>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;
namespace es {
using WorkTask                   = function<void(int64_t waitms)>;
using ExitTask                   = function<void()>;
constexpr int kWaitLoopTimeoutMs = 10000;
class EventLoop {
private:
    atomic<bool> exit_;
    mutex        mtx;
    WorkTask     workTask_;
    ExitTask     exitTask_;
    ExitTask     cleanupTask_;

public:
    EventLoop() : exit_(false) {}
    ~EventLoop() {}
    void loop() {
        while (!exit_) {
            workTask_(kWaitLoopTimeoutMs);
        }
        if (exitTask_ != nullptr) {
            lock_guard<mutex> lk(mtx);
            cleanupTask_();
            cleanupTask_ = nullptr;
        }
    }
    void onWork(const WorkTask &workcb) { workTask_ = workcb; }
    void onExitNotify(const ExitTask &exitcb) { exitTask_ = exitcb; }
    void onClean(const ExitTask &cleancb) { cleanupTask_ = cleancb; }
    void exit(bool is_exit) {
        exit_ = is_exit;
        if (exitTask_ != nullptr) {
            lock_guard<mutex> lk(mtx);
            exitTask_();
            exitTask_ = nullptr;
        }
    }
};

} // namespace es
