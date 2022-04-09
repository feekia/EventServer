#include "tcp_connection.h"
#include "buffer.h"
#include "channel.h"
#include "logging.h"

namespace es {
void TcpConnection::handleRead(const TcpConnectionPtr &con) {
    while (true) {
        input_.makeRoom();
        int rd = 0;
        if (chan_->fd() >= 0) {
            rd = read(chan_->fd(), input_.end(), input_.space());
            spdlog::debug("channel {} fd {} readed {} bytes", (long long)chan_->id(), chan_->fd(), rd);
        }
        if (rd == -1 && errno == EINTR) {
            continue;
        } else if (rd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (readcb_ && input_.size()) {
                readcb_(con);
            }
            updateFreeTimeOut(con);
            break;
        } else if (chan_->fd() == -1 || rd == 0 || rd == -1) {
            cleanup(con);
            break;
        } else { // rd > 0
            input_.addSize(rd);
        }
    }
}
void TcpConnection::handleWrite(const TcpConnectionPtr &con) {
    ssize_t sended = write(output_.begin(), output_.size());
    output_.consume(sended);
    if (output_.empty() && writablecb_) {
        writablecb_(con);
    }
    if (output_.empty() && chan_->writeEnabled()) { // writablecb_ may write something
        chan_->enableWrite(false);
    }
}

ssize_t TcpConnection::write(const char *buf, size_t len) {
    size_t sended = 0;
    while (len > sended) {
        ssize_t wd = ::write(chan_->fd(), buf + sended, len - sended);
        spdlog::debug("channel {} fd {} write {} bytes", (long long)chan_->id(), chan_->fd(), wd);
        if (wd > 0) {
            sended += wd;
            continue;
        } else if (wd == -1 && errno == EINTR) {
            continue;
        } else if (wd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (!chan_->writeEnabled()) {
                chan_->enableWrite(true);
            }
            break;
        } else {
            spdlog::error("write error: channel {} fd {} wd {} {} {}", (long long)chan_->id(), chan_->fd(), wd, errno,
                          strerror(errno));
            break;
        }
    }
    return sended;
}

void TcpConnection::send(Buffer &buf) {
    if (chan_) {
        if (chan_->writeEnabled()) {
            output_.append(buf);
        }
        if (buf.size()) {
            ssize_t sended = write(buf.begin(), buf.size());
            buf.consume(sended);
        }
        if (buf.size()) {
            output_.append(buf);
            if (!chan_->writeEnabled()) {
                chan_->enableWrite(true);
            }
        }
    } else {
        spdlog::warn("connection closed, but still writing {} bytes", buf.size());
    }
}

void TcpConnection::send(const char *buf, size_t len) {
    if (chan_) {
        if (output_.empty()) {
            ssize_t sended = write(buf, len);
            buf += sended;
            len -= sended;
        }
        if (len) {
            output_.append(buf, len);
        }
    } else {
        spdlog::warn("connection closed, but still writing {} bytes", len);
    }
}
void TcpConnection::attachIoOps(IoOps *ops) {
    ops_  = ops;
    chan_ = new Channel(ops->getPlexer(), fd_, kWriteEvent | kReadEvent);

    TcpConnectionPtr tcpcon = shared_from_this();
    chan_->onRead([=] { tcpcon->handleRead(tcpcon); });
    chan_->onWrite([=] { tcpcon->handleWrite(tcpcon); });
    if (freecb_ != nullptr) {
        stringstream ss;
        string       n;
        ss << "con:{" << fd_ << "}";
        ss >> n;
        idle_task_ = make_shared<IdleTask>(std::move(n));
        idle_task_->setSchedDelay(free_timeout_ms_);
        idle_task_->setRepeatPeriod(free_timeout_ms_);
        idle_task_->onRun([=](IdleTaskPtr &t) { tcpcon->handleFreeTimeOut(tcpcon); });
        ops->addIdleTask(idle_task_);
    }
}

void TcpConnection::cleanup(const TcpConnectionPtr &con) {
    if (readcb_ && input_.size()) {
        readcb_(con);
    }

    if (statecb_) {
        statecb_(con);
    }

    // channel may have hold TcpConnPtr, set channel_ to NULL before delete
    readcb_ = writablecb_ = statecb_ = freecb_ = nullptr;
    if (idle_task_ != nullptr) idle_task_->cancel();
    idle_task_  = nullptr;
    Channel *ch = chan_;
    chan_       = NULL;
    delete ch;
}

void TcpConnection::close() {
    TcpConnectionPtr con = shared_from_this();
    ops_->sendAndWakeup([con](){
        if(con->chan()!=nullptr){
            con->chan()->close();
        }
    });
}
} // namespace es
