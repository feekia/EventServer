#pragma once

#include "channel.h"
#include "logging.h"
#include "multi_plexer.h"
#include "netutils.h"
#include <assert.h>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <unistd.h>

using namespace std;

namespace es {
using WTask = std::function<void()>;
class Waker {
private:
    int         wakerFds_[2];
    list<WTask> tasks_;
    mutex       mtx;

public:
    Waker() {}
    ~Waker() { ::close(wakerFds_[1]); }

    void attach(MultiPlexer *plexer) {
        int r = pipe(wakerFds_);
        assert(r == 0);
        r = addFlag(wakerFds_[0], FD_CLOEXEC);
        assert(r == 0);
        r = addFlag(wakerFds_[1], FD_CLOEXEC);
        assert(r == 0);
        Channel *ch = new Channel(plexer, wakerFds_[0], kReadEvent);
        ch->onRead([=] {
            char buf[256];
            int  r = ch->fd() >= 0 ? ::read(ch->fd(), buf, sizeof buf) : 0;
            if (r > 0) {
                WTask task;
                while (!tasks_.empty()) {
                    {
                        lock_guard<mutex> lk(mtx);
                        task = tasks_.front();
                        tasks_.pop_front();
                    }

                    task();
                }
            } else if (r == 0) {
                delete ch;
            } else if (errno == EINTR) {
            } else {
                spdlog::critical("wakeup channel read error %d %d %s", r, errno, strerror(errno));
                assert(false);
            }
        });
    }
    void wakeup() {
        int r = write(wakerFds_[1], "", 1);
        assert(r >= 0);
    }

    void sendAndWakeup(WTask &&t) {
        lock_guard<mutex> lk(mtx);
        tasks_.push_back(std::move(t));
    }
};

} // namespace es