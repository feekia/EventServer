#ifndef _TIMER_TASK_H_
#define _TIMER_TASK_H_

#include <functional>
#include <inttypes.h>
#include <mutex>

using namespace std;

namespace es {

class TimerThread;
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

class TimerTask {
private:
    int32_t state = VIRGIN;

    /**
     * Next execution time for this task in the format returned by
     * System.currentTimeMillis, assuming this task is scheduled for execution.
     * For repeating tasks, this field is updated prior to each task execution.
     */
    int64_t nextExecutionTime;

    /**
     * Period in milliseconds for repeating tasks.  A positive value indicates
     * fixed-rate execution.  A negative value indicates fixed-delay execution.
     * A value of 0 indicates a non-repeating task.
     */
    int64_t period = 0;

    /**
     * This lock is used to control access to the TimerTask internals.
     */
    mutex            lock;
    function<void()> run;

    friend class TimerThread;
    friend class Timer;
    friend class TaskQueue;

public:
    TimerTask() {}
    TimerTask(function<void()> &&callback);
    TimerTask(const TimerTask &t) { run = t.run; }
    TimerTask &operator=(const TimerTask &t) {
        run = t.run;
        return *this;
    }

    bool operator==(const TimerTask &t) {
        if(nextExecutionTime == t.nextExecutionTime) return true;
        return false;
    }
    ~TimerTask();
    inline bool cancel() {
        lock_guard<mutex> guard(lock);

        bool result = (state == SCHEDULED);
        state       = CANCELLED;
        return result;
    }

    inline int64_t scheduledExecutionTime() {
        lock_guard<mutex> guard(lock);
        return (period < 0 ? nextExecutionTime + period : nextExecutionTime - period);
    }

    friend bool operator<(const TimerTask &t1, const TimerTask &t2) {
        if (t1.nextExecutionTime < t2.nextExecutionTime) return true;

        return false;
    }
};

} // namespace es

#endif // end of _TIMER_TASK_H_