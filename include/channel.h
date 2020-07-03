
#ifndef __CHANNEL_H
#define __CHANNEL_H

#pragma once

#include <mutex>
#include <atomic>

#include <event.h>
#include <event2/event.h>
#include "events.h"
#include "buffer.h"

using namespace std;
using f_event_cb = void (*)(evutil_socket_t socket_fd, short events, void *ctx);
class socketholder;
class channel:public std::enable_shared_from_this<channel>
{
private:
    evutil_socket_t fd;
    std::shared_ptr<socketholder> holder;
    std::mutex rMutex;
    std::mutex wMutex;
    std::atomic<bool> wFromStart; // 写状态，true：有正在写的任务，将数据添加到buffer末尾； false：没有写的任务，将数据写到socket中
    std::atomic<bool> rFromStart; // 读状态，true:有尚未读完的数据，将数据读到buffer中； false：没有需要读的数据，直接读出来处理。
    buffer wBuf;
    buffer rBuf;
    raii_event rEvent;
    raii_event wEvent;

public:
    channel(std::shared_ptr<socketholder> h, evutil_socket_t _fd);
    ~channel();

    void startWatcher();
    void addReadEvent(size_t timeout);
    void addWriteEvent(size_t timeout);
    void send(char *buffer, size_t l);
    void onDisconnect(evutil_socket_t fd);
    std::shared_ptr<channel> share()  
    {  
        return shared_from_this();  
    } 

    static void onRead(evutil_socket_t socket_fd, short events, void *ctx);
    static void onWrite(evutil_socket_t socket_fd, short events, void *ctx);
};
#endif

