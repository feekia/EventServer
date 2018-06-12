#ifndef CLIENTSOCKETACCEPTOR_H
#define CLIENTSOCKETACCEPTOR_H

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
#include "utils/events.h"

#include "ThreadPool.h"
#include "utils/events.h"
#include "utils/wrapper.h"

using namespace std;

class ClientSocketAcceptor {
private:
	std::map<evutil_socket_t,std::function<void()>> taskMap;
	std::map<evutil_socket_t,raii_event_base> baseMap;
	std::map<evutil_socket_t,raii_event> eventMap;
	std::mutex taskMutex;

public:
	ClientSocketAcceptor();
	~ClientSocketAcceptor();

	static void connectionListener(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *ctx);
//	static void bufferevent_writecb(struct bufferevent *bev, void *ctx);
//	static void bufferevent_eventcb(struct bufferevent *bev, short events, void *ctx);
//	static void bufferevent_onRead(struct bufferevent *bev, void *ctx);

	static void onRead(evutil_socket_t socket_fd, short events, void *ctx);
};
#endif
