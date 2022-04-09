#pragma once
#include "channel.h"
#include "event_loop.h"
#include "idle_task.h"
#include "logging.h"
#include "multi_plexer.h"
#include "netutils.h"
#include <assert.h>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <unistd.h>

namespace es {
using IOTask = std::function<void()>;
class IoOps {
private:
    int           wakerFds_[2];
    int64_t       nextTimeoutMs_;
    list<IOTask>  tasks_;
    mutex         mtx;
    EventLoop *   loop_;
    MultiPlexer * plexer_;
    IdleTaskQueue idleTaskQueue;

public:
    IoOps(EventLoop *loop) : nextTimeoutMs_(1 << 30), loop_(loop), plexer_(new MultiPlexer) {
        loop_->onWork([=](int waitMs) { poll(waitMs); });
        loop_->onExitNotify([=]() { wakeup(); });
        loop_->onClean([=]() { close(); });
        init();
    }
    ~IoOps() { close(); }

    MultiPlexer *getPlexer() { return plexer_; }

    void init();
    void wakeup() {
        int r = write(wakerFds_[1], "", 1);
        assert(r >= 0);
    }

    void addIdleTask(const IdleTaskPtr &t) { idleTaskQueue.push(t); }

    void poll(int64_t waitms) {
        int64_t minWaitMs = std::min(nextTimeoutMs_, waitms);
        plexer_->poll(minWaitMs);
        handleTimeouts();
    }

    void sendAndWakeup(IOTask &&t) {
        lock_guard<mutex> lk(mtx);
        tasks_.push_back(std::move(t));
        int r = write(wakerFds_[1], "", 1);
        assert(r >= 0);
    }
    void handleTimeouts();

    void close() {
        if (wakerFds_[1] >= 0) {
            ::close(wakerFds_[1]);
            wakerFds_[1] = -1;
        }
        while (!idleTaskQueue.empty()) {
            idleTaskQueue.pop();
        }
        if (plexer_ != nullptr) {
            delete plexer_;
            plexer_ = nullptr;
        }
    }
};

} // namespace es