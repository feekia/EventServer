#pragma once
#include <functional>
#include <signal.h>
#include <unordered_map>

using namespace std;
namespace es {
template <typename _TPointer>
struct SpGreater {
    bool operator()(const _TPointer &a, const _TPointer &b) { return *a > *b; }
};

template <typename _TPointer>
struct SpLess {
    bool operator()(const _TPointer &a, const _TPointer &b) { return *a < *b; }
};

class Signal {
public:
    static void signal(int sig, const std::function<void()> &h) {
        handlers[sig] = h;
        ::signal(sig, signal_handler_wrapper);
    }
    static void signal_handler_wrapper(int sig) { handlers[sig](); }

    static std::unordered_map<int, std::function<void()>> handlers;
};
} // namespace es
