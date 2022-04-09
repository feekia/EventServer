#pragma once

#include "logging.h"
#include "es_util.h"
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
class Timer;
class TimerTask;

using TimerPtr         = std::shared_ptr<Timer>;
using TimePoint_t      = std::chrono::steady_clock::time_point;
using Clock_t          = std::chrono::steady_clock;
using MillisDuration_t = std::chrono::milliseconds;
using TimerTaskPtr     = std::shared_ptr<TimerTask>;

using TimerTaskQueue = priority_queue<TimerTaskPtr, deque<TimerTaskPtr>, SpGreater<TimerTaskPtr>>;

enum {
    VIRGIN = 0,

    /**
     * This task is scheduled for execution.  If it is a non-repeating task,
     * it has not yet been executed.
     */
    SCHEDULED = 1,

    /**
     * This non-repeating task has already executed (or is currently
     * executing) and has not been cancelled.
     */
    EXECUTED = 2,

    /**
     * This task has been cancelled (with a call to TimerTask.cancel).
     */
    CANCELLED = 3,
};

class TimerTask {
private:
    int64_t task_id;
    int32_t state = VIRGIN;

    /**
     * Next execution time for this task in the format returned by
     * , assuming this task is scheduled for execution.
     * For repeating tasks, this field is updated prior to each task execution.
     */
    TimePoint_t nextExecutionTime;

    /**
     * Period in milliseconds for repeating tasks.  A positive value indicates
     * fixed-rate execution.  A negative value indicates fixed-delay execution.
     * A value of 0 indicates a non-repeating task.
     */
    MillisDuration_t period;

    /**
     * This lock is used to control access to the TimerTask internals.
     */
    std::function<void(TimerTaskPtr &t)> run_;

    string name;

    static int64_t asignTaskId() {
        static std::atomic_int64_t id;
        return id == INT64_MAX ? id = 0 : ++id;
    }

public:
    TimerTask(string &&n) : task_id(asignTaskId()), state(VIRGIN), name(std::move(n)) {}
    TimerTask(const string &n) : task_id(asignTaskId()), state(VIRGIN), name(n) {}
    TimerTask() : TimerTask("") {}
    TimerTask(const TimerTask &t) {
        nextExecutionTime = t.nextExecutionTime;
        period            = t.period;
        run_              = t.run_;
        name              = t.name;
    }
    TimerTask &operator=(const TimerTask &t) {
        if (&t == this) return *this;
        run_              = t.run_;
        nextExecutionTime = t.nextExecutionTime;
        period            = t.period;
        name              = t.name;
        return *this;
    }

    TimerTask &operator=(TimerTask &&t) {
        if (&t == this) return *this;

        run_              = std::move(t.run_);
        nextExecutionTime = std::move(t.nextExecutionTime);
        period            = std::move(t.period);
        name              = t.name;
        return *this;
    }

    void onRun(function<void(TimerTaskPtr &t)> &&callback) { run_ = std::move(callback); }
    void run(TimerTaskPtr &t) {
        if (run_ != nullptr) run_(t);
    }

    ~TimerTask() { state = CANCELLED; }
    inline bool cancel() {
        bool result = (state == SCHEDULED);
        state       = CANCELLED;
        return result;
    }

    inline int32_t State() { return state; }
    inline void    setState(int32_t s) { state = s; }

    bool operator<(const TimerTask &t2) { return nextExecutionTime < t2.nextExecutionTime; }
    bool operator>(const TimerTask &t2) { return nextExecutionTime > t2.nextExecutionTime; }

    bool operator==(const TimerTask &t2) {
        if (task_id == t2.task_id) return true;

        return false;
    }

    int64_t getTaskId() const { return task_id; }

    void reload() { nextExecutionTime += period; }

    const TimePoint_t &schedTime() const { return nextExecutionTime; }

    void setName(const string n) { name = n; }
    void setName(const char *n) { name = n; }

    void setSchedDelay(int64_t delay) { nextExecutionTime = Clock_t::now() + MillisDuration_t(delay); }

    void setSchedTime(const TimePoint_t &tp) { nextExecutionTime = tp; }

    const MillisDuration_t &repeatPeriod() const { return period; }

    void setRepeatPeriod(int64_t p) { period = MillisDuration_t(p); }

    void setRepeatPeriod(const MillisDuration_t &p) { period = p; }

    const string &taskName() { return name; }
};

class Timer {
private:
    TimerTaskQueue          queue;
    bool                    newTasksMayBeScheduled;
    std::mutex              lock;
    std::thread             worker_;
    std::condition_variable condition;

public:
    ~Timer();
    Timer() : newTasksMayBeScheduled(true) {}

    template <typename T = TimerTaskPtr>
    void schedule(T &&task, int64_t delay) {
        if (delay < 0) return;
        sched(std::forward<T>(task), delay, 0);
    }

    template <typename T = TimerTaskPtr>
    void scheduleAtFixedRate(T &&task, int64_t delay, int64_t period) {
        if (delay < 0) return;
        if (period <= 0) return;
        sched(std::forward<T>(task), delay, period);
    }
    void cancel();
    void start() {
        worker_ = thread([this]() { mainLoop(); });
    }

    void removeCancelTask() {
        while (!queue.empty()) {
            if (queue.top()->State() == CANCELLED) {
                spdlog::info("Task id : {} has bean canceled , remove now !", queue.top()->getTaskId());
                queue.pop(); // 删除
            } else {
                break;
            }
        }
    }

private:
    void        mainLoop();
    void        finalize();
    inline void notify() { condition.notify_one(); }

    template <typename T = TimerTaskPtr>
    void sched(T &&task, int64_t delay, int64_t period) {
        lock_guard<mutex> guard(lock);
        if (!newTasksMayBeScheduled) return;

        {
            if (task->State() != VIRGIN) return;
            task->setSchedTime(Clock_t::now() + MillisDuration_t(delay));
            task->setRepeatPeriod(MillisDuration_t(period));
            task->setState(SCHEDULED);
        }

        queue.push(std::forward<T>(task));
        notify();
    }
};

Timer::~Timer() {
    finalize();
    worker_.join();
}

void Timer::finalize() {
    lock_guard<mutex> guard(lock);
    newTasksMayBeScheduled = false;
    notify();
}

void Timer::cancel() {
    lock_guard<mutex> guard(lock);
    newTasksMayBeScheduled = false;
    notify(); // In case queue was already empty.
}

/**
 * The main timer loop.  (See class comment.)
 */
void Timer::mainLoop() {
    while (true) {
        TimerTaskPtr task;
        bool         taskFired = false;
        {
            unique_lock<mutex> lk(lock);
            // Wait for queue to become non-empty
            while (queue.empty() && newTasksMayBeScheduled) {
                condition.wait(lk, [this] { return !this->queue.empty() || !newTasksMayBeScheduled; });
            }

            if (!newTasksMayBeScheduled) break;

            removeCancelTask();
            if (queue.empty()) continue;

            // Queue nonempty; look at first evt and do the right thing
            MillisDuration_t waitFor(0);
            task.reset();
            task = queue.top();
            {
                TimePoint_t currentTime = Clock_t::now();
                waitFor                 = std::chrono::duration_cast<MillisDuration_t>(task->schedTime() - currentTime);
                if (waitFor.count() <= 0) {
                    queue.pop(); // 删除
                    taskFired = true;
                    if (task->repeatPeriod().count() <= 0) { // Non-repeating, remove
                        task->setState(EXECUTED);
                        spdlog::info("Executed task id : {} ,has removed !", task->getTaskId());
                    } else { // Repeating task, reschedule
                        task->reload();
                        queue.push(task); // Re queue task!
                    }
                }
            }

            if (!taskFired && waitFor.count() > 0) { // Task hasn't yet fired; wait
                condition.wait_for(lk, waitFor);
            }
        }
        if (taskFired) // Task fired; run it, holding no locks
            task->run(task);
    }
    {
        lock_guard<mutex> guard(this->lock);
        this->newTasksMayBeScheduled = false;
    }
}

} // namespace es