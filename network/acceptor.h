#ifndef _ACCEPTOR_H_
#define _ACCEPTOR_H_

#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <event2/event.h>
#include <event2/listener.h>
#include <map>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "event_raii.h"
#include "socket_holder.h"
#include "wrapper.h"

using namespace std;
#define LISTEN_CNT (2)

namespace es {
class Acceptor {
private:
    std::array<raii_event_base, LISTEN_CNT>     bases;
    std::array<std::thread, LISTEN_CNT>         threads;
    std::array<raii_evconnlistener, LISTEN_CNT> listeners;
    std::shared_ptr<SocketHolder>               holder;
    std::function<void()>                       breakCb;

public:
    Acceptor(std::function<void()> &&f);
    virtual ~Acceptor();
    int         init(int port);
    void        stop();
    void        wait();
    static void connection_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen,
                              void *ctx);

private:
    void onListenBreak();
};

} // namespace es
#endif /* end of _ACCEPTOR_H_ */
