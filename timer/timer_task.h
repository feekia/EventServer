#ifndef _TIMER_TASK_H_
#define _TIMER_TASK_H_

#include <chrono>
#include <ctime>
#include <functional>
#include <inttypes.h>
#include <mutex>

using namespace std;

namespace es {

class Timer;

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

class TimerTask : private std::mutex {
    using TimePoint_t = std::chrono::time_point<std::chrono::steady_clock>;
    using Clock_t     = std::chrono::steady_clock;

private:
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
    std::chrono::milliseconds period;

    /**
     * This lock is used to control access to the TimerTask internals.
     */
    std::function<void()> run;
    friend class Timer;
    friend class TaskQueue;

public:
    TimerTask() : state(VIRGIN) {}
    TimerTask(const TimerTask &t) {
        nextExecutionTime = t.nextExecutionTime;
        period            = t.period;
        run               = t.run;
    }
    TimerTask &operator=(const TimerTask &t) {
        if (&t == this) return *this;
        run               = t.run;
        nextExecutionTime = t.nextExecutionTime;
        period            = t.period;
        return *this;
    }

    TimerTask &operator=(TimerTask &&t) {
        if (&t == this) return *this;
        run               = std::move(t.run);
        nextExecutionTime = std::move(t.nextExecutionTime);
        period            = std::move(t.period);
        return *this;
    }

    void onRun(function<void()> &&callback) { run = std::move(callback); }

    ~TimerTask() { state = CANCELLED; }
    inline bool cancel() {
        lock_guard<mutex> guard(*this);

        bool result = (state == SCHEDULED);
        state       = CANCELLED;
        return result;
    }

    friend bool operator<(const TimerTask &t1, const TimerTask &t2) {
        if (t1.nextExecutionTime < t2.nextExecutionTime) return true;

        return false;
    }

    friend bool operator==(const TimerTask &t1, const TimerTask &t2) {
        if (t1.nextExecutionTime == t2.nextExecutionTime) return true;

        return false;
    }
};

} // namespace es

#endif // end of _TIMER_TASK_H_