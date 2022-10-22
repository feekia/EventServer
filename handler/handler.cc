
#include "handler.h"
#include <algorithm>
#include <chrono>
#include <iostream>

namespace es {

void Handler::initLoop() {
    _looper = std::thread([this]() {
        while (true) {
            Message msg;
            bool isFired = false;
            {
                std::unique_lock<std::mutex> lock(_queue_lock);
                if (_msg_list.empty()) {
                    _cond.wait(lock, [this] { return _stoped || !_msg_list.empty(); });
                } else {
                    _cond.wait_until(lock, _msg_list.front().when,
                                     [this] { return _stoped || _msg_list.empty(); });
                }
                if (!_stoped && _msg_list.empty())
                    continue;
                if (_stoped) {
                    _msg_list.clear();
                    return;
                }
                if (_msg_list.front().when < Clock_t::now()) {
                    msg = std::move(_msg_list.front());
                    _msg_list.pop_front();
                    isFired = true;
                }
            }
            if (isFired) {
                dispatchMessage(msg);
            }
        }
    });
}

bool Handler::sendEmptyMessageDelay(int what, long delay_millis) {

    if (what < 0 || delay_millis < 0)
        return false;
    Message msg(what, delay_millis);

    std::unique_lock<std::mutex> lock(_queue_lock);
    _msg_list.push_back(msg);
    _msg_list.sort(std::less<Message>());
    _cond.notify_one();
    return true;
}

bool Handler::postDelay(std::function<void()> &&f, long delay_millis) {
    if (f == nullptr || delay_millis < 0) {
        return false;
    }
    std::unique_lock<std::mutex> lock(_queue_lock);
    Message msg(0, delay_millis);
    msg.onRun(std::move(f));
    _msg_list.push_back(msg);
    _msg_list.sort(std::less<Message>());
    _cond.notify_one();
    return true;
}

void Handler::dispatchMessage(const Message &msg) const {
    if (msg.task != nullptr) {
        msg.task();
    } else {
        if (msg.what < 0 || _callback == nullptr)
            return;
        _callback(msg);
    }
}

} // namespace es
