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

using namespace std;
using namespace es;

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
