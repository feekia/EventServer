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

Buffer &Buffer::append(Buffer &buf) {
    if (&buf != this) {
        append(buf.begin(), buf.size());
        buf.clear();
    }
    return *this;
}

Buffer &Buffer::append(Buffer &&buf) {
    if (&buf != this) {
        if (size() == 0) {
            begin_    = buf.begin_;
            end_      = buf.end_;
            capacity_ = buf.capacity_;
            buffer_   = buf.buffer_;
            // clear
            buf.begin_    = 0;
            buf.end_      = 0;
            buf.capacity_ = 0;
            buf.buffer_   = nullptr;
        } else {
            append(buf.begin(), buf.size());
            buf.clear();
        }
    }
    return *this;
}

Buffer::Buffer(const Buffer &b) {
    begin_    = b.begin_;
    end_      = b.end_;
    capacity_ = b.capacity_;
    expand_   = b.expand_;
    if (b.buffer_) {
        buffer_ = new char[capacity_];
        memcpy(data(), b.begin(), b.size());
    }
}

Buffer::Buffer(Buffer &&b) {
    begin_    = b.begin_;
    end_      = b.end_;
    capacity_ = b.capacity_;
    expand_   = b.expand_;
    buffer_   = b.buffer_;

    b.begin_    = 0;
    b.end_      = 0;
    b.capacity_ = 0;
    b.buffer_   = nullptr;
}
} // namespace es