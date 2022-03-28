
#include "io_ops.h"

namespace es {

void IoOps::init() {
    int r = pipe(wakerFds_);
    assert(r == 0);
    r = addFlag(wakerFds_[0], FD_CLOEXEC);
    assert(r == 0);
    r = addFlag(wakerFds_[1], FD_CLOEXEC);
    assert(r == 0);
    Channel *ch = new Channel(plexer_, wakerFds_[0], kReadEvent);

    // 使用ET 模式，需要一次性把所有数据读取完，否则下次无法唤醒线程
    ch->onRead([=] {
        int  rd    = 0;
        int  bytes = 0;
        char buf[256];
        while (true) {
            rd = ch->fd() >= 0 ? ::read(ch->fd(), buf, sizeof buf) : 0;
            if (rd == -1 && errno == EINTR) {
                continue;
            } else if (rd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                break;
            } else if (rd == 0 || rd == -1) { // 读取到0 ：fd已关闭，读取到-1，发生错误
                spdlog::critical("wakeup channel read error {} {} {}", r, errno, strerror(errno));
                delete ch;
                break;
            } else { // rd > 0
                bytes += rd;
            }
        }

        if (bytes == 0) return;

        IOTask task;
        while (!tasks_.empty()) { // 将队列全部处理完
            {
                lock_guard<mutex> lk(mtx);
                task = tasks_.front();
                tasks_.pop_front();
            }

            task();
        }
    });
}

void IoOps::handleTimeouts() {
    nextTimeoutMs_ = 1 << 30;
    while (!idleTaskQueue.empty()) {
        IdleTaskPtr task;
        int64_t     diffMs = 0;
        task               = idleTaskQueue.top();

        if (task->isCanceled()) {
            spdlog::info("Task id : {} has bean canceled , remove now !", idleTaskQueue.top()->getTaskId());
            idleTaskQueue.pop(); // 删除
            continue;
        }

        if (task->shouldUpdate()) {
            idleTaskQueue.pop(); // 删除
            task->reload();
            idleTaskQueue.push(task);
            continue;
        }

        diffMs = std::chrono::duration_cast<MillisDuration_t>(task->nextSchedTime() - Clock_t::now()).count();
        if (diffMs <= 0) {
            idleTaskQueue.pop(); // 删除
            task->run(task);
        } else {
            nextTimeoutMs_ = diffMs;
            break;
        }
    }
}
} // namespace es