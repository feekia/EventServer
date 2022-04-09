#pragma once
#include "event_loop.h"
#include <atomic>
#include <thread>
#include <vector>

using namespace std;
namespace es {
class EventWorkGroup {
private:
    int               size_;
    atomic<int>       id_;
    vector<thread>    workers_;
    vector<EventLoop> loops_;

public:
    EventWorkGroup(int size) : size_(size), id_(0), workers_(size), loops_(size) {}
    ~EventWorkGroup() {}

    vector<EventLoop> *getLoops() { return &loops_; }

    EventLoop *getLoop() { return &loops_[id_++ % loops_.size()]; }

    EventLoop *getLoop(int index) { return &loops_[index]; }

    void loop() {
        for (int i = 0; i < size_; i++) {
            workers_[i] = thread([this, i] { loops_[i].loop(); });
        }
    }

    int size() { return size_; }

    void sync() {
        for (int i = 0; i < size_; i++) {
            if (workers_[i].joinable()) workers_[i].join();
        }
    }
    void exit() {
        for (auto &loop : loops_) {
            loop.exit(true);
        }
    }
};
} // namespace es
