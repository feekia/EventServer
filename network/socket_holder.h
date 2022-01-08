#ifndef __SOCKET_HOLDER_H__
#define __SOCKET_HOLDER_H__

#pragma once

#include <algorithm>
#include <string>
#include <thread>
#include <vector>

#include <atomic>
#include <condition_variable>
#include <event2/event.h>
#include <event2/listener.h>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "channel.h"
#include "event_raii.h"
#include "thread_pools.h"
#include "wrapper.h"

#define READ_LOOP_MAX        (8)
#define EVENT_BASE_WATCH_MAX (128 * 1024)

using namespace std;

namespace es {
class SocketHolder : public std::enable_shared_from_this<SocketHolder> {
private:
    std::array<raii_event_base, READ_LOOP_MAX>                                     rwatchers;
    std::array<std::thread, READ_LOOP_MAX>                                         watcher_thread;
    std::mutex                                                                     syncMutex[READ_LOOP_MAX];
    std::condition_variable                                                        condition;
    std::atomic_bool                                                               isStop;
    std::array<std::map<evutil_socket_t, std::shared_ptr<Channel>>, READ_LOOP_MAX> chns;
    ThreadPool                                                                     pools;
    friend Channel;
    static SocketHolder *instance;

private:
    SocketHolder();

public:
    virtual ~SocketHolder();
    void                     onConnect(evutil_socket_t fd);
    void                     onDisconnect(evutil_socket_t fd);
    size_t                   send(std::shared_ptr<Channel> c, char *d, size_t l);
    std::shared_ptr<Channel> getChannel(evutil_socket_t fd);
    void                     closeIdleChannel();
    void                     waitStop();

    std::shared_ptr<SocketHolder> share() { return shared_from_this(); }

    friend void onEvent(evutil_socket_t socket_fd, short events, void *ctx);

public:
    static SocketHolder *getInstance() {
        if (instance == nullptr) instance = new SocketHolder();

        return instance;
    }
    static std::shared_ptr<SocketHolder> getShared_ptr() {
        if (instance == nullptr) return nullptr;

        return instance->shared_from_this();
    }
};

} // namespace es

#endif /* __SOCKET_HOLDER_H__ */
