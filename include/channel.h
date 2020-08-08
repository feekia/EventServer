
#ifndef __CHANNEL_H
#define __CHANNEL_H

#pragma once

#include <mutex>
#include <atomic>
#include <chrono>
#include <event.h>
#include <event2/event.h>
#include "events.h"
#include "buffer.h"

using namespace std;
using f_event_cb = void (*)(evutil_socket_t socket_fd, short events, void *ctx);
class socketholder;
enum socket_state
{
    INIT,
    SHUTDOWN,
    CLOSE
};

enum timeout_config
{
    READTIMEOUT = 20,
    WRITETIMEOUT = 10,
    HEARTBITTIMEOUT = 60
};
class channel : public std::enable_shared_from_this<channel>
{
private:
    evutil_socket_t fd;
    std::weak_ptr<socketholder> holder;
    std::mutex cMutex;
    std::atomic<bool> stop;
    std::atomic<int8_t> state;
    buffer wBuf;
    buffer rBuf;
    raii_event rEvent;
    raii_event wEvent;
    int64_t timestamp = 0;

public:
    channel(std::weak_ptr<socketholder> &&h, evutil_socket_t _fd);
    ~channel();

    void listenWatcher(raii_event &&revent, raii_event &&wevent);

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

    bool isHeartBrakeExpired()
    {
        std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
        std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(d);
        return (sec.count() - timestamp) > HEARTBITTIMEOUT;
    }
    void updateHearBrakeExpired()
    {
        std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
        std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(d);
        timestamp = sec.count();
    }

    void onChannelRead(short events, void *ctx);
    void onChannelWrite(short events, void *ctx);
    void closeSafty();
};
#endif
