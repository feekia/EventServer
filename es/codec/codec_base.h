#pragma once
#include "buffer.h"
#include "tcp_connection.h"
#include <list>

namespace es {
class CodecBase {
private:
    /* data */
public:
    CodeBase(/* args */);
    ~CodeBase();

    virtual void decode(const TcpConnectionPtr &con, void *c);
    virtual void encode(const TcpConnectionPtr &con, void *c, list<Buffer> *out) {}
};

CodeBase::CodeBase(/* args */) {}

CodeBase::~CodeBase() {}
} // namespace es
