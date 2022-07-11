#include "accept_plexer.h"
#include "spdlog/cfg/env.h"  // support for loading levels from the environment variable
#include "spdlog/fmt/ostr.h" // support for user defined types
#include "spdlog/spdlog.h"
#include "spdlog/stopwatch.h"
#include <assert.h>

using namespace std;
namespace es {

AcceptMultiPlexer::AcceptMultiPlexer(vector<EventLoop> *loops) : loops_(loops), exit_(false) {

    assert(loops_->size() >= 1);
    epollfd_ = epoll_create1(EPOLL_CLOEXEC);
    assert(epollfd_ >= 0);
    spdlog::info("AcceptPlexer epoll created,fd: {}", epollfd_);

    if (loops_->size() == 1) {
        loops_->front().onWork([=](int64_t waitMs) {
            int cfd = -1;

            cfd = this->waitTimeout(waitMs);
            if (cfd >= 0) {
                this->acceptor->handleAccept(cfd);
            }
        });

        loops_->front().onExitNotify([=]() { onExit(); });
    } else {
        int size = static_cast<int>(loops_->size());
        for (int i = 0; i < size; i++) {
            loops_->at(i).onWork([=](int64_t waitMs) {
                int cfd = -1;
                cfd     = this->waitTimeoutWithLock(waitMs);
                if (cfd >= 0) {
                    if (!exit_) {
                        this->acceptor->handleAccept(cfd);
                    } else {
                        close(cfd);
                    }
                }
            });
            loops_->at(i).onExitNotify([=]() { onExit(); });
        }
    }
}

int AcceptMultiPlexer::waitTimeout(int64_t waitMs) {
    int cfd     = -1;
    int actives = epoll_wait(epollfd_, &activeEvs_, 1, waitMs);
    spdlog::info("waitTimeout actives {} ", actives);
    if (actives > 0) {
        int events = activeEvs_.events;
        assert(static_cast<bool>((events & EPOLLIN) > 0));
        struct sockaddr_in raddr;
        socklen_t          rsz = sizeof(raddr);

        cfd = accept(acceptor->fd(), (struct sockaddr *)&raddr, &rsz);
        spdlog::info("Listen fd {} accept connection fd {} !", acceptor->fd(), cfd);
    }
    // spdlog::info("Listen exit !");
    return cfd;
}
} // namespace es
