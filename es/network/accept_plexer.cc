#include "accept_plexer.h"
#include "spdlog/cfg/env.h"  // support for loading levels from the environment variable
#include "spdlog/fmt/ostr.h" // support for user defined types
#include "spdlog/spdlog.h"
#include "spdlog/stopwatch.h"
#include <assert.h>

using namespace std;
namespace es {

AcceptMultiPlexer::AcceptMultiPlexer(vector<EventLoop> *loops) : loops_(loops) {

    assert(loops_->size() >= 1);
    epollfd_ = epoll_create1(EPOLL_CLOEXEC);
    assert(epollfd_ >= 0);
    spdlog::info("AcceptPlexer epoll {} created", epollfd_);

    if (loops_->size() == 1) {
        loops_->front().onWork([=](int64_t waitMs) {
            int cfd = -1;

            cfd = this->waitTimeout(waitMs);
            if (cfd >= 0) {
                this->acceptor->handleAccept(cfd);
            }
        });
    } else {
        for (int i = 0; i < loops_->size(); i++) {
            loops_->at(i).onWork([=](int64_t waitMs) {
                int cfd = -1;
                cfd     = this->waitTimeoutWithLock(waitMs);
                if (cfd >= 0) {
                    this->acceptor->handleAccept(cfd);
                }
            });
        }
    }
}

int AcceptMultiPlexer::waitTimeout(int64_t waitMs) {
    int cfd     = -1;
    int actives = epoll_wait(epollfd_, &activeEvs_, 1, waitMs);
    if (actives > 0) {
        int events = activeEvs_.events;
        assert(static_cast<bool>((events & EPOLLIN) > 0));
        struct sockaddr_in raddr;
        socklen_t          rsz = sizeof(raddr);

        cfd = accept(acceptor->fd(), (struct sockaddr *)&raddr, &rsz);
        spdlog::debug("Listen fd {} accept connection fd {} !", acceptor->fd(), cfd);
        if (events & EPOLLIN) {
            struct sockaddr_in raddr;
            socklen_t          rsz = sizeof(raddr);

            cfd = accept(acceptor->fd(), (struct sockaddr *)&raddr, &rsz);
            spdlog::debug("Listen fd {} accept connection fd {} !", acceptor->fd(), cfd);
        }
    }

    return cfd;
}
} // namespace es
