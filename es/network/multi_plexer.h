#pragma once

#include "channel.h"
#include "event_loop.h"
#include <assert.h>
#include <atomic>
#include <inttypes.h>
#include <set>
#include <string.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/types.h>

using namespace std;

namespace es {
class Channel;
constexpr int kMaxEvents  = 2000;
constexpr int kReadEvent  = EPOLLIN;
constexpr int kWriteEvent = EPOLLOUT;
constexpr int kErrorEvent = EPOLLERR;
class MultiPlexer {
public:
    MultiPlexer();
    ~MultiPlexer();

    void add(Channel *ch);
    void remove(Channel *ch);
    void update(Channel *ch);
    void poll(int64_t waitMs);

private:
    int64_t             id_;
    int                 actives_;
    int                 epollfd_;
    std::set<Channel *> chans_;
    struct epoll_event  max_events_[kMaxEvents];
};

} // namespace es