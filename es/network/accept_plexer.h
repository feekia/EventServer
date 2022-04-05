#pragma once
#include "event_loop.h"
#include "real_lock.h"
#include <assert.h>
#include <atomic>
#include <fcntl.h>
#include <inttypes.h>
#include <map>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>


using namespace std;
namespace es {
class AcceptMultiPlexer;

using AcceptMultiPlexerPtr = std::shared_ptr<AcceptMultiPlexer>;
using AcceptTask           = std::function<void(int64_t)>;
class AcceptHandler {
private:
    int        lsfd_;
    short      events_;
    AcceptTask acceptcb_;

public:
    AcceptHandler(int lsfd) : lsfd_(lsfd), events_(EPOLLOUT) {}
    ~AcceptHandler() {
        if (lsfd_ >= 0) {
            ::close(lsfd_);
            lsfd_ = -1;
        }
    }

    short events() { return events_; }
    int   fd() { return lsfd_; }
    void  close() {
        if (lsfd_ >= 0) {
            ::close(lsfd_);
        }
    }
    void onAccept(AcceptTask &&task) { acceptcb_ = std::move(task); }
    void handleAccept(int fd) { acceptcb_(fd); }
};

class AcceptMultiPlexer {
private:
    vector<EventLoop> *loops_;
    RealLock           lk_;
    AcceptHandler *    acceptor;
    int                epollfd_;
    struct epoll_event activeEvs_;

public:
    AcceptMultiPlexer(vector<EventLoop> *loops);

    ~AcceptMultiPlexer() {
        acceptor->close();
        close(epollfd_);
    }

    int addAccept(AcceptHandler *accept) {
        struct epoll_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.events   = accept->events();
        ev.data.ptr = accept;
        acceptor    = accept;
        return epoll_ctl(epollfd_, EPOLL_CTL_ADD, accept->fd(), &ev);
    }

    int waitTimeout(int64_t waitMs);
    int waitTimeoutWithLock(int64_t waitMs) {
        int cfd = -1;
        if (lk_.try_lock(waitMs) == true) {
            cfd = waitTimeout(waitMs);
            lk_.unlock();
        }
        return cfd;
    }
};

} // namespace es
