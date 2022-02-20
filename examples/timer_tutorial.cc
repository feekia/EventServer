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
        t->cancel();
        t.reset();
    });

    TimerTaskPtr t2(new TimerTask("task2"));
    t2->onRun([&]() { cout << "TaskId: " << t2->getTaskId() << " TaskName: " << t2->taskName() << endl; });

    TimerTaskPtr t3(new TimerTask("task3"));
    t3->onRun([&]() { cout << "TaskId: " << t3->getTaskId() << " TaskName: " << t3->taskName() << endl; });

    timer->scheduleAtFixedRate(t, 2000, 2000);
    timer->scheduleAtFixedRate(t2, 3000, 1000);
    timer->scheduleAtFixedRate(t3, 3000, 3000);
    timer->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10 * 1000));
    cout << "program exit !" << endl;
    return 1;
}
