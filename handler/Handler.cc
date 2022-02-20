
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
            {
                std::unique_lock<std::mutex> lock(this->queue_mutex);
                if (this->msg_list.empty()) {
                    this->condition.wait(lock, [this] { return this->is_stop || !this->msg_list.empty(); });
                } else {
                    this->condition.wait_until(lock, this->msg_list.back().when,
                                               [this] { return this->is_stop || !this->msg_list.empty(); });
                }

                if (!is_stop && this->msg_list.empty()) continue;
                if (is_stop) {
                    msg_list.clear();
                    return;
                }
                msg = std::move(msg_list.back());
                msg_list.pop_back();
            }
            this->dispatchMessage(msg);
        }
    });
}
Handler::~Handler() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        is_stop = true;
    }
    condition.notify_all();
    looper.join();
    msg_list.clear();
}

void Handler::handleMessage(Message &msg) {
    std::cout << "IN Handler " << __func__ << " what:" << msg.what << std::endl;
}

bool Handler::sendMessageDelay(Message &msg, long delayMillis) {
    if (delayMillis < 0) return false;

    msg.setWhen(delayMillis);

    std::unique_lock<std::mutex> lock(queue_mutex);
    auto                         it = std::find(msg_list.begin(), msg_list.end(), msg);
    msg_list.erase(it);

    msg_list.push_back(msg);
    // std::sort(msg_list.begin(), msg_list.end(),std::greater<Message>());
    msg_list.sort(std::greater<Message>());
    condition.notify_one();
    return true;
}
bool Handler::sendMessage(Message &msg) {
    return false;

    std::unique_lock<std::mutex> lock(queue_mutex);
    auto                         i = find(msg_list.begin(), msg_list.end(), msg);
    if (i != msg_list.end()) msg_list.erase(i);

    msg_list.push_back(msg);
    msg_list.sort(std::greater<Message>());
    condition.notify_one();
    return true;
}

bool Handler::sendEmptyMessageDelay(int what) { return sendEmptyMessageDelay(what, 0); }

bool Handler::sendEmptyMessageDelay(int what, long delayMillis) {

    if (what < 0 || delayMillis < 0) return false;

    Message msg(what);
    msg.setWhen(delayMillis);

    std::unique_lock<std::mutex> lock(queue_mutex);

    std::list<Message>::iterator i = find(msg_list.begin(), msg_list.end(), msg);
    if (i != msg_list.end()) {
        msg_list.erase(i);
    }

    msg_list.push_back(msg);
    //	std::sort(msg_list.begin(), msg_list.end(),ValComp<Message>());
    // 跟进时间进行降序排列
    // std::sort(msg_list.begin(), msg_list.end(),std::greater<Message>());
    msg_list.sort(std::greater<Message>());
    condition.notify_one();
    return true;
}

bool Handler::post(std::function<void()> &&f) { return postDelay(std::move(f), 0); }
bool Handler::postDelay(std::function<void()> &&f, long delayMillis) {

    if (f == nullptr || delayMillis < 0) {
        return false;
    }

    std::unique_lock<std::mutex> lock(queue_mutex);
    Message                      msg;
    msg.setWhen(delayMillis);
    msg.onRun(std::forward<std::function<void()>>(f));
    msg_list.push_back(msg);
    // std::sort(msg_list.begin(), msg_list.end(), std::greater<Message>());
    msg_list.sort(std::greater<Message>());
    return true;
}

void Handler::removeMessages(int what) {
    if (what < 0) return;

    std::unique_lock<std::mutex> lock(queue_mutex);

    auto i = find(msg_list.begin(), msg_list.end(), what);
    if (i != msg_list.end()) {
        msg_list.erase(i);
    }
}

void Handler::removeAlls() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    msg_list.clear();
}

void Handler::stop() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    is_stop = true;
}

void Handler::dispatchMessage(Message &msg) {
    if (msg.task != nullptr) {
        msg.task();
    } else {
        if (msg.what < 0) return;
        handleMessage(msg);
    }
}

} // namespace es