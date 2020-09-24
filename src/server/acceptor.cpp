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

#include "events.h"
#include "wrapper.h"
#include "acceptor.h"

using namespace std;

#define BUF_SIZE 1024

static std::atomic<bool> isStop(false);

acceptor::acceptor(std::function<void()> &&f) : holder(nullptr), breakCb(f)
{
}

int acceptor::init(int port)
{
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	for (int i = 0; i < LISTEN_CNT; i++)
	{
		bases[i] = obtain_event_base();
		listeners[i] = obtain_evconnlistener(bases[i].get(), acceptor::connection_cb, (void *)this,
											 LEV_OPT_REUSEABLE | LEV_OPT_REUSEABLE_PORT, -1, (struct sockaddr *)&sin, sizeof(sin));
	}
	// holder = std::make_shared<socketholder>();
	holder = std::shared_ptr<socketholder>(socketholder::getInstance());
	return 0;
}
thread_local int con_cnt = 0;
thread_local std::chrono::system_clock::duration locald;
void acceptor::connection_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *ctx)
{
	if (ctx == nullptr)
		return;

	con_cnt++;
	if (con_cnt == 1)
	{
		locald = std::chrono::system_clock::now().time_since_epoch();
	}

	if (con_cnt > 20)
	{
		std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
		std::chrono::milliseconds msec = std::chrono::duration_cast<std::chrono::milliseconds>(d);
		std::chrono::milliseconds localmsec = std::chrono::duration_cast<std::chrono::milliseconds>(locald);
		int64_t diff = msec.count() - localmsec.count();
		if (diff > 5 * 1000)
		{
			cout << " connect tid: " << std::this_thread::get_id() << " con_cnt : " << con_cnt << endl;
			locald = std::chrono::system_clock::now().time_since_epoch();
			con_cnt = 0;
		}
	}

	acceptor *accp = (acceptor *)ctx;
	if (isStop)
	{
		close(fd);
		return;
	}
	evutil_make_socket_nonblocking(fd);
	accp->holder->onConnect(fd);
}

void acceptor::onListenBreak()
{
	if (breakCb)
		breakCb();
	holder->waitStop();
}

void acceptor::stop()
{
	if (!isStop)
	{
		isStop = true;
		for (int i = 0; i < LISTEN_CNT; i++)
		{
			event_base_loopexit(bases[i].get(), nullptr);
		}
	}
}

void acceptor::wait()
{
	for (int i = 0; i < LISTEN_CNT; i++)
	{
		threads[i] = std::thread([this, i]() {
			event_base_loop(this->bases[i].get(), EVLOOP_NO_EXIT_ON_EMPTY);
		});
	}

	for (int i = 0; i < LISTEN_CNT; i++)
	{
		threads[i].join();
	}
	onListenBreak();
	cout << " exit the acceptor wait" << endl;
}
acceptor::~acceptor()
{
	cout << " Exit the acceptor" << endl;
}
