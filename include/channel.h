
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
    SHUTDOWNTIMEOUT = 5,
    WRITETIMEOUT = 10,
    READTIMEOUT = 20,
    CLOSETIMEOUT = 10,
    HEARTBITTIMEOUT = 120
};
class channel : public std::enable_shared_from_this<channel>
{
private:
    evutil_socket_t fd;
    std::weak_ptr<socketholder> holder;
    std::mutex cMutex;
    std::atomic<bool> stop;
    std::atomic<bool> isProc;
    std::atomic<int8_t> state;
    buffer wBuf;
    buffer rBuf;
    raii_event rwEvent;
    int64_t timestamp = 0;

public:
    channel(std::weak_ptr<socketholder> &&h, evutil_socket_t _fd);
    ~channel();

    void listenWatcher(raii_event &&events);

    void monitorEvent(short events, size_t timeout);
    int32_t send(char *buffer, size_t l);
    void onDisconnect(evutil_socket_t fd);
    std::shared_ptr<channel> share()
    {
        return shared_from_this();
    }

    void onChannelRead(short events, void *ctx);
    void onChannelWrite(short events, void *ctx);
    void onChannelTimeout(short events, void *ctx);
    void handleEvent(short events)
    {
        std::unique_lock<std::mutex> lock(cMutex);
        if (events & EV_READ)
        {
            cout << " EV_READ :" << fd <<  endl;
            onChannelRead(events, nullptr);
        }
        else if (events & EV_WRITE)
        {
            cout << " EV_WRITE :" << fd <<  endl;
            onChannelWrite(events, nullptr);
        }
        else if (events & EV_TIMEOUT)
        {
            cout << " EV_TIMEOUT: " << fd <<  endl;
            onChannelTimeout(events, nullptr);
        }
        setProcing(false);
    }
    bool isProcing()
    {
        return isProc;
    }
    void setProcing(bool b)
    {
        isProc = b;
    }
    void closeSafty();

private:
    bool isHeartBrakeExpired()
    {
        std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
        std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(d);
        return (sec.count() - timestamp) > HEARTBITTIMEOUT;
    }
    uint64_t heartBrakeLeft()
    {
        std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
        std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(d);
        return (HEARTBITTIMEOUT - (sec.count() - timestamp));
    }

    void updateHearBrakeExpired()
    {
        std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
        std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(d);
        timestamp = sec.count();
    }

    void handleClose();
};
#endif
