
#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#pragma once

#include "buffer.h"
#include "event_raii.h"
#include <atomic>
#include <chrono>
#include <event.h>
#include <event2/event.h>
#include <mutex>

using namespace std;

namespace es {
using f_event_cb = void (*)(evutil_socket_t socket_fd, short events, void *ctx);
class SocketHolder;
enum socket_state { INIT, SHUTDOWN, CLOSE };

enum timeout_config {
    SHUTDOWNTIMEOUT = 5,
    WRITETIMEOUT    = 10,
    READTIMEOUT     = 20,
    CLOSETIMEOUT    = 10,
    HEARTBITTIMEOUT = 120
};
class Channel : public std::enable_shared_from_this<Channel> {
private:
    evutil_socket_t             fd;
    std::weak_ptr<SocketHolder> holder;
    std::mutex                  cMutex;
    std::atomic<bool>           stop;
    std::atomic<bool>           isClose;
    std::atomic<int8_t>         state;
    Buffer                      wBuf;
    Buffer                      rBuf;
    raii_event                  rwEvent;
    int64_t                     timestamp = 0;

public:
    Channel(std::weak_ptr<SocketHolder> &&h, evutil_socket_t _fd);
    ~Channel();

    void listenWatcher(raii_event &&events);

    void                     monitorEvent(short events, int64_t timeout);
    int32_t                  send(char *buffer, size_t l);
    void                     onDisconnect(evutil_socket_t fd);
    std::shared_ptr<Channel> share() { return shared_from_this(); }

    void onChannelRead(short events, void *ctx);
    void onChannelWrite(short events, void *ctx);
    void onChannelTimeout(short events, void *ctx);
    void handleEvent(short events);
    void closeSafty();

private:
    inline bool isHeartBrakeExpired() {
        std::chrono::system_clock::duration d   = std::chrono::system_clock::now().time_since_epoch();
        std::chrono::seconds                sec = std::chrono::duration_cast<std::chrono::seconds>(d);
        return (sec.count() - timestamp) > HEARTBITTIMEOUT;
    }
    inline uint64_t heartBrakeLeft() {
        std::chrono::system_clock::duration d   = std::chrono::system_clock::now().time_since_epoch();
        std::chrono::seconds                sec = std::chrono::duration_cast<std::chrono::seconds>(d);
        return (HEARTBITTIMEOUT - (sec.count() - timestamp));
    }

    inline void updateHearBrakeExpired() {
        std::chrono::system_clock::duration d   = std::chrono::system_clock::now().time_since_epoch();
        std::chrono::seconds                sec = std::chrono::duration_cast<std::chrono::seconds>(d);
        timestamp                               = sec.count();
    }

    void handleClose();
};

} // namespace es
#endif /* end of _CHANNEL_H_ */
