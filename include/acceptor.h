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
#include <signal.h>
#include <event2/event.h>
#include <event2/listener.h>

#include "events.h"
#include "wrapper.h"
#include "socketholder.h"

using namespace std;
#define LISTEN_CNT (2)

class acceptor
{
private:
	std::array<raii_event_base, LISTEN_CNT> bases;
	std::array<std::thread, LISTEN_CNT> threads;
	std::array<raii_evconnlistener, LISTEN_CNT> listeners;
	std::shared_ptr<socketholder> holder;
	std::function<void()> breakCb;
	
public:
	acceptor(std::function<void()> &&f);
	virtual ~acceptor();
	int init(int port);
	static void connection_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *ctx);
	void stop();
	void wait();
private:
	void onListenBreak();
};
#endif /* __ACCOPTOR_H__ */
