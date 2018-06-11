//============================================================================
// Name        : Helloword.cpp
// Author      : liyunfei
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <memory>
#include <stdio.h>
#include <event2/event-config.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/util.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <errno.h>
#include <string.h>

#include "Server.h"
#include "utils/events.h"
#include "Handler.h"
#include "Message.h"

using namespace std;
using namespace vivo;

#if 0
typedef void (*f_signal)(evutil_socket_t, short, void *);
static void
signal_cb(evutil_socket_t sig, short events, void *ctx)
{
	struct event_base *base = (struct event_base *)ctx;
	struct timeval delay = { 2, 0 };

	cout << "Caught an interrupt signal; exiting cleanly in two seconds." << endl;

	event_base_loopexit(base, &delay);
}

class Signal{
private:
	struct event_base *mBase;
	raii_event mRaii_signal_event;
public:
	Signal(struct event_base *base):mBase(base){}
	~Signal(){}
};

int main(int argc, char **argv)
{
	auto flags = EV_SIGNAL|EV_PERSIST;
//	struct sockaddr_in sin;
//
//	memset(&sin, 0, sizeof(sin));
//	sin.sin_family = AF_INET;
//	sin.sin_port = htons(9950);

    // Obtain event base
    auto raii_base = obtain_event_base();

    std::unique_ptr<Server> ptr_server = make_unique<Server>(raii_base.get(), 9950);
    ptr_server.get()->start_server();

	f_signal f = signal_cb;
    auto raii_signal_event = obtain_event(raii_base.get(), SIGINT, flags, f, (void*)raii_base.get());
    if(!raii_signal_event.get() || event_add(raii_signal_event.get(),nullptr) < 0){
		cout << "Could not create/add a signal event!" << endl;
		return 1;
    }

	event_base_dispatch(raii_base.get());

    return 0;
}

#else

class myHandler : public Handler{
	virtual void handleMessage(Message& msg) override {
		Handler::handleMessage(msg);
		switch(msg.m_what){
		case 0:

			break;

		case 1:
			break;

		case 2:
			break;

		case 3:
			break;

		case 4:
			break;

		case 5:
			break;

		default:
			break;
		}

		cout << "IN myHandler case: " << msg.m_what << endl;
	}
};
int main(int argc, char **argv)
{
	cout << "IN main" << endl;

	myHandler hdlr;
	for(int i = 0; i < 6; i++){
		hdlr.sendEmptyMessage(i, 100 * i);
	}

	hdlr.postAtTime([](){
		cout << "IN POST call back" << endl;
	}, 230);

	hdlr.stopSafty(true);
	while(true){
		std::this_thread::sleep_for(std::chrono::seconds(100000));
	}
	return 1;
}
#endif
