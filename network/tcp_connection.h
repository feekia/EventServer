#pragma once
#include "buffer.h"
#include "channel.h"
#include "idle_task.h"
#include "io_ops.h"
#include <assert.h>

namespace es {
class TcpConnection;
using TcpConnectionPtr  = std::shared_ptr<TcpConnection>;
using TcpCallBack       = std::function<void(const TcpConnectionPtr &)>;
using MsgCallBack       = std::function<void(const TcpConnectionPtr &, Slice msg)>;
using TcpCreateCallBack = std::function<void(const TcpConnectionPtr &)>;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    // Tcp连接的状态
    enum TcpState {
        kInvalid = 0,
        kHandshaking,
        kConnected,
        kClosed,
        kFailed,
    };

private:
    int         fd_;
    Buffer      input_, output_;
    Channel *   chan_;
    TcpState    state_;
    TcpCallBack readcb_, writablecb_, statecb_, freecb_;
    int64_t     free_timeout_ms_;
    IdleTaskPtr idle_task_;
    IoOps *     ops_;

public:
    TcpConnection() : fd_(-1), state_(kInvalid) {}
    ~TcpConnection() {}

    int fd() { return fd_; }

    void setFd(int fd) {
        assert(fd >= 0);
        fd_ = fd;
    }
    Channel *chan() { return chan_; }
    TcpState state() { return state_; }

    void attachIoOps(IoOps *ops);

    // 获取输入输出缓冲区
    Buffer &getReadBuffer() { return input_; }
    Buffer &getWriteBuffer() { return output_; }

    //发送数据
    void sendOutput() { send(output_); }
    void send(Buffer &msg);
    void send(const char *buf, size_t len);

    void sendByOtherThread(Buffer &&msg) {
        ops_->sendAndWakeup([&]() { send(msg); });
    }

    bool writable() { return chan_ ? chan_->writeEnabled() : false; }
    //数据到达时回调
    void onRead(const TcpCallBack &cb) { readcb_ = cb; };
    //当tcp缓冲区可写时回调
    void onWritable(const TcpCallBack &cb) { writablecb_ = cb; }
    // tcp状态改变时回调
    void onState(const TcpCallBack &cb) { statecb_ = cb; }
    // tcp空闲超时回调
    void onFreeTimeOut(int64_t timeoutMs, const TcpCallBack &cb) {
        assert(timeoutMs > 0);
        assert(cb != nullptr);

        freecb_          = cb;
        free_timeout_ms_ = timeoutMs;
    };

    void handleRead(const TcpConnectionPtr &con);
    void handleWrite(const TcpConnectionPtr &con);
    void handleFreeTimeOut(const TcpConnectionPtr &con) { con->freecb_(con); }

    void updateFreeTimeOut(const TcpConnectionPtr &con) {
        if (con->idle_task_ != nullptr) con->idle_task_->updateIdleTime();
    }

    void close();

private:
    ssize_t write(const char *buf, size_t len);
    void    cleanup(const TcpConnectionPtr &con);
};

} // namespace es
