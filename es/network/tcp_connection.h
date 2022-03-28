#pragma once
#include "buffer.h"
#include "channel.h"
#include "idle_task.h"
#include "io_ops.h"

namespace es {
class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using TcpCallBack      = std::function<void(const TcpConnectionPtr &)>;
using MsgCallBack      = std::function<void(const TcpConnectionPtr &, Slice msg)>;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
private:
    int         fd_;
    Buffer      input_, output_;
    Channel *   chan_;
    TcpCallBack readcb_, writablecb_, statecb_;
    TcpCallBack freecb_;
    std::string destHost_, localIp_;
    int64_t     freeTimeoutMs_;
    IdleTaskPtr idleTask;

public:
    TcpConnection() {}
    ~TcpConnection() {}

    void attachIoOps(IoOps *ops, int fd);
    //发送数据
    void    sendOutput() { send(output_); }
    void    send(Buffer &msg);
    void    send(const char *buf, size_t len);
    void    send(const std::string &s) { send(s.data(), s.size()); }
    void    send(const char *s) { send(s, strlen(s)); }
    ssize_t write(const char *buf, size_t len);

    bool writable() { return chan_ ? chan_->writeEnabled() : false; }
    //数据到达时回调
    void onRead(const TcpCallBack &cb) { readcb_ = cb; };
    //当tcp缓冲区可写时回调
    void onWritable(const TcpCallBack &cb) { writablecb_ = cb; }
    // tcp状态改变时回调
    void onChange(const TcpCallBack &cb) { statecb_ = cb; }
    // tcp空闲超时回调
    void onFreeTimeOut(int64_t timeoutMs, const TcpCallBack &cb) {
        assert(timeoutMs > 0);
        assert(cb != nullptr);

        freecb_        = cb;
        freeTimeoutMs_ = timeoutMs;
    };

    void handleRead(const TcpConnectionPtr &con);
    void handleWrite(const TcpConnectionPtr &con);
    void handleFreeTimeOut(const TcpConnectionPtr &con) { con->freecb_(con); }

    void updateFreeTimeOut(const TcpConnectionPtr &con) {
        if (con->idleTask != nullptr) con->idleTask->updateIdleTime();
    }

    void cleanup(const TcpConnectionPtr &con);
};

} // namespace es
