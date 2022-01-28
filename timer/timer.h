#ifndef _ES_TIMER_H_
#define _ES_TIMER_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <inttypes.h>
#include <math.h>
#include <mutex>
#include <string>
#include <task_queue.h>
#include <thread>
#include <timer_task.h>

using namespace std;

namespace es {

class TaskQueue;
class TimerTask;

class Timer {
    using TimePoint_t = std::chrono::time_point<std::chrono::steady_clock>;
    using Clock_t     = std::chrono::steady_clock;

private:
    TaskQueue *             queue;
    bool                    newTasksMayBeScheduled;
    std::mutex              lock;
    std::thread             worker_;
    std::condition_variable condition;

public:
    ~Timer();
    Timer();

    void schedule(TimerTask &&task, int64_t delay);
    void scheduleAtFixedRate(TimerTask &&task, int64_t delay, int64_t period);
    void cancel();
    void Start();

private:
    void        mainLoop();
    void        finalize();
    inline void notify() { condition.notify_one(); }
    void        sched(TimerTask &&task, int64_t delay, int64_t period);
};

} // namespace es

#endif // end of _ES_TIMER_H_
