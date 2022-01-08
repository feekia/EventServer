#include <algorithm>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <event2/event.h>
#include <event2/listener.h>

#include "acceptor.h"
#include "event_raii.h"
#include "wrapper.h"

using namespace std;

#define BUF_SIZE 1024
namespace es {
static std::atomic<bool> isStop(false);

Acceptor::Acceptor(std::function<void()> &&f) : holder(nullptr), breakCb(f) {}

int Acceptor::init(int port) {
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port   = htons(port);

    for (int i = 0; i < LISTEN_CNT; i++) {
        bases[i] = obtain_event_base();
        listeners[i] =
            obtain_evconnlistener(bases[i].get(), Acceptor::connection_cb, (void *)this,
                                  LEV_OPT_REUSEABLE | LEV_OPT_REUSEABLE_PORT, -1, (struct sockaddr *)&sin, sizeof(sin));
    }
    // holder = std::make_shared<SocketHolder>();
    holder = std::shared_ptr<SocketHolder>(SocketHolder::getInstance());
    return 0;
}
thread_local int                                 con_cnt = 0;
thread_local std::chrono::system_clock::duration locald;
void Acceptor::connection_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen,
                             void *ctx) {
    if (ctx == nullptr) return;

    con_cnt++;
    if (con_cnt == 1) {
        locald = std::chrono::system_clock::now().time_since_epoch();
    }

    if (con_cnt > 20) {
        std::chrono::system_clock::duration d         = std::chrono::system_clock::now().time_since_epoch();
        std::chrono::milliseconds           msec      = std::chrono::duration_cast<std::chrono::milliseconds>(d);
        std::chrono::milliseconds           localmsec = std::chrono::duration_cast<std::chrono::milliseconds>(locald);
        int64_t                             diff      = msec.count() - localmsec.count();
        if (diff > 5 * 1000) {
            cout << " connect tid: " << std::this_thread::get_id() << " con_cnt : " << con_cnt << endl;
            locald  = std::chrono::system_clock::now().time_since_epoch();
            con_cnt = 0;
        }
    }

    Acceptor *accp = (Acceptor *)ctx;
    if (isStop) {
        close(fd);
        return;
    }
    evutil_make_socket_nonblocking(fd);
    accp->holder->onConnect(fd);
}

void Acceptor::onListenBreak() {
    if (breakCb) breakCb();
    holder->waitStop();
}

void Acceptor::stop() {
    if (!isStop) {
        isStop = true;
        for (int i = 0; i < LISTEN_CNT; i++) {
            event_base_loopexit(bases[i].get(), nullptr);
        }
    }
}

void Acceptor::wait() {
    for (int i = 0; i < LISTEN_CNT; i++) {
        threads[i] = std::thread([this, i]() { event_base_loop(this->bases[i].get(), EVLOOP_NO_EXIT_ON_EMPTY); });
    }

    for (int i = 0; i < LISTEN_CNT; i++) {
        threads[i].join();
    }
    onListenBreak();
    cout << " exit the Acceptor wait" << endl;
}
Acceptor::~Acceptor() { cout << " Exit the Acceptor" << endl; }
} // namespace es