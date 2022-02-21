
#include "handler.h"
#include "message.h"
#include <algorithm>
#include <chrono>
#include <iostream>

namespace es {
Handler::Handler() : is_stop(false) {
    looper = std::thread([this]() {
        for (;;) {
            Message msg;
            bool    isFired = false;
            {
                std::unique_lock<std::mutex> lock(this->queue_mutex);
                if (this->msg_list.empty()) {
                    this->condition.wait(lock, [this] { return this->is_stop || !this->msg_list.empty(); });
                } else {
                    this->condition.wait_until(lock, this->msg_list.front().when,
                                               [this] { return this->is_stop || this->msg_list.empty(); });
                }

                if (!is_stop && this->msg_list.empty()) continue;
                if (is_stop) {
                    msg_list.clear();
                    return;
                }

                if (msg_list.front().when < Clock_t::now()) {
                    msg = std::move(msg_list.front());
                    msg_list.pop_front();
                    isFired = true;
                }
            }
            if (isFired) this->dispatchMessage(msg);
        }
    });
}
Handler::~Handler() {
    is_stop = true;
    condition.notify_all();
    looper.join();
}

bool Handler::sendEmptyMessageDelay(int what) { return sendEmptyMessageDelay(what, 0); }

bool Handler::sendEmptyMessageDelay(int what, long delayMillis) {

    if (what < 0 || delayMillis < 0) return false;
    Message msg(what, delayMillis);

    std::unique_lock<std::mutex> lock(queue_mutex);
    msg_list.push_back(msg);
    // 跟进时间进行降序排列
    msg_list.sort(std::less<Message>());
    condition.notify_one();
    return true;
}

bool Handler::post(std::function<void()> &&f) { return postDelay(std::move(f), 0); }
bool Handler::postDelay(std::function<void()> &&f, long delayMillis) {

    if (f == nullptr || delayMillis < 0) {
        return false;
    }

    std::unique_lock<std::mutex> lock(queue_mutex);
    Message                      msg(0, delayMillis);
    msg.onRun(std::move(f));
    msg_list.push_back(msg);
    msg_list.sort(std::less<Message>());
    return true;
}

void Handler::removeMessages(int what) {
    if (what < 0) return;

    std::unique_lock<std::mutex> lock(queue_mutex);
    msg_list.remove_if([what](const Message &m) { return m.what == what; });
}

void Handler::removeAlls() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    msg_list.clear();
}

void Handler::stop() {
    is_stop = true;
    condition.notify_one();
}

void Handler::dispatchMessage(const Message &msg) const {
    if (msg.task != nullptr) {
        msg.task();
    } else {
        if (msg.what < 0 || callback == nullptr) return;
        callback(msg);
    }
}

} // namespace es