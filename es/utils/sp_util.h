#pragma once

template<typename _TPointer>
struct SpGreater {
    bool operator()(const _TPointer &a, const _TPointer &b) { return *a > *b; }
};

template<typename _TPointer>
struct SpLess {
    bool operator()(const _TPointer &a, const _TPointer &b) { return *a < *b; }
};