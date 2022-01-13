#include "timer.h"
#include "task_queue.h"
#include "timer_task.h"
#include "timer_thread.h"
#include <atomic>
#include <inttypes.h>
#include <math.h>
#include <mutex>
#include <string>
#include <thread>

using namespace std;

namespace es {

Timer::Timer() {
    queue  = new TaskQueue();
    worker = new TimerThread(queue);
}

Timer::~Timer() {
    delete worker;
    worker = nullptr;
    delete queue;
    queue = nullptr;
}

void Timer::finalize() {
    lock_guard<mutex> guard(lock);
    worker->newTasksMayBeScheduled = false;
    worker->notify(); // In case queue is empty.
}

void Timer::schedule(TimerTask &task, int64_t delay) {
    if (delay < 0) return;
    sched(task, delay, 0);
}

void Timer::schedule(TimerTask &task, int64_t delay, int64_t period) {
    if (delay < 0) return;
    if (period <= 0) return;

    sched(task, delay, period);
}

void Timer::scheduleAtFixedRate(TimerTask &task, int64_t delay, int64_t period) {
    if (delay < 0) return;
    if (period <= 0) return;
    sched(task, delay, period);
}

void Timer::sched(TimerTask &task, int64_t delay, int64_t period) {

    if (abs(period) > (0xFFFFFFFFFFFFFFFF >> 1)) period >>= 1;

    std::chrono::system_clock::duration current = std::chrono::system_clock::now().time_since_epoch();
    std::chrono::milliseconds           msec    = std::chrono::duration_cast<std::chrono::milliseconds>(current);
    {
        lock_guard<mutex> guard(lock);
        if (!worker->newTasksMayBeScheduled) return;

        {
            lock_guard<mutex> guard(task.lock);
            if (task.state != VIRGIN) return;
            task.nextExecutionTime = msec.count() + delay;
            task.period            = period;
            task.state             = SCHEDULED;
        }

        queue->add(task);
        if (queue->getMin() == task) worker->notify();
    }
}

void Timer::cancel() {
    lock_guard<mutex> guard(lock);
    worker->newTasksMayBeScheduled = false;
    queue->clear();
    worker->notify(); // In case queue was already empty.
}

int32_t Timer::purge() {
    int32_t result = 0;
    {
        lock_guard<mutex> guard(lock);

        for (int32_t i = queue->size(); i > 0; i--) {
            if (queue->get(i).state == CANCELLED) {
                queue->quickRemove(i);
                result++;
            }
        }
    }
    return result;
}
} // end of namespace es