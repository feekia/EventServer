#if 0
#include <inttypes.h>
#include <poll.h>
#include <set>
#include <sys/epoll.h>

#include "poller.h"
namespace es {

class EPoller : public Poller {
private:
    int                                fd_;
    std::unordered_map<int, Channel *> liveChannels_;
    // for epoll selected active events
    struct epoll_event activeEvs_[kMaxEvents];

public:
    EPoller();
    ~EPoller();

    void add(Channel *ch) override;
    void remove(Channel *ch) override;
    void update(Channel *ch) override;
    void looponce(Channel *ch) override;
};

Poller *createPoller() { return new EPoller(); }

EPoller::EPoller() {
    fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (fd_ < 0) {
        cout << "epoll_create error " << errno << " " << strerror(errno) << endl;
        return;
    }

    info("poller epoll %d created", fd_);
}

EPoller::~EPoller() {
    info("destroying poller %d", fd_);
    while (liveChannels_.size()) {
        (*liveChannels_.begin())->close();
    }
    for(auto &[k,v]:liveChannels_){
        v->closeSafty();
    }
    ::close(fd_);
    info("poller %d destroyed", fd_);
}

void EPoller::add(Channel *ch) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events   = ch->events();
    ev.data.ptr = ch;
    trace("adding channel %lld fd %d events %d epoll %d", (long long)ch->id(), ch->fd(), ev.events, fd_);
    int r = epoll_ctl(fd_, EPOLL_CTL_ADD, ch->fd(), &ev);
    fatalif(r, "epoll_ctl add failed %d %s", errno, strerror(errno));
    liveChannels_.insert(ch);
}

void EPoller::update(Channel *ch) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events   = ch->events();
    ev.data.ptr = ch;
    trace("modifying channel %lld fd %d events read %d write %d epoll %d", (long long)ch->id(), ch->fd(),
          ev.events & POLLIN, ev.events & POLLOUT, fd_);
    int r = epoll_ctl(fd_, EPOLL_CTL_MOD, ch->fd(), &ev);
    fatalif(r, "epoll_ctl mod failed %d %s", errno, strerror(errno));
}

void EPoller::remove(Channel *ch) {
    trace("deleting channel %lld fd %d epoll %d", (long long)ch->id(), ch->fd(), fd_);
    liveChannels_.erase(ch);
    for (int i = lastActive_; i >= 0; i--) {
        if (ch == activeEvs_[i].data.ptr) {
            activeEvs_[i].data.ptr = NULL;
            break;
        }
    }
}

void EPoller::looponce(int waitMs) {
    int64_t ticks = util::timeMilli();
    lastActive_   = epoll_wait(fd_, activeEvs_, kMaxEvents, waitMs);
    int64_t used  = util::timeMilli() - ticks;
    trace("epoll wait %d return %d errno %d used %lld millsecond", waitMs, lastActive_, errno, (long long)used);
    fatalif(lastActive_ == -1 && errno != EINTR, "epoll return error %d %s", errno, strerror(errno));
    while (--lastActive_ >= 0) {
        int      i      = lastActive_;
        Channel *ch     = (Channel *)activeEvs_[i].data.ptr;
        int      events = activeEvs_[i].events;
        if (ch) {
            if (events & (kReadEvent | POLLERR)) {
                trace("channel %lld fd %d handle read", (long long)ch->id(), ch->fd());
                ch->handleRead();
            } else if (events & kWriteEvent) {
                trace("channel %lld fd %d handle write", (long long)ch->id(), ch->fd());
                ch->handleWrite();
            } else {
                fatal("unexpected poller events");
            }
        }
    }
}

} // namespace es

#endif 