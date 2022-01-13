#include "task_queue.h"
#include "timer_task.h"
#include <inttypes.h>
#include <list>
#include <vector>

#include <algorithm>

using namespace std;

namespace es {
TaskQueue::TaskQueue() : queue(TASK_QUEUE_DEFAULT_LEN){}

TaskQueue::~TaskQueue() {}

/**
 * Returns the number of tasks currently on the queue.
 */
int32_t TaskQueue::size() { return queue.size(); }

/**
 * Adds a new task to the priority queue.
 */ 
void TaskQueue::add(const TimerTask &task) {
    // Grow backing store if necessary
    queue.push_back(task);
    sort(queue.begin(),queue.end(),less<TimerTask>());
}
/**
 * Return the "head task" of the priority queue.  (The head task is an
 * task with the lowest nextExecutionTime.)
 */
TimerTask &TaskQueue::getMin() { return queue[0]; }

/**
 * Return the ith task in the priority queue, where i ranges from 1 (the
 * head task, which is returned by getMin) to the number of tasks on the
 * queue, inclusive.
 */
TimerTask &TaskQueue::get(int32_t i) { return queue[i]; }

/**
 * Remove the head task from the priority queue.
 */
void TaskQueue::removeMin() {
    vector<TimerTask>::iterator it = queue.begin();
    queue.erase(it);

    sort(queue.begin(),queue.end(),less<TimerTask>());
}

/**
 * Removes the ith element from queue without regard for maintaining
 * the heap invariant.  Recall that queue is one-based, so
 * 1 <= i <= size.
 */
void TaskQueue::quickRemove(int32_t i) {
    vector<TimerTask>::iterator it = queue.begin();
    it += i;
    queue.erase(it);
}

/**
 * Sets the nextExecutionTime associated with the head task to the
 * specified value, and adjusts priority queue accordingly.
 */
void TaskQueue::rescheduleMin(int64_t newTime) {
    queue[1].nextExecutionTime = newTime;
    sort(queue.begin(),queue.end(),less<TimerTask>());
}

/**
 * Returns true if the priority queue contains no elements.
 */
bool TaskQueue::isEmpty() { return queue.size() == 0; }

/**
 * Removes all elements from the priority queue.
 */
void TaskQueue::clear() {
    queue.clear();
}

} // namespace es