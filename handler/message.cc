#include "message.h"
#include <chrono>
#include <iostream>
using namespace std;
namespace es {
Message::Message() : Message(-1, 0) {}

Message::Message(int what) : Message(what, 0) {}

Message::Message(int what, long delayMillis) : what(what), when(Clock_t::now() + MillisDuration_t(delayMillis)) {
    task = nullptr;
}

void Message::setWhen(long delayMillis) { when = Clock_t::now() + MillisDuration_t(delayMillis); }

void Message::onRun(std::function<void()> &&f) { this->task = f; }
Message::~Message() {}

Message::Message(const Message &msg) : what(msg.what), task(msg.task), when(msg.when) {}

Message::Message(Message &&msg) : what(msg.what), task(msg.task), when(msg.when) {}

Message &Message::operator=(const Message &msg) {
    this->what = msg.what;
    this->when = msg.when;
    this->task = msg.task;

    return *this;
}

Message &Message::operator=(Message &&msg) {
    this->what = msg.what;
    this->when = msg.when;
    this->task = std::move(msg.task);
    return *this;
}
} // namespace es