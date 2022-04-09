#pragma once
#include "accept_plexer.h"
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
class TcpServer;
using TcpServerPtr      = std::shared_ptr<TcpServer>;
using TcpCreateCallBack = std::function<void(const TcpConnectionPtr &)>;

class TcpServer {
private:
    TcpCreateCallBack    new_connection_cb_;
    AcceptMultiPlexerPtr accept_plexer_;
    AcceptHandler *      accept_handler_;
    int                  listen_fd_;
    vector<IoOps *>      io_ops_;
    int                  size_;

private:
    IoOps *getOps() {
        static std::atomic<int64_t> id(0);
        return io_ops_[id++ % size_];
    }

public:
    TcpServer(EventWorkGroup *b, EventWorkGroup *w, TcpCreateCallBack &&create_cb)
        : new_connection_cb_(std::move(create_cb)), accept_plexer_(new AcceptMultiPlexer(b->getLoops())),
          io_ops_(w->size()), size_(w->size()) {
        for (int i = 0; i < size_; i++) {
            io_ops_[i] = new IoOps(w->getLoop(i));
        }
    }
    ~TcpServer() {
        if (accept_handler_) {
            delete accept_handler_;
            accept_handler_ = nullptr;
        }
        for (int i = 0; i < size_; i++) {
            delete io_ops_[i];
            io_ops_[i] = nullptr;
        }
    }

    int bind(const std::string &host, unsigned short port, bool reusePort);

    static TcpServerPtr startServer(EventWorkGroup *b, EventWorkGroup *w, TcpCreateCallBack &&create_cb,
                                     const std::string &host, unsigned short port, bool reusePort) {
        TcpServerPtr p(new TcpServer(b, w, std::move(create_cb)));

        int r = p->bind(host, port, reusePort);
        if (r) {
            spdlog::error("bind to {}:{} failed {} {}", host.c_str(), port, errno, strerror(errno));
        }
        return r == 0 ? p : NULL;
    }
};
} // namespace es
