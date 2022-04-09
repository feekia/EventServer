#pragma once

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
    using Task             = std::function<void(const Message &msg)>;

    Handler();
    ~Handler() {
        is_stop = true;
        condition.notify_all();
        looper.join();
    }

    bool sendEmptyMessageDelay(int what, long delayMillis);
    bool sendEmptyMessageDelay(int what) { return sendEmptyMessageDelay(what, 0); }
    bool postDelay(function<void()> &&f, long delayMillis);
    bool post(std::function<void()> &&f) { return postDelay(std::move(f), 0); }
    void removeMessages(int what) {
        if (what < 0) return;

        std::unique_lock<std::mutex> lock(queue_mutex);
        if (!msg_list.empty()) msg_list.remove_if([what](const Message &m) { return m.what == what; });
    }
    void removeAlls() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (!msg_list.empty()) msg_list.clear();
    }
    void stop() {
        is_stop = true;
        condition.notify_one();
    }

    void handleMessage(Task &&cb) { callback = cb; }

private:
    void dispatchMessage(const Message &msg) const;

private:
    bool               is_stop;
    list<Message>      msg_list;
    mutex              queue_mutex;
    condition_variable condition;
    thread             looper;
    Task               callback;
};
} // namespace es