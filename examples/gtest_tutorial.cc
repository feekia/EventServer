#include <gtest/gtest.h>
#include <iostream>
#include <list>
#include <queue>
#include <set>
#include <stack>
#include <unordered_set>
#include <vector>
#include <deque>

using namespace std;

int add(int a, int b) { return a + b; }

int minus2(int a, int b) { return a - b; }

TEST(testCase, test0) { EXPECT_EQ(add(2, 3), 5); }
TEST(testCase, tes1) { EXPECT_EQ(minus2(2, 3), 2); }

class SetElement {
public:
    int id_;
    int timer;
    SetElement(int id) : id_(id), timer(0) {}
    ~SetElement() {}
    void setId(int id) { id_ = id; }
    bool operator<(const SetElement &t2) const { return id_ < t2.id_; }
    bool operator>(const SetElement &t2) const { return id_ > t2.id_; }
};

template <typename T>
class Compare {
public:
    bool operator()(const T &t1, const T &t2) { return t1.id_ < t2.id_; }
};

int testElements() {
    // priority_queue 采用堆排序实现
    priority_queue<SetElement, deque<SetElement>, greater<SetElement>> elments;
    elments.push(144);
    elments.push(12);
    elments.push(15);
    elments.push(19);
    elments.push(32);

    while (!elments.empty()) {
        auto temp = elments.top();
        elments.pop();
        cout <<"deque container: " << temp.id_ << endl;
    }
    return 0;
}

int testStack() {
    // priority_queue 采用堆排序实现
    stack<int> ss;
    ss.push(144);
    ss.push(12);
    ss.push(15);
    ss.push(19);
    ss.push(32);

    while (!ss.empty()) {
        auto temp = ss.top();
        ss.pop();
        cout << temp << endl;
    }
    return 0;
}

int testUnorderedSet() {
    // priority_queue 采用堆排序实现
    unordered_multiset<int> ss;
    ss.insert(144);
    ss.insert(12);
    ss.insert(15);
    ss.insert(19);
    ss.insert(32);
    ss.insert(32);
    int cnt = ss.count(32);
    cout << "cnt : " << cnt << endl;
    unordered_multiset<int>::iterator it = ss.find(32);
    for (int count = ss.count(32); count > 0; count--, it++) {
        cout << "e: " << *it << endl;
    }

    return 0;
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    testElements();
    testStack();
    testUnorderedSet();
    return RUN_ALL_TESTS();
}