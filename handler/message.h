#pragma once

#include <chrono>
#include <functional>

namespace es {
class Message {
public:
    using TimePoint_t      = std::chrono::steady_clock::time_point;
    using Clock_t          = std::chrono::steady_clock;
    using MillisDuration_t = std::chrono::milliseconds;

    int                   what;
    int                   m_arg1;
    int                   m_arg2;
    std::function<void()> task;
    TimePoint_t           when;

public:
    Message();
    Message(int what);
    Message(int what, long delayMillis);
    virtual ~Message();

    Message(const Message &msg);
    Message(Message &&msg);
    Message &operator=(const Message &msg);
    Message &operator=(Message &&msg);

    void setWhen(long delayMillis);

    void onRun(std::function<void()> &&f);

    bool operator>(const Message &msg) const { return (this->when > msg.when); }

    bool operator<(const Message &msg) const { return (this->when < msg.when); }

    bool operator==(const Message &msg) const {
        return (this->what == msg.what) && (this->task != nullptr) && (msg.task != nullptr);
    }

    bool operator==(int what) const { return (this->what == what); }
};

} // namespace es
