#ifndef __ACCOPTOR_H__
#define __ACCOPTOR_H__

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <thread>

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <map>

#include <event2/event.h>
#include <event2/listener.h>

#include "ThreadPool.h"
#include "events.h"
#include "wrapper.h"
#include "socketholder.h"

using namespace std;
class acceptor
{
private:
	raii_evconnlistener listener;
	raii_event_base base;
	std::unique_ptr<socketholder> holder;
	std::thread acceptorThread;
	std::mutex syncMutex;
	std::function<void()> breakCb;

public:
	acceptor(std::function<void()> &&f);
	virtual ~acceptor();
	int init(int port);
	static void connection_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *ctx);
	void onListenBreak();
	void stop();
};
#endif /* __ACCOPTOR_H__ */
