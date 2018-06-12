//============================================================================
// Name        : EventSever.cpp
// Author      : liyunfei
// Version     :
// Copyright   : Your copyright notice
// Description : EventSever in C++, Ansi-style
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

#include "utils/events.h"
#include "Handler.h"
#include "Message.h"

using namespace std;


#if 1
#include "ClientSocketAcceptor.h"

typedef void (*f_signal)(evutil_socket_t, short, void *);
static void
signal_cb(evutil_socket_t sig, short events, void *ctx)
{
	struct event_base *base = (struct event_base *)ctx;
	struct timeval delay = { 2, 0 };

	cout << "Caught an interrupt signal; exiting cleanly in two seconds. sig:" << sig << endl;

	event_base_loopexit(base, &delay);
}

int main(int argc, char **argv)
{
	auto flags = EV_SIGNAL|EV_PERSIST;
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(9950);

    // Obtain event base
    auto raii_base = obtain_event_base();

    std::unique_ptr<ClientSocketAcceptor> ptr_acceptor = make_unique<ClientSocketAcceptor>();

    /*
     * 监听客户端socket链接，
     * 通过回调connectionListener将客户端的socket fd存储起来
     */
	auto raii_listener = obtain_evconnlistener(raii_base.get(), ClientSocketAcceptor::connectionListener, (void*)ptr_acceptor.get(),
		    LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr*)&sin, sizeof(sin));

	f_signal f = signal_cb;
    auto raii_signal_event = obtain_event(raii_base.get(), SIGINT, flags, f, (void*)raii_base.get());
    if(!raii_signal_event.get() || event_add(raii_signal_event.get(),nullptr) < 0){
		cout << "Could not create/add a signal event!" << endl;
		return 1;
    }


    std::thread acceptorThread([&raii_base](){

    	event_base_dispatch(raii_base.get());
    	cout << "work_thread exit" << endl;
    });

    acceptorThread.join();
//	event_base_dispatch(raii_base.get());

//    ptr_acceptor = nullptr; //提前释放acceptor raii
    ptr_acceptor.reset(); //提前释放acceptor raii
    while(true)
    	std::this_thread::sleep_for(std::chrono::seconds(1000));

    cout << "main thread exit" << endl;
    return 0;
}
#elif 0

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
#elif 0
#include <iostream>
#include <vector>
#include <chrono>

#include "ThreadPool.h"

int main()
{

    ThreadPool pool(4);
    std::vector< std::future<int> > results;

    for(int i = 0; i < 8; ++i) {
        results.emplace_back(
            pool.enqueue([i] {
                std::cout << "hello " << i << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "world " << i << std::endl;
                return i*i;
            })
        );
    }

    for(auto && result: results)
        std::cout << result.get() << ' ';
    std::cout << std::endl;

    return 0;
}
#endif
