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

typedef void (*f_signal)(evutil_socket_t, short, void *);

static std::atomic<bool> isStop(false);

static void signal_int(evutil_socket_t sig, short events, void *ctx)
{
	struct event_base *base = (struct event_base *)ctx;
	cout << "Caught an signal : " << events << endl;
	if (!isStop)
	{
		isStop = true;
		cout << "Caught an signal : SIGINT" << endl;
		event_base_loopexit(base, nullptr);
	}
}

static void signal_pipe(evutil_socket_t sig, short events, void *ctx)
{
	cout << "signal_pipe: " << sig << endl;
}
acceptor::acceptor(std::function<void()> &&f) : base(nullptr), listener(nullptr), signal_event(nullptr), pipe_event(nullptr), holder(nullptr), breakCb(f)
{
}

int acceptor::init(int port)
{
	auto flags = EV_SIGNAL | EV_PERSIST;
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	base = obtain_event_base();

	listener = obtain_evconnlistener(base.get(), acceptor::connection_cb, (void *)this,
									 LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr *)&sin, sizeof(sin));

	signal_event = obtain_event(base.get(), SIGINT, flags, signal_int, (void *)base.get());
	if (!signal_event.get() || event_add(signal_event.get(), nullptr) < 0)
	{
		cout << "Could not create/add a SIGINT event -!" << endl;
	}

	pipe_event = obtain_event(base.get(), SIGPIPE, flags, signal_pipe, (void *)base.get());
	if (!pipe_event.get() || event_add(pipe_event.get(), nullptr) < 0)
	{
		cout << "Could not create/add a SIGPIPE event -!" << endl;
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
	if(con_cnt == 1){
		locald= std::chrono::system_clock::now().time_since_epoch();
	}

	if (con_cnt > 20)
	{
		std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
		std::chrono::milliseconds msec = std::chrono::duration_cast<std::chrono::milliseconds>(d);
		std::chrono::milliseconds localmsec = std::chrono::duration_cast<std::chrono::milliseconds>(locald);
		int64_t diff = msec.count() - localmsec.count();
		if (diff > 10 * 1000)
		{
			cout << " connect tid: " << std::this_thread::get_id() << " con_cnt : " << con_cnt << endl;
			locald = std::chrono::system_clock::now().time_since_epoch();
			con_cnt= 0;
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
	event_base_loopexit(base.get(), nullptr);
}

void acceptor::wait()
{
	event_base_loop(this->base.get(), EVLOOP_NO_EXIT_ON_EMPTY);
	cout << " exit the acceptor wait" << endl;
	listener.reset();
	signal_event.reset();
	onListenBreak();
	cout << " exit the acceptor wait" << endl;
}
acceptor::~acceptor()
{
	cout << " Exit the acceptor" << endl;
}
