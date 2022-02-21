#pragma once
#ifndef HANDLER_H
#define HANDLER_H

#include "message.h"
#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <thread>

using namespace std;
/*
 * Handler will run in it's own thread, you don't want to care about it.
 * Message will be proccess by the Handler. Two ways to add your task to the Handler.
 * 1. send message to the handler
 * 2. post the task(Function) to handler
 */
namespace es {
class Handler {
public:
    using TimePoint_t      = std::chrono::steady_clock::time_point;
    using Clock_t          = std::chrono::steady_clock;
    using MillisDuration_t = std::chrono::milliseconds;

    Handler();
    ~Handler();

    bool sendEmptyMessageDelay(int what);
    bool sendEmptyMessageDelay(int what, long delayMillis);
    bool post(function<void()> &&f);
    bool postDelay(function<void()> &&f, long delayMillis);
    void removeMessages(int what);
    void removeAlls();
    void stop();
    void handleMessage(function<void(const Message &msg)> &&cb) { callback = cb; }

private:
    void dispatchMessage(const Message &msg) const;

private:
    list<Message>                      msg_list;
    mutex                              queue_mutex;
    condition_variable                 condition;
    thread                             looper;
    bool                               is_stop;
    function<void(const Message &msg)> callback;
};
} // namespace es
#endif
