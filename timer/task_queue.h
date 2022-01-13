#ifndef _TASK_QUEUE_H_
#define _TASK_QUEUE_H_
#include <inttypes.h>
#include <list>
#include <vector>

#define TASK_QUEUE_DEFAULT_LEN (64)
using namespace std;

namespace es {
class TimerTask;

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
    vector<TimerTask> queue;

public:
    TaskQueue();
    ~TaskQueue();

    /**
     * Returns the number of tasks currently on the queue.
     */
    int32_t size();

    /**
     * Adds a new task to the priority queue.
     */
    void add(const TimerTask &task);
    /**
     * Return the "head task" of the priority queue.  (The head task is an
     * task with the lowest nextExecutionTime.)
     */
    TimerTask &getMin();

    /**
     * Return the ith task in the priority queue, where i ranges from 1 (the
     * head task, which is returned by getMin) to the number of tasks on the
     * queue, inclusive.
     */
    TimerTask &get(int32_t i);

    /**
     * Remove the head task from the priority queue.
     */
    void removeMin();

    /**
     * Removes the ith element from queue without regard for maintaining
     * the heap invariant.  Recall that queue is one-based, so
     * 1 <= i <= size.
     */
    void quickRemove(int32_t i);

    /**
     * Sets the nextExecutionTime associated with the head task to the
     * specified value, and adjusts priority queue accordingly.
     */
    void rescheduleMin(int64_t newTime);

    /**
     * Returns true if the priority queue contains no elements.
     */
    bool isEmpty();

    /**
     * Removes all elements from the priority queue.
     */
    void clear();
};
} // namespace es

#endif // end of