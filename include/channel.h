
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
    HEARTBITTIMEOUT = 480
};
class channel : public std::enable_shared_from_this<channel>
{
private:
    evutil_socket_t fd;
    std::weak_ptr<socketholder> holder;
    std::mutex cMutex;
    std::atomic<bool> stop;
    std::atomic<bool> rFinish;
    std::atomic<bool> wFinish;
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
    void removeRWEvent()
    {
        int wpending = event_pending(wEvent.get(), EV_WRITE | EV_TIMEOUT, nullptr);
        if (wpending == (EV_WRITE | EV_TIMEOUT))
        {
            int ret = event_del(wEvent.get());
            if (ret != 0)
            {
                cout << "remove write event error :" << fd << endl;
            }
        }
        int rpending = event_pending(rEvent.get(), EV_READ | EV_TIMEOUT, nullptr);
        if (rpending == (EV_READ | EV_TIMEOUT))
        {
            int ret = event_del(rEvent.get());
            if (ret != 0)
            {
                cout << "remove read event error :" << fd << endl;
            }
        }
    }
    int32_t send(char *buffer, size_t l);
    void onDisconnect(evutil_socket_t fd);
    std::shared_ptr<channel> share()
    {
        return shared_from_this();
    }

    void onChannelRead(short events, void *ctx);
    void onChannelWrite(short events, void *ctx);

    void closeSafty();
private:
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

    bool send_internal()
    {
        std::unique_lock<std::mutex> lock(cMutex);
        if (wBuf.size() > 0)
        {
            wFinish = false;
            int ret = wBuf.writesocket(fd);
            if(ret == -1){
                wFinish = true;
                return false;
            }
            addWriteEvent(WRITETIMEOUT);
        }
        else
        {
            wFinish = true;
            if (stop == true && state == INIT)
            {
                cout << "send_internal: shutdown write mode" << endl;
                shutdown(fd, SHUT_WR);
                state = SHUTDOWN;
            }
        }
        return true;
    }
};
#endif
