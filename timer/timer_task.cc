#include "timer_task.h"

namespace es {
TimerTask::TimerTask(function<void()> &&callback) : run(callback) {}

TimerTask::~TimerTask() {}
} // namespace es