#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <thread>

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#include <event2/event.h>
#include <event2/listener.h>

#include "ThreadPool.h"
#include "events.h"
#include "wrapper.h"
#include "acceptor.h"

using namespace std;

#define BUF_SIZE 1024

acceptor::acceptor(std::function<void()> &&f)
{
	breakCb = f;
	holder = std::make_unique<socketholder>();
}
int acceptor::init(int port)
{
	auto flags = EV_SIGNAL | EV_PERSIST;
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(9950);

	base = obtain_event_base();

	listener = obtain_evconnlistener(base.get(), acceptor::connection_cb, (void *)base.get(),
									 LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr *)&sin, sizeof(sin));
	if (!listener.get())
	{
		cout << "error listen event get null" << endl;
		return -1;
	}
	acceptorThread = std::thread([this]() {
		event_base_dispatch(this->base.get());
		cout << "acceptorThread exit" << endl;
		onListenBreak();
	});
}

void acceptor::connection_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *ctx)
{
	if (ctx == nullptr)
		return;

	acceptor *accp = (acceptor *)ctx;
	std::unique_lock<std::mutex> lock(accp->syncMutex);
	accp->holder->onConnect(fd);
}

void acceptor::onListenBreak()
{
	if (breakCb)
		breakCb();
}

void acceptor::stop()
{
	event_base_loopexit(base.get(),nullptr);
	holder->stop();
}
acceptor::~acceptor()
{
	cout << " Exit the acceptor" << endl;
	acceptorThread.join();
	holder = nullptr;
}
