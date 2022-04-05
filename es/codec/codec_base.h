#pragma once
#include "buffer.h"
#include "tcp_connection.h"
#include <list>

namespace es {
class CodecBase {
private:
    /* data */
public:
    CodecBase(/* args */);
    ~CodecBase();

    virtual int decode(const TcpConnectionPtr &con, void *c);
    virtual int encode(const TcpConnectionPtr &con, void *c, list<Buffer> *out) {}
};

CodecBase::CodecBase(/* args */) {}

CodecBase::~CodecBase() {}
} // namespace es
