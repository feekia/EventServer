#ifndef _TASK_QUEUE_H_
#define _TASK_QUEUE_H_
#include "timer_task.h"
#include <algorithm>
#include <chrono>
#include <inttypes.h>
#include <list>
#include <mutex>
#include <vector>


#define TASK_QUEUE_DEFAULT_LEN (64)
using namespace std;

namespace es {
class TimerTask;

struct CompareList {
    bool operator()(const TimerTask &t1, const TimerTask &t2) {
        if (t1 < t2) return true;
        return false;
    }
};

class TaskQueue {
    using TimePoint_t = std::chrono::time_point<std::chrono::steady_clock>;
    using Clock_t     = std::chrono::steady_clock;

private:
    /**
     * Priority queue represented as a balanced binary heap: the two children
     * of queue[n] are queue[2*n] and queue[2*n+1].  The priority queue is
     * ordered on the nextExecutionTime field: The TimerTask with the lowest
     * nextExecutionTime is in queue[1] (assuming the queue is nonempty).  For
     * each node n in the heap, and each descendant of n, d,
     * n.nextExecutionTime <= d.nextExecutionTime.
     */
    list<TimerTask> queue;

public:
    TaskQueue() {}
    ~TaskQueue() {}

    /**
     * Returns the number of tasks currently on the queue.
     */
    int32_t size() { return queue.size(); }

    /**
     * Adds a new task to the priority queue.
     */
    void add(const TimerTask &task) {
        queue.push_back(task);
        queue.sort(less<TimerTask>());
    }

    void add(TimerTask &&task) {
        queue.push_back(std::move(task));
        queue.sort(less<TimerTask>());
    }
    /**
     * Return the "head task" of the priority queue.  (The head task is an
     * task with the lowest nextExecutionTime.)
     */
    TimerTask &getFront() { return queue.front(); }

    /**
     * Remove the head task from the priority queue.
     */
    void removeFront() { queue.pop_front(); }

    /**
     * Sets the nextExecutionTime associated with the head task to the
     * specified value, and adjusts priority queue accordingly.
     */
    void rescheduleFront(const TimePoint_t &newTime) {
        queue.front().nextExecutionTime = newTime;
        queue.sort(less<TimerTask>());
    }

    /**
     * Returns true if the priority queue contains no elements.
     */
    bool isEmpty() { return queue.size() == 0; }

    /**
     * Removes all elements from the priority queue.
     */
    void clear() { queue.clear(); };
};
} // namespace es

#endif // end of