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
            unique_lock<mutex> lk(lock);
            // Wait for queue to become non-empty
            while (queue->isEmpty() && newTasksMayBeScheduled) {
                condition.wait(lk, [this] { return !this->queue->isEmpty() || !newTasksMayBeScheduled; });
            }

            if (!newTasksMayBeScheduled) break;

            queue->removeCancel(); // Remove all CANCELED task
            if (queue->isEmpty()) continue;

            // Queue nonempty; look at first evt and do the right thing
            MillisDuration_t waitFor;
            task = queue->getFront();
            {
                TimePoint_t currentTime = Clock_t::now();
                waitFor                 = std::chrono::duration_cast<MillisDuration_t>(task->schedTime() - currentTime);
                if (task->schedTime() <= currentTime) {
                    taskFired = true;
                    if (task->repeatPeriod() == MillisDuration_t(0)) { // Non-repeating, remove
                        queue->removeFront();
                        task->setState(EXECUTED);
                        cout << "Executed task id : " << task->getTaskId() << endl;
                    } else { // Repeating task, reschedule
                        task->reload();
                        queue->sort();
                    }
                    // wait for next schedule time
                    // if (queue->isEmpty()) {
                    //     waitFor = MillisDuration_t(0);
                    // } else {
                    //     waitFor = std::chrono::duration_cast<MillisDuration_t>(queue->getFront()->schedTime() -
                    //                                                                     currentTime);
                    // }
                }
            }

            if (!taskFired && waitFor > MillisDuration_t(0)) { // Task hasn't yet fired; wait
                condition.wait_for(lk, waitFor);
            }
        }
        if (taskFired) // Task fired; run it, holding no locks
            task->run();
    }
    {
        lock_guard<mutex> guard(this->lock);
        this->newTasksMayBeScheduled = false;
        this->queue->clear(); // Eliminate obsolete references
    }
}
} // end of namespace es