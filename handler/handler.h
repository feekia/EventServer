#pragma once
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

namespace es {
class Message {
public:
    using TimePoint_t      = std::chrono::steady_clock::time_point;
    using Clock_t          = std::chrono::steady_clock;
    using MillisDuration_t = std::chrono::milliseconds;

    int what;
    int m_arg1;
    int m_arg2;
    std::function<void()> task;
    TimePoint_t when;

public:
    Message() : Message(-1, 0) {}
    Message(int what) : Message(what, 0) {}
    Message(int what, long delayMillis)
        : what(what), when(Clock_t::now() + MillisDuration_t(delayMillis)) {
        task = nullptr;
    }
    Message(const Message &msg) : what(msg.what), task(msg.task), when(msg.when) {}
    Message(Message &&msg) : what(msg.what), task(msg.task), when(msg.when) {}

    ~Message() {}

    Message &operator=(const Message &msg) {
        this->what = msg.what;
        this->when = msg.when;
        this->task = msg.task;

        return *this;
    }

    Message &operator=(Message &&msg) {
        this->what = msg.what;
        this->when = msg.when;
        this->task = std::move(msg.task);
        return *this;
    }

    void setWhen(long delayMillis) { when = Clock_t::now() + MillisDuration_t(delayMillis); }

    void onRun(std::function<void()> &&f) { this->task = f; }

    bool operator>(const Message &msg) const { return (this->when > msg.when); }

    bool operator<(const Message &msg) const { return (this->when < msg.when); }

    bool operator==(const Message &msg) const {
        return (this->what == msg.what) && (this->task != nullptr) && (msg.task != nullptr);
    }

    bool operator==(int what) const { return (this->what == what); }
};

class Handler {
public:
    using TimePoint_t      = std::chrono::steady_clock::time_point;
    using Clock_t          = std::chrono::steady_clock;
    using MillisDuration_t = std::chrono::milliseconds;
    using Task             = std::function<void(const Message &msg)>;

    Handler() : _stoped(false) { initLoop(); }
    ~Handler() {
        _stoped = true;
        _cond.notify_all();
        _looper.join();
    }

    bool sendEmptyMessageDelay(int what, long delayMillis);
    bool sendEmptyMessageDelay(int what) { return sendEmptyMessageDelay(what, 0); }
    bool postDelay(std::function<void()> &&f, long delayMillis);
    bool post(std::function<void()> &&f) { return postDelay(std::move(f), 0); }
    void removeMessages(int what) {
        if (what < 0)
            return;

        std::unique_lock<std::mutex> lock(_queue_lock);
        if (!_msg_list.empty())
            _msg_list.remove_if([what](const Message &m) {
                return m.what == what;
            });
    }
    void removeAlls() {
        std::unique_lock<std::mutex> lock(_queue_lock);
        if (!_msg_list.empty())
            _msg_list.clear();
    }
    void stop() {
        _stoped = true;
        _cond.notify_one();
    }

    void handleMessage(Task &&cb) { _callback = cb; }

private:
    void dispatchMessage(const Message &msg) const;
    void initLoop();

private:
    bool _stoped;
    std::list<Message> _msg_list;
    std::mutex _queue_lock;
    std::condition_variable _cond;
    std::thread _looper;
    Task _callback;
};

using HandlerPtr = std::shared_ptr<Handler>;

} // namespace es
