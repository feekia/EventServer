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
    TcpCreateCallBack    newConnectionCb_;
    TcpCallBack          readcb_;
    TcpCallBack          statecb_;
    AcceptMultiPlexerPtr acceptPlexer_;
    AcceptHandler *      acceptHandler_;
    int                  listenFd_;
    vector<IoOps *>      io_ops_;
    int                  size;

private:
    IoOps *getOps() {
        static std::atomic<int64_t> id(0);
        return io_ops_[id++ % size];
    }

public:
    TcpServer(EventWorkGroup *b, EventWorkGroup *w)
        : acceptPlexer_(new AcceptMultiPlexer(b->getLoops())), io_ops_(w->size()), size(w->size()) {
        for (int i = 0; i < size; i++) {
            io_ops_[i] = new IoOps(w->getLoop());
        }
    }
    ~TcpServer() {
        if (acceptHandler_) {
            delete acceptHandler_;
            acceptHandler_ = nullptr;
        }
        for (int i = 0; i < size; i++) {
            delete io_ops_[i];
            io_ops_[i] = nullptr;
        }
    }

    void onNewConnection(TcpCreateCallBack &&cb) { newConnectionCb_ = std::move(cb); }
    void onConnectionRead(TcpCallBack &&cb) { readcb_ = std::move(cb); }
    void onConnectionChange(TcpCallBack &&cb) { statecb_ = std::move(cb); }

    int bind(const std::string &host, unsigned short port, bool reusePort);

    static TcpServerPtr startServer(EventWorkGroup *b, EventWorkGroup *w, const std::string &host, unsigned short port,
                                    bool reusePort);
};
} // namespace es
