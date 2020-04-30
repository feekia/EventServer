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

#include "ThreadPool.h"
#include "events.h"
#include "wrapper.h"

using namespace std;

class ClientSocketAcceptor {
private:
	/*
	 * 每个客户端对应一个fd，并有一个数据处理任务std::function<void()>
	 */
	std::map<evutil_socket_t,std::function<void()>> taskMap;

	/*
	 * 每个客户端对应一个event_base
	 */
	std::map<evutil_socket_t,raii_event_base> baseMap;

	/*
	 * 每个客户端对应一个event
	 */
	std::map<evutil_socket_t,raii_event> eventMap;

	std::map<evutil_socket_t,std::thread> threadMap;
	std::mutex syncMutex;

public:
	ClientSocketAcceptor();
	virtual ~ClientSocketAcceptor();

	static void connectionListener(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *ctx);
	/*
	 * 尚未使用bufferevent
	 */
//	static void bufferevent_writecb(struct bufferevent *bev, void *ctx);
//	static void bufferevent_eventcb(struct bufferevent *bev, short events, void *ctx);
//	static void bufferevent_onRead(struct bufferevent *bev, void *ctx);

	static void onRead(evutil_socket_t socket_fd, short events, void *ctx);

//	void stop();
//	virtual void onDestroy();
};
#endif
