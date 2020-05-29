#include <iostream>
#include <memory>
#include <vector>
#include<algorithm>
using namespace std;

class test
{
private:
    /* data */
public:
    test(/* args */);
    ~test();
};

test::test(/* args */)
{
    cout << "test" << endl;
}

test::~test()
{
    cout << "~test" << endl;
}


class RightTest
{
private:
    /* data */
    int value;
public:
    std::unique_ptr<test> ptr;
public:
    RightTest(/* args */);
    RightTest(int v);
    RightTest(RightTest&);
    RightTest(RightTest&&);
    ~RightTest();

    void setValue(int v);
    int getValue(void);
};

RightTest::RightTest(/* args */):value(23)
{
    cout << "RightTest construct" << endl;
}
RightTest::RightTest(int v):value(v)
{
    cout << "RightTest " <<  v << " construct" << endl;
}
RightTest::RightTest(RightTest& r)
{
    cout << "RightTest & construct" << endl;
    value = r.value;
}
RightTest::RightTest(RightTest&& r)
{
    cout << "RightTest && construct" << endl;
    value = r.value;
}
RightTest::~RightTest()
{
    cout << "~RightTest deconstruct" << endl;
}

void RightTest::setValue(int v){
    value = v;
}

int RightTest::getValue(void){
    return value;
}

RightTest getRightTest(){
    RightTest t;
    t.setValue(25);
    return t;
}

 void fun(int& x) { cout << "call lvalue ref" << endl; }
 void fun(int&& x) { cout << "call rvalue ref" << endl; }
 void fun(const int& x) { cout << "call const lvalue ref" << endl; }
 void fun(const int&& x) { cout << "call const rvalue ref" << endl; }
template<typename T>
 void PerfectForward(T&& t)
 {
     std::cout << "T is a ref type?: " << std::is_reference<T>::value << std::endl;
     std::cout << "T is a lvalue ref type?: " << std::is_lvalue_reference<T>::value << std::endl;
     std::cout << "T is a rvalue ref type?: " << std::is_rvalue_reference<T>::value << std::endl;
 
     fun(forward<T>(t));
 }

int main(){

    // RightTest t;
    // t.setValue(15);
    // cout << t.getValue() << endl;
    // RightTest tl{RightTest(t)};
    // RightTest tll{getRightTest()};
    
    // cout << tll.getValue() << endl;
    // // RightTest gRt = getRightTest();

    // PerfectForward(10);           // call rvalue ref

    // int a = 5;
    // PerfectForward(a);            // call lvalue ref
    // PerfectForward(move(a));      // call rvalue ref

    // const int b = 8;
    // PerfectForward(b);           // call const lvalue ref
    // PerfectForward(move(b));     // call const rvalue ref
    std::unique_ptr<RightTest> rtPtr{new RightTest(178)};
    // rtPtr->setValue(26);
   //std::unique_ptr<test> p{new test()};
    //rtPtr->ptr = std::move(p);
    cout << rtPtr->getValue() << endl;
    std::vector<std::unique_ptr<RightTest>> arr;
    arr.reserve(3);
    arr.push_back(std::make_unique<RightTest>(34));
    arr.push_back(std::make_unique<RightTest>(38));
    arr.push_back(std::make_unique<RightTest>(64));

// std::vector<std::unique_ptr<RightTest>>::iterator iter=arr.begin();

// for ( ;iter!=arr.end();)
// {
//     cout << "value "<< iter->get()->getValue() << endl;
//     iter = arr.erase(iter);
// }
    std::for_each(arr.begin(),arr.end(),[&arr](std::unique_ptr<RightTest> &r){
        cout << "value "<< r->getValue() << endl;
    });

std::vector<std::unique_ptr<RightTest>>().swap(arr);

    cout << "asdfnasldfj" << endl;
}