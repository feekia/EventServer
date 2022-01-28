#include "timer.h"
#include "task_queue.h"
#include "timer_task.h"
#include <atomic>
#include <inttypes.h>
#include <iostream>
#include <math.h>
#include <mutex>
#include <string>
#include <thread>

using namespace std;

namespace es {

Timer::Timer() : newTasksMayBeScheduled(true) { queue = new TaskQueue(); }

Timer::~Timer() {
    finalize();
    worker_.join();

    delete queue;
    cout << "~Timer" << endl;
    queue = nullptr;
}

void Timer::finalize() {
    lock_guard<mutex> guard(lock);
    newTasksMayBeScheduled = false;
    notify();
}

void Timer::schedule(TimerTask &&task, int64_t delay) {
    if (delay < 0) return;
    sched(std::move(task), delay, 0);
}

void Timer::scheduleAtFixedRate(TimerTask &&task, int64_t delay, int64_t period) {
    if (delay < 0) return;
    if (period <= 0) return;
    sched(std::move(task), delay, period);
}

void Timer::sched(TimerTask &&task, int64_t delay, int64_t period) {

    {
        lock_guard<mutex> guard(lock);
        if (!newTasksMayBeScheduled) return;

        {
            lock_guard<mutex> guard(task);
            if (task.state != VIRGIN) return;
            task.nextExecutionTime = Clock_t::now() + std::chrono::milliseconds(delay);
            task.period            = std::chrono::milliseconds(period);
            task.state             = SCHEDULED;
        }

        queue->add(task);
        notify();
    }
}

void Timer::cancel() {
    lock_guard<mutex> guard(lock);
    newTasksMayBeScheduled = false;
    queue->clear();
    notify(); // In case queue was already empty.
}

void Timer::Start() {
    worker_ = thread([this]() {
        mainLoop();
        {
            unique_lock<mutex> unique_guard(this->lock);
            this->newTasksMayBeScheduled = false;
            this->queue->clear(); // Eliminate obsolete references
            cout << "Timer is quit !" << endl;
        }
    });
    // worker_.detach();
}

/**
 * The main timer loop.  (See class comment.)
 */
void Timer::mainLoop() {
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
            std::chrono::milliseconds waitFor;
            task = queue->getFront();
            {
                lock_guard<mutex> guard(task);
                if (task.state == CANCELLED) {
                    queue->removeFront();
                    continue; // No action required, poll queue again
                }

                TimePoint_t currentTime = Clock_t::now();
                waitFor = std::chrono::duration_cast<std::chrono::milliseconds>(task.nextExecutionTime - currentTime);
                if (task.nextExecutionTime <= currentTime) {
                    taskFired = true;
                    if (task.period == std::chrono::milliseconds(0)) { // Non-repeating, remove
                        queue->removeFront();
                        task.state = EXECUTED;
                    } else { // Repeating task, reschedule
                        queue->rescheduleFront(task.nextExecutionTime + task.period);
                    }
                }
            }

            if (!taskFired) { // Task hasn't yet fired; wait
                condition.wait_for(unique_guard, waitFor);
            }
        }
        if (taskFired) // Task fired; run it, holding no locks
            task.run();
    }
}
} // end of namespace es