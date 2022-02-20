#include <iostream>
#include <memory>

#include "handler.h"
#include "message.h"

using namespace std;
using namespace es;

class myHandler : public Handler {
    virtual void handleMessage(Message &msg) override {
        switch (msg.what) {
        case 0: break;

        case 1: break;

        default: break;
        }

        cout << "IN myHandler case: " << msg.what << endl;
    }
};
int main(int argc, char **argv) {
    cout << "Handler tutorial" << endl;

    myHandler hdlr;
    for (int i = 0; i < 6; i++) {
        hdlr.sendEmptyMessageDelay(i, 100 * i);
    }

    hdlr.postDelay([]() { cout << "IN POST call back" << endl; }, 230);

    hdlr.stop();
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(100000));
    }
    return 1;
}
