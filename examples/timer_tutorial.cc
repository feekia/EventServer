#include "logging.h"
#include <iostream>
#include <memory>
#include <stdio.h>
#include <string.h>

#include "timer.h"

using namespace std;
using namespace es;

int main(int argc, char **argv) {
    spdlog::info("Enter timer task tutorial !");
    TimerPtr     timer(new Timer);
    TimerTaskPtr t(new TimerTask("task1"));
    t->onRun([](TimerTaskPtr &t) {
        spdlog::info("TaskId (name: {}, id：{}) onRun invoke !", t->taskName(), t->getTaskId());
        t->cancel();
        t.reset();
    });

    TimerTaskPtr t2(new TimerTask("task2"));
    t2->onRun([](TimerTaskPtr &t) {
        spdlog::info("TaskId (name: {}, id：{}) onRun invoke !", t->taskName(), t->getTaskId());
    });

    TimerTaskPtr t3(new TimerTask("task3"));
    t3->onRun([](TimerTaskPtr &t) {
        spdlog::info("TaskId (name: {}, id：{}) onRun invoke !", t->taskName(), t->getTaskId());
    });

    TimerTaskPtr t4(new TimerTask("task4"));
    t4->onRun([](TimerTaskPtr &t) {
        spdlog::info("TaskId (name: {}, id：{}) onRun invoke !", t->taskName(), t->getTaskId());
        t->cancel();
        t.reset();
    });

    timer->scheduleAtFixedRate(t, 2000, 2000);
    timer->schedule(t2, 3000);                  // schedule once
    timer->scheduleAtFixedRate(t3, 3000, 1000); // schedule repeated in 3000 milliseconds;
    timer->scheduleAtFixedRate(t4, 2000, 2000); // schedule repeated in 3000 milliseconds;
    timer->start();
    std::this_thread::sleep_for(MillisDuration_t(10 * 1000));
    spdlog::info("program exit !");
    return 1;
}
