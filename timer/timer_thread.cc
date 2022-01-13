#include "timer_thread.h"
#include "task_queue.h"
#include "timer_task.h"
#include <chrono>
#include <condition_variable>
#include <inttypes.h>
#include <mutex>
#include <thread>

using namespace std;
namespace es {
TimerThread::TimerThread(TaskQueue *q) : queue(q) {}

TimerThread::~TimerThread() {}

void TimerThread::Start() {
    worker_ = thread([this]() {
        mainLoop();
        {
            unique_lock<mutex> unique_guard(this->lock);
            this->newTasksMayBeScheduled = false;
            this->queue->clear(); // Eliminate obsolete references
        }
    });
}

/**
 * The main timer loop.  (See class comment.)
 */
void TimerThread::mainLoop() {
    while (true) {
        TimerTask task;
        bool      taskFired = false;
        {
            unique_lock<mutex> unique_guard(lock);
            // Wait for queue to become non-empty
            while (queue->isEmpty() && newTasksMayBeScheduled) {
                condition.wait(unique_guard, [this] { return !this->queue->isEmpty(); });
            }

            if (queue->isEmpty()) break; // Queue is empty and will forever remain; die

            // Queue nonempty; look at first evt and do the right thing
            int64_t currentTime = 0, executionTime = 0;
            task = queue->getMin();
            {
                lock_guard<mutex> guard(task.lock);
                if (task.state == CANCELLED) {
                    queue->removeMin();
                    continue; // No action required, poll queue again
                }
                // TODO: fix this
                // currentTime   = System.currentTimeMillis();
                currentTime   = 0;
                executionTime = task.nextExecutionTime;
                if (taskFired = (executionTime <= currentTime)) {
                    if (task.period == 0) { // Non-repeating, remove
                        queue->removeMin();
                        task.state = EXECUTED;
                    } else { // Repeating task, reschedule
                        queue->rescheduleMin(task.period < 0 ? currentTime - task.period : executionTime + task.period);
                    }
                }
            }

            if (!taskFired) // Task hasn't yet fired; wait
                condition.wait_for(unique_guard, std::chrono::milliseconds(executionTime - currentTime));
        }
        if (taskFired) // Task fired; run it, holding no locks
            task.run();
    }
}
} // namespace es