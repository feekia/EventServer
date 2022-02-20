#include <iostream>
#include <memory>
#include <stdio.h>
#include <string.h>

#include "timer.h"

using namespace std;
using namespace es;

int main(int argc, char **argv) {
    cout << "Timer task tutorial " << endl;
    TimerPtr     timer(new Timer);
    TimerTaskPtr t(new TimerTask("task1"));
    t->onRun([&]() {
        cout << "TaskId: " << t->getTaskId() << " TaskName: " << t->taskName() << endl;

        // remove self !
        t->cancel();
        t.reset();
    });

    TimerTaskPtr t2(new TimerTask("task2"));
    t2->onRun([&]() { cout << "TaskId: " << t2->getTaskId() << " TaskName: " << t2->taskName() << endl; });

    TimerTaskPtr t3(new TimerTask("task3"));
    t3->onRun([&]() { cout << "TaskId: " << t3->getTaskId() << " TaskName: " << t3->taskName() << endl; });

    TimerTaskPtr t4(new TimerTask("task4"));
    t4->onRun([&]() {
        cout << "TaskId: " << t4->getTaskId() << " TaskName: " << t4->taskName() << endl;
        t4->cancel();
        t4.reset();
    });

    timer->scheduleAtFixedRate(t, 2000, 2000);
    timer->schedule(t2, 3000);                  // schedule once
    timer->scheduleAtFixedRate(t3, 3000, 3000); // schedule repeated in 3000 milliseconds;
    timer->scheduleAtFixedRate(t4, 2000, 2000); // schedule repeated in 3000 milliseconds;
    timer->Start();
    std::this_thread::sleep_for(MillisDuration_t(10 * 1000));
    cout << "program exit !" << endl;
    return 1;
}
