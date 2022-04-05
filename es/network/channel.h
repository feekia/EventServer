#pragma once

#include "logging.h"
#include "multi_plexer.h"
#include <cstdio>
#include <fcntl.h>
#include <functional>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

namespace es {
class MultiPlexer;
class Channel {
    using ChanTask = std::function<void()>;

private:
    MultiPlexer *         plexer_;
    int                   fd_;
    short                 events_;
    int64_t               id_;
    std::function<void()> readcb_, writecb_;

public:
    Channel(MultiPlexer *plexer, int fd, int events);
    ~Channel() { close(); }

    short   events() { return events_; }
    int     fd() { return fd_; }
    int64_t id() { return id_; }

    //挂接事件处理器
    void onRead(const ChanTask &readcb) { readcb_ = readcb; }
    void onWrite(const ChanTask &writecb) { writecb_ = writecb; }
    void onRead(ChanTask &&readcb) { readcb_ = std::move(readcb); }
    void onWrite(ChanTask &&writecb) { writecb_ = std::move(writecb); }

    //启用读写监听
    void enableRead(bool enable);
    void enableWrite(bool enable);
    void enableReadWrite(bool readable, bool writable);
    bool readEnabled() { return events_ & kReadEvent; }
    bool writeEnabled() { return events_ & kWriteEvent; }

    //处理读写事件
    void handleRead() { readcb_(); }
    void handleWrite() { writecb_(); }

    void close();
};

} // namespace es
