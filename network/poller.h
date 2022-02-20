#pragma once

#include "channel.h"
#include <atomic>
#include <inttypes.h>


namespace es {
constexpr int kMaxEvents  = 2000;
constexpr int kReadEvent  = POLLIN;
constexpr int kWriteEvent = POLLOUT;
class Poller {
public:
    Poller() : last_active_(-1) {
        static std::atomic<int64_t> id(0);
        id_ = ++id;
    }
    virtual void add(Channel *ch)      = 0;
    virtual void remove(Channel *ch)   = 0;
    virtual void update(Channel *ch)   = 0;
    virtual void looponce(Channel *ch) = 0;

    virtual ~Poller(){};

private:
    int64_t id_;
    int     last_active_;
};

Poller *createPoller();

} // namespace es