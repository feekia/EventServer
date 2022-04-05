#include "buffer.h"

namespace es {
char *Buffer::makeRoom(size_t len) {
    if (end_ + len <= capacity_) {
    } else if (size() + len < capacity_ / 2) {
        moveHead();
    } else {
        expand(len);
    }
    return end();
}

void Buffer::expand(size_t len) {
    size_t ncap = std::max(expand_, std::max(2 * capacity_, size() + len));
    char * p    = new char[ncap];
    std::copy(begin(), end(), p);
    end_ -= begin_;
    begin_ = 0;
    delete[] buffer_;
    buffer_   = p;
    capacity_ = ncap;
}

Buffer &Buffer::absorb(Buffer &buf) {
    if (&buf != this) {
        if (size() == 0) {
            char b[sizeof buf];
            memcpy(b, this, sizeof b);
            memcpy(this, &buf, sizeof b);
            memcpy(&buf, b, sizeof b);
            std::swap(expand_, buf.expand_); // keep the origin expand_
        } else {
            append(buf.begin(), buf.size());
            buf.clear();
        }
    }
    return *this;
}

} // namespace es