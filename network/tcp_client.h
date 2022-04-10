#pragma once
#include "event_work_group.h"
#include "io_ops.h"
#include "netutils.h"
#include "tcp_connection.h"
#include <atomic>
#include <errno.h>
#include <fcntl.h>
#include <functional>
#include <inttypes.h>
#include <memory>
#include <vector>

namespace es {

class TcpClient;
using TcpClientPtr = std::shared_ptr<TcpClient>;

class TcpClient {
private:
    TcpCreateCallBack new_connection_cb_;
    vector<IoOps *>   io_ops_;
    int               size_;

private:
    IoOps *getOps() {
        static std::atomic<int64_t> id(0);
        return io_ops_[id++ % size_];
    }

public:
    TcpClient(EventWorkGroup *w, TcpCreateCallBack &&create_cb)
        : new_connection_cb_(std::move(create_cb)), io_ops_(w->size()), size_(w->size()) {
        for (int i = 0; i < size_; i++) {
            io_ops_[i] = new IoOps(w->getLoop(i));
        }
    }
    ~TcpClient() {
        for (int i = 0; i < size_; i++) {
            delete io_ops_[i];
            io_ops_[i] = nullptr;
        }
    }

    void attach(int fd) {
        TcpConnectionPtr tcp(new TcpConnection);
        IoOps *          ops = getOps();
        tcp->setFd(fd);
        new_connection_cb_(tcp);
        ops->sendAndWakeup([=]() { tcp->attachIoOps(ops); });
    }

    static TcpClientPtr startClient( EventWorkGroup *w, TcpCreateCallBack &&create_cb) {
        TcpClientPtr p(new TcpClient(w, std::move(create_cb)));
        w->loop();
        return p;
    }

};
} // namespace es
