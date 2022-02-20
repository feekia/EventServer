#ifndef _ES_TIMER_H_
#define _ES_TIMER_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <inttypes.h>
#include <list>
#include <math.h>
#include <mutex>
#include <string>
#include <thread>

using namespace std;

namespace es {
class TimerTask;
class Timer;
class TaskQueue;

using TimePoint_t      = std::chrono::time_point<std::chrono::steady_clock>;
using Clock_t          = std::chrono::steady_clock;
using MillisDuration_t = std::chrono::milliseconds;
using TimerTaskPtr     = std::shared_ptr<TimerTask>;
using TaskQueuePtr     = std::shared_ptr<TaskQueue>;
using TimerPtr         = std::shared_ptr<Timer>;

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
    std::function<void()> run_;

    string name;

    static int64_t asignTaskId() {
        static std::atomic_int64_t id;
        return id == INT64_MAX ? id = 0 : ++id;
    }

public:
    TimerTask(const string n) : task_id(asignTaskId()), state(VIRGIN), name(n) {}
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

    void onRun(function<void()> &&callback) { run_ = std::move(callback); }
    void run() {
        if (run_ != nullptr) run_();
    }

    ~TimerTask() { state = CANCELLED; }
    inline bool cancel() {
        bool result = (state == SCHEDULED);
        state       = CANCELLED;
        return result;
    }

    inline int32_t State() { return state; }
    inline void    setState(int32_t s) { state = s; }

    friend bool operator<(const TimerTask &t1, const TimerTask &t2) {
        if (t1.nextExecutionTime < t2.nextExecutionTime) return true;

        return false;
    }

    friend bool operator==(const TimerTask &t1, const TimerTask &t2) {
        if (t1.nextExecutionTime == t2.nextExecutionTime) return true;

        return false;
    }

    int64_t getTaskId() const { return task_id; }

    void reload() { nextExecutionTime += period; }

    const TimePoint_t &schedTime() const { return nextExecutionTime; }

    void setSchedTime(const TimePoint_t &tp) { nextExecutionTime = tp; }

    const MillisDuration_t &repeatPeriod() const { return period; }

    void setRepeatPeriod(const MillisDuration_t &p) { period = p; }

    const string &taskName() { return name; }
};

struct Compare {
    bool operator()(const TimerTaskPtr &a, const TimerTaskPtr &b) { return *a < *b; }
};

class TaskQueue {
private:
    /**
     * Priority queue represented as a balanced binary heap: the two children
     * of queue[n] are queue[2*n] and queue[2*n+1].  The priority queue is
     * ordered on the nextExecutionTime field: The TimerTask with the lowest
     * nextExecutionTime is in queue[1] (assuming the queue is nonempty).  For
     * each node n in the heap, and each descendant of n, d,
     * n.nextExecutionTime <= d.nextExecutionTime.
     */
    list<TimerTaskPtr> queue;
    Compare            compare;

public:
    TaskQueue() : compare(Compare()) {}
    ~TaskQueue() {}

    /**
     * Returns the number of tasks currently on the queue.
     */
    int32_t size() { return queue.size(); }

    /**
     * Adds a new task to the priority queue.
     */
    template <typename T = TimerTaskPtr>
    void add(T &&task) {
        queue.push_back(std::forward<T>(task));
        queue.sort(compare);
    }

    /**
     * Return the "head task" of the priority queue.  (The head task is an
     * task with the lowest nextExecutionTime.)
     */
    TimerTaskPtr &getFront() { return queue.front(); }

    /**
     * Remove the head task from the priority queue.
     */
    void removeFront() { queue.pop_front(); }

    /**
     * Sets the nextExecutionTime associated with the head task to the
     * specified value, and adjusts priority queue accordingly.
     */
    void sort() { queue.sort(compare); }

    /**
     * Returns true if the priority queue contains no elements.
     */
    bool isEmpty() { return queue.size() == 0; }

    /**
     * Removes all elements from the priority queue.
     */
    void clear() { queue.clear(); };
};

class Timer {

private:
    TaskQueuePtr            queue;
    bool                    newTasksMayBeScheduled;
    std::mutex              lock;
    std::thread             worker_;
    std::condition_variable condition;

public:
    ~Timer();
    Timer();

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
    void Start();

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

        queue->add(std::forward<T>(task));
        notify();
    }
};

} // namespace es

#endif // end of _ES_TIMER_H_
