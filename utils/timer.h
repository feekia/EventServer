#pragma once

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

namespace es {
class Timer;
class TimerTask;

using TimerPtr         = std::shared_ptr<Timer>;
using TimePoint_t      = std::chrono::steady_clock::time_point;
using Clock_t          = std::chrono::steady_clock;
using MillisDuration_t = std::chrono::milliseconds;
using TimerTaskPtr     = std::shared_ptr<TimerTask>;

using TimerTaskQueue =
    std::priority_queue<TimerTaskPtr, std::deque<TimerTaskPtr>, util::SpGreater<TimerTaskPtr>>;

enum {
    VIRGIN    = 0,
    SCHEDULED = 1,
    EXECUTED  = 2,
    CANCELLED = 3,
};

class TimerTask {
private:
    int64_t task_id;
    int32_t state = VIRGIN;

    TimePoint_t nextExecutionTime;

    MillisDuration_t period;

    std::function<void(TimerTaskPtr &t)> run_;

    std::string name;

    static int64_t asignTaskId() {
        static std::atomic_int64_t id;
        return id == INT64_MAX ? id = 0 : ++id;
    }

public:
    TimerTask(std::string &&n) : task_id(asignTaskId()), state(VIRGIN), name(std::move(n)) {}
    TimerTask(const std::string &n) : task_id(asignTaskId()), state(VIRGIN), name(n) {}
    TimerTask() : TimerTask("") {}
    TimerTask(const TimerTask &t) {
        nextExecutionTime = t.nextExecutionTime;
        period            = t.period;
        run_              = t.run_;
        name              = t.name;
    }
    TimerTask &operator=(const TimerTask &t) {
        if (&t == this)
            return *this;
        run_              = t.run_;
        nextExecutionTime = t.nextExecutionTime;
        period            = t.period;
        name              = t.name;
        return *this;
    }

    TimerTask &operator=(TimerTask &&t) {
        if (&t == this)
            return *this;

        run_              = std::move(t.run_);
        nextExecutionTime = std::move(t.nextExecutionTime);
        period            = std::move(t.period);
        name              = t.name;
        return *this;
    }

    void onRun(std::function<void(TimerTaskPtr &t)> &&callback) { run_ = std::move(callback); }
    void run(TimerTaskPtr &t) {
        if (run_ != nullptr)
            run_(t);
    }

    ~TimerTask() {
        state = CANCELLED;
        std::cout << "TimerTask deconstruct !" << std::endl;
    }
    inline bool cancel() {
        bool result = (state == SCHEDULED);
        state       = CANCELLED;
        return result;
    }

    inline int32_t State() { return state; }
    inline void setState(int32_t s) { state = s; }

    bool operator<(const TimerTask &t2) { return nextExecutionTime < t2.nextExecutionTime; }
    bool operator>(const TimerTask &t2) { return nextExecutionTime > t2.nextExecutionTime; }

    bool operator==(const TimerTask &t2) {
        if (task_id == t2.task_id)
            return true;

        return false;
    }

    int64_t getTaskId() const { return task_id; }

    void reload() { nextExecutionTime += period; }

    const TimePoint_t &schedTime() const { return nextExecutionTime; }

    void setName(const std::string n) { name = n; }
    void setName(const char *n) { name = n; }

    void setSchedDelay(int64_t delay) {
        nextExecutionTime = Clock_t::now() + MillisDuration_t(delay);
    }

    void setSchedTime(const TimePoint_t &tp) { nextExecutionTime = tp; }

    const MillisDuration_t &repeatPeriod() const { return period; }

    void setRepeatPeriod(int64_t p) { period = MillisDuration_t(p); }

    void setRepeatPeriod(const MillisDuration_t &p) { period = p; }

    const std::string &taskName() { return name; }
};

class Timer {
private:
    TimerTaskQueue queue;
    bool newTasksMayBeScheduled;
    std::mutex lock;
    std::thread worker_;
    std::condition_variable condition;

public:
    ~Timer() {
        finalize();
        worker_.join();
    }

    Timer() : newTasksMayBeScheduled(true) {}

    template <typename T = TimerTaskPtr>
    void schedule(T &&task, int64_t delay, int64_t period = 0) {
        if (delay < 0 || period < 0)
            return;

        sched(std::forward<T>(task), delay, period);
    }

    template <typename T = TimerTaskPtr>
    void scheduleAtFixedRate(T &&task, int64_t delay, int64_t period) {
        if (delay < 0)
            return;
        if (period <= 0)
            return;
        sched(std::forward<T>(task), delay, period);
    }
    void cancel() {
        std::lock_guard<std::mutex> guard(lock);
        newTasksMayBeScheduled = false;
        notify(); // In case queue was already empty.
    }
    void start() {
        worker_ = std::thread([this]() { mainLoop(); });
    }

    void removeCancelTask() {
        while (!queue.empty()) {
            if (queue.top()->State() == CANCELLED) {
                queue.pop(); // 删除
            } else {
                break;
            }
        }
    }

private:
    void mainLoop() {
        while (true) {
            TimerTaskPtr task;
            bool taskFired = false;
            {
                std::unique_lock<std::mutex> lk(lock);
                // Wait for queue to become non-empty
                while (queue.empty() && newTasksMayBeScheduled) {
                    condition.wait(
                        lk, [this] { return !this->queue.empty() || !newTasksMayBeScheduled; });
                }

                if (!newTasksMayBeScheduled)
                    break;

                removeCancelTask();
                if (queue.empty())
                    continue;

                // Queue nonempty; look at first evt and do the right thing
                MillisDuration_t waitFor(0);
                task.reset();
                task = queue.top();
                {
                    TimePoint_t currentTime = Clock_t::now();
                    waitFor = std::chrono::duration_cast<MillisDuration_t>(task->schedTime() -
                                                                           currentTime);
                    if (waitFor.count() <= 0) {
                        queue.pop(); // 删除
                        taskFired = true;
                        if (task->repeatPeriod().count() <= 0) { // Non-repeating, remove
                            task->setState(EXECUTED);
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
            std::lock_guard<std::mutex> guard(this->lock);
            this->newTasksMayBeScheduled = false;
        }
        std::cout << "end timer mainloop" << std::endl;
    }
    void finalize() {
        std::lock_guard<std::mutex> guard(lock);
        newTasksMayBeScheduled = false;
        notify();
    }

    inline void notify() { condition.notify_one(); }

    template <typename T = TimerTaskPtr> void sched(T &&task, int64_t delay, int64_t period) {
        std::lock_guard<std::mutex> guard(lock);
        if (!newTasksMayBeScheduled)
            return;

        task->setSchedTime(Clock_t::now() + MillisDuration_t(delay));
        task->setRepeatPeriod(MillisDuration_t(period));
        task->setState(SCHEDULED);

        queue.push(std::forward<T>(task));
        notify();
    }
};
} // namespace es
