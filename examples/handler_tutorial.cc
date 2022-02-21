#include <iostream>
#include <memory>

#include "handler.h"
#include "message.h"

using namespace std;
using namespace es;

int main(int argc, char **argv) {
    cout << "Handler tutorial" << endl;

    Handler hdlr;
    hdlr.handleMessage([](const Message &msg) {
        switch (msg.what) {
        case 0: break;

        case 1: break;

        default: break;
        }

        cout << "Handler what: " << msg.what << endl;
    });
    for (int i = 0; i < 6; i++) {
        hdlr.sendEmptyMessageDelay(i + 1, 1000 * i);
    }

    hdlr.postDelay([]() { cout << "POST call back" << endl; }, 230);


    std::this_thread::sleep_until(std::chrono::steady_clock::now() + std::chrono::seconds(12));
    // hdlr.stop();
    cout << "Program exit !" << endl;
    return 1;
}
