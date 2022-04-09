#pragma once
#include <string.h>
#include <string>
#include <vector>

namespace es {

class Slice {
public:
    Slice() : pbegin_("") { pend_ = pbegin_; }
    Slice(const char *b, const char *e) : pbegin_(b), pend_(e) {}
    Slice(const char *d, size_t n) : pbegin_(d), pend_(d + n) {}
    Slice(const std::string &s) : pbegin_(s.data()), pend_(s.data() + s.size()) {}
    Slice(const char *s) : pbegin_(s), pend_(s + strlen(s)) {}

    const char *data() const { return pbegin_; }
    const char *begin() const { return pbegin_; }
    const char *end() const { return pend_; }
    char        front() { return *pbegin_; }
    char        back() { return pend_[-1]; }
    size_t      size() const { return pend_ - pbegin_; }
    void        resize(size_t sz) { pend_ = pbegin_ + sz; }
    inline bool empty() const { return pend_ == pbegin_; }
    void        clear() { pend_ = pbegin_ = ""; }

    // return the eated data
    Slice eatWord();
    Slice eatLine();
    Slice eat(int sz) {
        Slice s(pbegin_, sz);
        pbegin_ += sz;
        return s;
    }
    Slice sub(int boff, int eoff = 0) const {
        Slice s(*this);
        s.pbegin_ += boff;
        s.pend_ += eoff;
        return s;
    }
    Slice &trimSpace();

    inline char operator[](size_t n) const { return pbegin_[n]; }

    std::string toString() const { return std::string(pbegin_, pend_); }
    // Three-way comparison.  Returns value:
    int compare(const Slice &b) const;

    // Return true if "x" is a prefix of "*this"
    bool startsWith(const Slice &x) const { return (size() >= x.size() && memcmp(pbegin_, x.pbegin_, x.size()) == 0); }

    bool endWith(const Slice &x) const {
        return (size() >= x.size() && memcmp(pend_ - x.size(), x.pbegin_, x.size()) == 0);
    }

    operator std::string() const { return std::string(pbegin_, pend_); }

    std::vector<Slice> split(char ch) const;

private:
    const char *pbegin_;
    const char *pend_;
};

inline Slice Slice::eatWord() {
    const char *b = pbegin_;
    while (b < pend_ && isspace(*b)) {
        b++;
    }
    const char *e = b;
    while (e < pend_ && !isspace(*e)) {
        e++;
    }
    pbegin_ = e;
    return Slice(b, e - b);
}

inline Slice Slice::eatLine() {
    const char *p = pbegin_;
    while (pbegin_ < pend_ && *pbegin_ != '\n' && *pbegin_ != '\r') {
        pbegin_++;
    }
    return Slice(p, pbegin_ - p);
}

inline Slice &Slice::trimSpace() {
    while (pbegin_ < pend_ && isspace(*pbegin_))
        pbegin_++;
    while (pbegin_ < pend_ && isspace(pend_[-1]))
        pend_--;
    return *this;
}

inline bool operator<(const Slice &x, const Slice &y) { return x.compare(y) < 0; }

inline bool operator==(const Slice &x, const Slice &y) {
    return ((x.size() == y.size()) && (memcmp(x.data(), y.data(), x.size()) == 0));
}

inline bool operator!=(const Slice &x, const Slice &y) { return !(x == y); }

inline int Slice::compare(const Slice &b) const {
    size_t    sz = size(), bsz = b.size();
    const int min_len = (sz < bsz) ? sz : bsz;

    int r = memcmp(pbegin_, b.pbegin_, min_len);
    if (r == 0) {
        if (sz < bsz)
            r = -1;
        else if (sz > bsz)
            r = +1;
    }
    return r;
}

inline std::vector<Slice> Slice::split(char ch) const {
    std::vector<Slice> r;
    const char *       pb = pbegin_;
    for (const char *p = pbegin_; p < pend_; p++) {
        if (*p == ch) {
            r.push_back(Slice(pb, p));
            pb = p + 1;
        }
    }
    if (pend_ != pbegin_) r.push_back(Slice(pb, pend_));
    return r;
}

} // namespace es
