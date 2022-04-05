#include "channel.h"
#include "netutils.h"
#include <assert.h>

namespace es {

Channel::Channel(MultiPlexer *plexer, int fd, int events) : plexer_(plexer), fd_(fd), events_(events) {
    static atomic<int64_t> id(0);

    int r = -1;
    id_   = ++id;

    r = setNonBlock(fd);
    assert(r == 0);
    plexer_->add(this);
    spdlog::info("Add channel {} fd {} success !", (long)id_, fd_);
}

void Channel::enableRead(bool enable) {
    if (enable) {
        events_ |= kReadEvent;
    } else {
        events_ &= ~kReadEvent;
    }
    plexer_->update(this);
}

void Channel::enableWrite(bool enable) {
    if (enable) {
        events_ |= kWriteEvent;
    } else {
        events_ &= ~kWriteEvent;
    }
    plexer_->update(this);
}

void Channel::enableReadWrite(bool readable, bool writable) {
    if (readable) {
        events_ |= kReadEvent;
    } else {
        events_ &= ~kReadEvent;
    }
    if (writable) {
        events_ |= kWriteEvent;
    } else {
        events_ &= ~kWriteEvent;
    }
    plexer_->update(this);
}
void Channel::close() {
    plexer_->remove(this);
    ::close(fd_);
    fd_      = -1;
    readcb_  = nullptr;
    writecb_ = nullptr;
}
} // namespace es