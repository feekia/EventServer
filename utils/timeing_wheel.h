#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <list>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace es {

class TimeWheel;
// 定时器节点
class TimerNode {
public:
    TimerNode(int32_t id, int64_t base_ticks, int64_t timeout_ticks,
        const std::function<void()> &cb, bool is_repeated = false)
        : id_(id), base_ticks_(base_ticks), timeout_ticks_(timeout_ticks), cb_(cb),
          repeated_(is_repeated) {}

    int32_t id() const { return id_; }
    int64_t timeoutAtTicks() const { return timeout_ticks_ + base_ticks_; }
    int64_t timeoutTicks() const { return timeout_ticks_; }
    void run() { cb_(); }
    bool repeated() const { return repeated_; }
    std::function<void()> runnable() const { return cb_; }

private:
    int32_t id_;                // 定时器ID
    int64_t base_ticks_;        // 定时器起始的tick数
    int64_t timeout_ticks_;     // 定时器超时的tick 个数
    std::function<void()> cb_;  // 定时器回调函数
    bool repeated_;             // 定时器是否重复执行
    friend class TimeWheel;
};

// 时间轮
class TimeWheel {
public:
    TimeWheel(int32_t tickMs, int32_t wheelSize)
        : tickMs_(tickMs), wheelSize_(wheelSize), curPos_(0), ticks_(0), slots_(wheelSize),
          stop_(false), nextTimerId_(0) {}

    ~TimeWheel() {
        stop();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    // 添加定时器，返回定时器ID
    int32_t addTimer(int64_t timeout, const std::function<void()> &cb, bool repeated = false) {
        int64_t timeout_ticks = timeout / tickMs_;  // 计算定时器需要走多少个tick
        std::lock_guard<std::mutex> locker(lock_);
        // 在当前槽位添加定时器节点
        int32_t index = (curPos_ + timeout_ticks) % wheelSize_;
        TimerNode timer(nextTimerId_++, ticks_, timeout_ticks, cb, repeated);
        slots_[index].emplace_back(timer);
        node_map_[timer.id()] = std::make_pair(index, std::prev(slots_[index].end()));
        return timer.id();
    }

    // 移除定时器
    void removeTimer(int32_t timerId) {
        std::lock_guard<std::mutex> locker(lock_);
        if (node_map_.count(timerId) > 0) {
            slots_[node_map_[timerId].first].erase(node_map_[timerId].second);
            node_map_.erase(timerId);
        }
    }

    // 移除定时器
    int32_t reScheduleTimer(int32_t timerId, int64_t timeout, const std::function<void()> &cb,
        bool repeated = false) {
        bool use_old_id = false;
        std::lock_guard<std::mutex> locker(lock_);
        if (node_map_.count(timerId) > 0) {
            slots_[node_map_[timerId].first].erase(node_map_[timerId].second);
            node_map_.erase(timerId);
            use_old_id = true;
        }

        int64_t timeout_ticks = timeout / tickMs_;  // 计算定时器需要走多少个tick
        // 在当前槽位添加定时器节点
        int32_t index = (curPos_ + timeout_ticks) % wheelSize_;
        TimerNode timer(use_old_id ? timerId : nextTimerId_++, ticks_, timeout_ticks, cb, repeated);
        slots_[index].emplace_back(timer);
        node_map_[timer.id()] = std::make_pair(index, std::prev(slots_[index].end()));
        return timer.id();
    }

    // 启动时间轮
    void start() {
        worker_ = std::thread([&]() {
            while (!stop_) {
                auto start = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
                tick();
                auto end = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
                auto usetime = (end - start).count();
                if (usetime <= tickMs_) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(tickMs_ - usetime));
                    // std::this_thread::sleep_for(milliseconds(tickMs_));
                }
            }
        });
    }

    // 停止时间轮
    void stop() { stop_ = true; }

private:
    void tick() {
        std::lock_guard<std::mutex> locker(lock_);
        auto &slot = slots_[curPos_];
        // 执行当前槽位上的所有定时器
        for (auto it = slot.begin(); it != slot.end();) {
            if (it->timeoutAtTicks() > ticks_) {
                ++it;
            } else {
                it->run();
                node_map_.erase(it->id());
                if (it->repeated()) {
                    int64_t ticks = it->timeoutAtTicks() / tickMs_;
                    int32_t index = (curPos_ + ticks) % wheelSize_;
                    // slots_[index].emplace_back(it->id(), ticks_, it->timeoutTicks(),
                    // it->runnable(), true);
                    auto insert_it = slots_[index].emplace(slots_[index].end(), it->id(), ticks_,
                        it->timeoutTicks(), it->runnable(), true);
                    node_map_[it->id()] = std::make_pair(index, insert_it);
                }
                it = slot.erase(it);
            }
        }
        // 更新时间轮的当前槽位
        curPos_ = (curPos_ + 1) % wheelSize_;
        ++ticks_;
    }

private:
    const int64_t tickMs_;                     // 时间轮的tick间隔（毫秒）
    const int64_t wheelSize_;                  // 时间轮的槽数
    int32_t curPos_;                           // 时间轮当前槽位的索引
    std::vector<std::list<TimerNode>> slots_;  // 时间轮的槽
    std::mutex lock_;                          // 互斥锁
    std::atomic<bool> stop_;                   // 时间轮是否停止
    int32_t nextTimerId_;                      // 下一个定时器ID
    int64_t ticks_;                            // 时间轮已经走过的tick数
    std::unordered_map<int32_t, std::pair<int32_t, std::list<TimerNode>::iterator>>
        node_map_;  // node id 与 tick index 、node 的键值对
    std::thread worker_;
};

}  // namespace es
