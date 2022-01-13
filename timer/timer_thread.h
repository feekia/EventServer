#ifndef _TIMER_THREAD_H_
#define _TIMER_THREAD_H_

#include <condition_variable>
#include <inttypes.h>
#include <mutex>
#include <thread>

using namespace std;

namespace es {

class TaskQueue;
class TimerTask;

class TimerThread {
private:
    /**
     * This flag is set to false by the reaper to inform us that there
     * are no more live references to our Timer object.  Once this flag
     * is true and there are no more tasks in our queue, there is no
     * work left for us to do, so we terminate gracefully.  Note that
     * this field is protected by queue's monitor!
     */
    bool newTasksMayBeScheduled = true;

    /**
     * Our Timer's queue.  We store this reference in preference to
     * a reference to the Timer so the reference graph remains acyclic.
     * Otherwise, the Timer would never be garbage-collected and this
     * thread would never go away.
     */

    TaskQueue *        queue;
    mutex              lock;
    thread             worker_;
    condition_variable condition;
    
    friend class Timer;

public:
    TimerThread(TaskQueue *q);
    ~TimerThread();

    void Start();

    /**
     * The main timer loop.  (See class comment.)
     */
    void mainLoop();

    
    inline void notify(){}
};

} // namespace es

#endif // end of _TIMER_THREAD_H_