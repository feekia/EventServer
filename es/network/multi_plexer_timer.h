#pragma once
#include <assert.h>
#include <atomic>
#include <functional>
#include <inttypes.h>

using namespace std;
namespace es {
using TimeOutTask = function<void()>;
class MultiPlexerTimer {
private:
    int64_t     id_;
    int64_t     at_;       // ms
    int64_t     internal_; // ms
    TimeOutTask task;

public:
    MultiPlexerTimer() : at_(0), internal_(0), task(nullptr) {
        static atomic<int64_t> id(0);
        id_ = id++;
    }
    MultiPlexerTimer(int64_t at, int64_t internal = 0, const TimeOutTask &t) : at_(at), internal_(internal), task(t) {
        static atomic<int64_t> id(0);
        id_ = id++;
    }

    MultiPlexerTimer(const MultiPlexerTimer &timer) {
        id_       = timer.id_;
        at_       = timer.at_;
        internal_ = timer.internal_;
        task      = timer.task;
    }
    ~MultiPlexerTimer() {}

    int64_t id() { return id_; }
    int64_t at() { return at_; }

    void setAt(int64_t when) { at_ = when; }
    void setInternal(int64_t internal) { internal_ = internal; }

    void onTimerWork(const TimeOutTask &t) { task = t; }
    void onTimerWork(TimeOutTask &&t) { task = std::move(t); }

    void handleTimerWork() {
        if (task != nullptr) task();
    }

    void updateTimerWork()

    bool operator==(const MultiPlexerTimer &t) { return id_ == t.id_; }
    bool operator>(const MultiPlexerTimer &t) { return at_ > t.at_; }
    bool operator<(const MultiPlexerTimer &t) { return at_ < t.at_; }
};

MultiPlexerTimer::MultiPlexerTimer(/* args */) {}

MultiPlexerTimer::~MultiPlexerTimer() {}

} // namespace es