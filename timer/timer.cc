#include "timer.h"
#include <atomic>
#include <inttypes.h>
#include <iostream>
#include <math.h>
#include <mutex>
#include <string>
#include <thread>

using namespace std;

namespace es {

Timer::Timer() : queue(new TaskQueue), newTasksMayBeScheduled(true) {}

Timer::~Timer() {
    finalize();
    worker_.join();
    queue = nullptr;
}

void Timer::finalize() {
    lock_guard<mutex> guard(lock);
    newTasksMayBeScheduled = false;
    queue->clear();
    notify();
}

void Timer::cancel() {
    lock_guard<mutex> guard(lock);
    newTasksMayBeScheduled = false;
    queue->clear();
    notify(); // In case queue was already empty.
}

void Timer::Start() {
    worker_ = thread([this]() { mainLoop(); });
}

/**
 * The main timer loop.  (See class comment.)
 */
void Timer::mainLoop() {
    while (true) {
        TimerTaskPtr task;
        bool         taskFired = false;
        {
            unique_lock<mutex> unique_guard(lock);
            // Wait for queue to become non-empty
            while (queue->isEmpty() && newTasksMayBeScheduled) {
                condition.wait(unique_guard, [this] { return !this->queue->isEmpty() || !newTasksMayBeScheduled; });
            }

            if (queue->isEmpty()) break; // Queue is empty and will forever remain; die
            if (!newTasksMayBeScheduled) break;

            // Queue nonempty; look at first evt and do the right thing
            std::chrono::milliseconds waitFor;
            task = queue->getFront();
            {
                if (task->State() == CANCELLED) {
                    queue->removeFront();
                    cout << "remove cancelled task id : " << task->getTaskId() << endl;
                    continue; // No action required, poll queue again
                }

                TimePoint_t currentTime = Clock_t::now();
                waitFor = std::chrono::duration_cast<std::chrono::milliseconds>(task->schedTime() - currentTime);
                if (task->schedTime() <= currentTime) {
                    taskFired = true;
                    if (task->repeatPeriod() == std::chrono::milliseconds(0)) { // Non-repeating, remove
                        queue->removeFront();
                        task->setState(EXECUTED);
                    } else { // Repeating task, reschedule
                        task->reload();
                        queue->sort();
                    }
                    // wait for next schedule time
                    if (queue->isEmpty()) {
                        waitFor = std::chrono::milliseconds(0);
                    } else {
                        waitFor = std::chrono::duration_cast<std::chrono::milliseconds>(queue->getFront()->schedTime() -
                                                                                        currentTime);
                    }
                }
            }

            if (waitFor > std::chrono::milliseconds(0)) { // Task hasn't yet fired; wait
                condition.wait_for(unique_guard, waitFor);
            }
        }
        if (taskFired) // Task fired; run it, holding no locks
            task->run();
    }
    {
        unique_lock<mutex> unique_guard(this->lock);
        this->newTasksMayBeScheduled = false;
        this->queue->clear(); // Eliminate obsolete references
    }
}
} // end of namespace es