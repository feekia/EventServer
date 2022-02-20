#pragma once
#ifndef MESSAGE_H
#define MESSAGE_H

#include <chrono>
#include <functional>

namespace es {
class Message {
public:
    int                           what;
    int                           m_arg1;
    int                           m_arg2;
    typedef std::function<void()> Function;
    Function                      task;

    std::chrono::steady_clock::time_point when;

public:
    Message();
    Message(int what);
    Message(int what, long delayMillis);
    virtual ~Message();

    Message &operator=(const Message &msg);

    void setWhen(long delayMillis);

    void onRun(std::function<void()> &&f);

    bool operator>(const Message &msg) const { return (this->when > msg.when); }

    bool operator<(const Message &msg) const { return (this->when < msg.when); }

    bool operator==(const Message &msg) const {
        return (this->what == msg.what) && (this->task != nullptr) && (msg.task != nullptr);
    }

    bool operator==(int what) const { return (this->what == what); }

private:
};

} // namespace es
#endif
