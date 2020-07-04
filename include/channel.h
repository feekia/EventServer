
#ifndef __CHANNEL_H
#define __CHANNEL_H

#pragma once

#include <mutex>
#include <atomic>

#include <event.h>
#include <event2/event.h>
#include "events.h"
#include "buffer.h"

using namespace std;
using f_event_cb = void (*)(evutil_socket_t socket_fd, short events, void *ctx);
class socketholder;
class channel : public std::enable_shared_from_this<channel>
{
private:
    evutil_socket_t fd;
    std::weak_ptr<socketholder> holder;
    std::mutex cMutex;
    std::atomic<bool> stop;
    buffer wBuf;
    buffer rBuf;
    raii_event rEvent;
    raii_event wEvent;

public:
    channel(std::weak_ptr<socketholder> h, evutil_socket_t _fd);
    ~channel();

    void startWatcher();
    void addReadEvent(size_t timeout);
    void addWriteEvent(size_t timeout);
    void removeReadEvent()
    {
        event_del(rEvent.get());
    }
    void removeWriteEvent()
    {
        event_del(wEvent.get());
    }
    bool send(char *buffer, size_t l);
    void onDisconnect(evutil_socket_t fd);
    std::shared_ptr<channel> share()
    {
        return shared_from_this();
    }

    static void onRead(evutil_socket_t socket_fd, short events, void *ctx);
    static void onWrite(evutil_socket_t socket_fd, short events, void *ctx);
    void closeSafty();
};
#endif
