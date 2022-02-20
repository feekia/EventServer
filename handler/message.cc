#include "message.h"
#include <chrono>
namespace es {
Message::Message() : Message(-1, 0) {}

Message::Message(int what) : Message(what, 0) {}

Message::Message(int what, long delayMillis)
    : what(what), when(std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMillis)) {
    task = nullptr;
}

void Message::setWhen(long delayMillis) {
    when = std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMillis);
}

void Message::onRun(std::function<void()> &&f) { this->task = f; }
Message::~Message() {}

Message &Message::operator=(const Message &msg) {
    this->what = msg.what;
    this->when = msg.when;
    this->task = msg.task;

    return *this;
}
} // namespace es