#pragma once

#include "logging.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <inttypes.h>
#include <iostream>
#include <list>
#include <math.h>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

using namespace std;

namespace es {

struct IdleTaskPtrCompare;
class IdleTask;

using TimePoint_t      = std::chrono::steady_clock::time_point;
using Clock_t          = std::chrono::steady_clock;
using MillisDuration_t = std::chrono::milliseconds;
using IdleTaskPtr      = std::shared_ptr<IdleTask>;

using IdleTaskPtrQueue = priority_queue<IdleTaskPtr, deque<IdleTaskPtr>, IdleTaskPtrCompare>;

class IdleTask {
private:
    int64_t task_id_;
    bool    canceled_ = true;

    TimePoint_t nextRunTime_;
    int64_t     period_;

    bool        shouldUpdate_;
    TimePoint_t updateTime_;

    std::function<void(IdleTaskPtr &t)> run_;

    string name_;

    static int64_t asignTaskId() {
        static std::atomic_int64_t id;
        return id == INT64_MAX ? id = 0 : ++id;
    }

public:
    IdleTask(const string &n) : task_id_(asignTaskId()), canceled_(false), period_(0), shouldUpdate_(false), name_(n) {}
    IdleTask(string &&n)
        : task_id_(asignTaskId()), canceled_(false), period_(0), shouldUpdate_(false), name_(std::move(n)) {}

    IdleTask &operator=(const IdleTask &t) {
        if (&t == this) return *this;

        task_id_      = t.task_id_;
        canceled_     = t.canceled_;
        nextRunTime_  = t.nextRunTime_;
        shouldUpdate_ = t.shouldUpdate_;
        updateTime_   = t.updateTime_;
        return *this;
    }

    IdleTask &operator=(IdleTask &&t) {
        if (&t == this) return *this;

        task_id_      = t.task_id_;
        canceled_     = t.canceled_;
        nextRunTime_  = t.nextRunTime_;
        shouldUpdate_ = t.shouldUpdate_;
        updateTime_   = t.updateTime_;
        return *this;
    }

    ~IdleTask() {}

    void onRun(function<void(IdleTaskPtr &t)> &&callback) { run_ = std::move(callback); }
    void run(IdleTaskPtr &t) {
        if (run_ != nullptr) run_(t);
    }

    void cancel() {
        run_      = nullptr;
        canceled_ = true;
    }
    bool isCanceled() { return canceled_; }

    bool shouldUpdate() { return shouldUpdate_; }

    void updateIdleTime() {
        shouldUpdate_ = true;
        updateTime_   = Clock_t::now();
    }
    void setName(string &&n) { name_ = std::move(n); }
    void setName(const string &n) { name_ = n; }
    bool operator<(const IdleTask &t2) { return nextRunTime_ < t2.nextRunTime_; }
    bool operator>(const IdleTask &t2) { return nextRunTime_ > t2.nextRunTime_; }

    bool operator==(const IdleTask &t2) {
        if (task_id_ == t2.task_id_) return true;

        return false;
    }

    int64_t getTaskId() const { return task_id_; }

    void reload() {
        nextRunTime_  = updateTime_ + MillisDuration_t(period_);
        shouldUpdate_ = false;
    }

    const TimePoint_t &nextSchedTime() const { return nextRunTime_; }

    void setSchedDelay(int64_t delay) { nextRunTime_ = Clock_t::now() + MillisDuration_t(delay); }

    int64_t repeatPeriod() const { return period_; }
    void    setRepeatPeriod(int64_t pMs) { period_ = pMs; }
};

struct IdleTaskPtrCompare {
    bool operator()(const IdleTaskPtr &a, const IdleTaskPtr &b) { return *a > *b; }
};

} // namespace es