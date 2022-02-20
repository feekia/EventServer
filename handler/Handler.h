#pragma once
#ifndef HANDLER_H
#define HANDLER_H

#include <chrono>
#include <condition_variable>
#include <list>
#include <map>
#include <mutex>
#include <thread>

#include "message.h"

/*
 * Handler will run in it's own thread, you don't want to care about it.
 * Message will be proccess by the Handler. Two ways to add your task to the Handler.
 * 1. send message to the handler
 * 2. post the task(Function) to handler
 */
namespace es {
class Handler {
public:
    Handler();
    virtual ~Handler();

    bool sendMessageDelay(Message &msg, long delayMillis);
    bool sendMessage(Message &msg);
    bool sendEmptyMessageDelay(int what);
    bool sendEmptyMessageDelay(int what, long delayMillis);
    bool post(std::function<void()> &&f);
    bool postDelay(std::function<void()> &&f, long delayMillis);
    void removeMessages(int what);
    void removeAlls();
    void stop();

    virtual void handleMessage(Message &msg);

    /*
     * for msg list sorted when insert,  in ascending order !
     *
     */
    template <class T>
    class Compare {
    public:
        bool operator()(const T &t1, const T &t2) const { return (t1 < t2); }
    };

private:
    void dispatchMessage(Message &msg);

private:
    std::list<Message> msg_list;

    std::mutex              queue_mutex;
    std::condition_variable condition;
    std::thread             looper;
    bool                    is_stop;
};
} // namespace es
#endif
