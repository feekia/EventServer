
#include <inttypes.h>
#include <set>
#include <sys/epoll.h>

#include "spdlog/cfg/env.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"
#include "spdlog/stopwatch.h"
#include <chrono>
#include <cstdio>

#include "channel.h"
#include "multi_plexer.h"

namespace es {

MultiPlexer::MultiPlexer() : actives_(-1) {
    static std::atomic<int64_t> id(0);
    id_      = ++id;
    epollfd_ = epoll_create1(EPOLL_CLOEXEC);
    assert(epollfd_ >= 0);
    spdlog::info("MultiPlexer created,fd: {}", epollfd_);
}

MultiPlexer::~MultiPlexer() {
    while (chans_.size()) {
        (*chans_.begin())->close();
    }
    ::close(epollfd_);
    spdlog::info("Epoll destroyed, fd: {}", epollfd_);
}

void MultiPlexer::add(Channel *ch) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events   = ch->events();
    ev.data.ptr = ch;
    spdlog::info("adding channel fd {} events {} epoll {}", ch->fd(), ch->events(), epollfd_);
    int r = epoll_ctl(epollfd_, EPOLL_CTL_ADD, ch->fd(), &ev);
    assert(r >= 0);
    chans_.insert(ch);
}

void MultiPlexer::update(Channel *ch) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events   = ch->events();
    ev.data.ptr = ch;
    spdlog::trace("modifying channel {} fd {} events read {} write {} epoll {}", (long long)ch->id(), ch->fd(),
                  ev.events & kReadEvent, ev.events & kWriteEvent, epollfd_);
    int r = epoll_ctl(epollfd_, EPOLL_CTL_MOD, ch->fd(), &ev);
    assert(r >= 0);
}

void MultiPlexer::remove(Channel *ch) {
    spdlog::info("deleting channel {} fd {} epoll {}", (long long)ch->id(), ch->fd(), epollfd_);
    chans_.erase(ch);
    for (int i = actives_; i >= 0; i--) {
        if (ch == max_events_[i].data.ptr) {
            max_events_[i].data.ptr = NULL;
            break;
        }
    }
}

void MultiPlexer::poll(int64_t waitMs) {
    spdlog::stopwatch sw;
    actives_ = epoll_wait(epollfd_, max_events_, kMaxEvents, waitMs);
    spdlog::debug("epoll wait {} return {} errno {} used {} second", waitMs, actives_, errno, sw);

    while (--actives_ >= 0) {
        int      i      = actives_;
        Channel *ch     = (Channel *)max_events_[i].data.ptr;
        int      events = max_events_[i].events;
        assert((events & (kReadEvent | kWriteEvent | kErrorEvent)) > 0);
        if (events & (kReadEvent | kErrorEvent)) {
            spdlog::info("channel {} fd {} handle read", (long long)ch->id(), ch->fd());
            ch->handleRead();
        } else if (events & kWriteEvent) {
            spdlog::info("channel {} fd {} handle write", (long long)ch->id(), ch->fd());
            ch->handleWrite();
        }
    }
}

} // namespace es
