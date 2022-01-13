#ifndef _ES_TIMER_H_
#define _ES_TIMER_H_

#include <atomic>
#include <inttypes.h>
#include <math.h>
#include <mutex>
#include <string>

using namespace std;

namespace es {

class TimerThread;
class TaskQueue;
class TimerTask;

class Timer {
private:
    TaskQueue *queue;
    mutex lock;
    TimerThread *worker;

public:
    ~Timer();
    Timer();
    void finalize();
    int32_t serialNumber();

    void schedule(TimerTask &task, int64_t delay);
    void schedule(TimerTask &task, int64_t delay, int64_t period) ;

    void scheduleAtFixedRate(TimerTask &task, int64_t delay, int64_t period) ;

    void sched(TimerTask &task, int64_t delay, int64_t period);
    void cancel();
    int32_t purge();
};

} // namespace es

#endif // end of _ES_TIMER_H_
