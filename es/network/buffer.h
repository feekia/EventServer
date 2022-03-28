#pragma once

#include "slice.h"
#include <algorithm>
#include <string.h>
#include <string>

namespace es {

class Buffer {
public:
    Buffer() : buffer_(NULL), begin_(0), end_(0), capacity_(0), expand_(512) {}
    ~Buffer() { delete[] buffer_; }
    void clear() {
        delete[] buffer_;
        buffer_   = NULL;
        capacity_ = 0;
        begin_ = end_ = 0;
    }
    size_t size() const { return end_ - begin_; }
    bool   empty() const { return end_ == begin_; }
    char * data() const { return buffer_ + begin_; }
    char * begin() const { return buffer_ + begin_; }
    char * end() const { return buffer_ + end_; }
    char * makeRoom(size_t len);
    void   makeRoom() {
        if (space() < expand_) expand(0);
    }
    size_t space() const { return capacity_ - end_; }
    void   addSize(size_t len) { end_ += len; }
    char * allocRoom(size_t len) {
        char *p = makeRoom(len);
        addSize(len);
        return p;
    }
    Buffer &append(const char *p, size_t len) {
        memcpy(allocRoom(len), p, len);
        return *this;
    }
    Buffer &append(Slice slice) { return append(slice.data(), slice.size()); }
    Buffer &append(const char *p) { return append(p, strlen(p)); }

    template <class T>
    Buffer &appendValue(const T &v) {
        append((const char *)&v, sizeof v);
        return *this;
    }
    Buffer &consume(size_t len) {
        begin_ += len;
        if (size() == 0) clear();
        return *this;
    }
    Buffer &absorb(Buffer &buf);
    void    setSuggestSize(size_t sz) { expand_ = sz; }
    Buffer(const Buffer &b) {
        memcpy(this, &b, sizeof b);
        if (b.buffer_) {
            buffer_ = new char[capacity_];
            memcpy(data(), b.begin(), b.size());
        }
    }
    Buffer &operator=(const Buffer &b) {
        delete[] buffer_;
        buffer_ = NULL;
        memcpy(this, &b, sizeof b);
        if (b.buffer_) {
            buffer_ = new char[capacity_];
            memcpy(data(), b.begin(), b.size());
        }
        return *this;
    }
    operator Slice() { return Slice(data(), size()); }

private:
    char * buffer_;
    size_t begin_, end_, capacity_, expand_;

    void moveHead() {
        std::copy(begin(), end(), buffer_);
        end_ -= begin_;
        begin_ = 0;
    }
    void expand(size_t len);
};

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