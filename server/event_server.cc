//============================================================================
// Name        : event_sever.cpp
// Author      : liyunfei   afreeliyunfeil@163.com
// Version     :
// Copyright   : Your copyright notice
// Description : event_sever in C++, Ansi-style
//============================================================================

#include <arpa/inet.h>
#include <csignal>
#include <errno.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event-config.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/listener.h>
#include <event2/thread.h>
#include <event2/util.h>
#include <iostream>
#include <memory>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "acceptor.h"
#include "event_raii.h"
#include "handler.h"
#include "message.h"

using namespace std;
using namespace es;

#if 1
#include "acceptor.h"

void errHandler(int err) { cout << "error in lv: " << err << endl; }

static Acceptor *pacceptor = nullptr;
void             sigHandler(int sig) {
    cout << "sigHandler sig: " << sig << endl;
    if (pacceptor == nullptr) {
        return;
    }
    switch (sig) {
    case SIGINT: pacceptor->stop(); break;
    case SIGPIPE: break;
    default: break;
    }
}

int main(int argc, char **argv) {

    if (argc < 2) {
        printf("Usage:port,example:8080 \n");
        return -1;
    }
    int port = atoi(argv[1]);
    std::signal(SIGINT, sigHandler);
    std::signal(SIGPIPE, sigHandler);
    // event_enable_debug_logging(EVENT_DBG_ALL);
    event_set_fatal_callback(errHandler);
    evthread_use_pthreads();
    pacceptor = new Acceptor([]() { cout << " break Acceptor in callback" << endl; });
    pacceptor->init(port);

    cout << "EventServer startup" << endl;
    pacceptor->wait();
    cout << "main thread exit" << endl;
    delete pacceptor;
    pacceptor = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}
#elif 0

class myHandler : public Handler {
    virtual void handleMessage(Message &msg) override {
        Handler::handleMessage(msg);
        switch (msg.m_what) {
        case 0: break;

        case 1: break;

        default: break;
        }

        cout << "IN myHandler case: " << msg.m_what << endl;
    }
};
int main(int argc, char **argv) {
    cout << "IN main" << endl;

    myHandler hdlr;
    for (int i = 0; i < 6; i++) {
        hdlr.sendEmptyMessage(i, 100 * i);
    }

    hdlr.postAtTime([]() { cout << "IN POST call back" << endl; }, 230);

    hdlr.stopSafty(true);
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(100000));
    }
    return 1;
}
#else
#include "timer.h"
int main(int argc, char **argv) {
    cout << "IN main" << endl;
    Timer     timer;
    TimerTask t;
    t.onRun([&]() {
        cout << "onRun is invoke 1 !" << endl;
    });


    TimerTask t2;
    t2.onRun([&]() {
        cout << "onRun is invoke 2 !" << endl;
    });

    timer.schedule(t, 1000);
    timer.schedule(t2, 3000, 1000);
    timer.Start();

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    return 1;
}
#endif
