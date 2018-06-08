#ifndef SERVER_H
#define SERVER_H

#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <thread>
#include <event2/event.h>
#include "utils/events.h"
#include <netinet/in.h>

using namespace std;

#define PORT 9950

namespace vivo {

class Server {
private:
	struct event_base *mBase;
	raii_evconnlistener mRaii_listener;
	raii_bufferevent mRaii_bufferevent;
	struct sockaddr_in mSin;

protected:

public:
	explicit Server(struct event_base *base, int port);
	~Server();

	void start_server();
	static void listener_callback(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *ctx);
	static void connection_writecb(struct bufferevent *bev, void *ctx);
	static void connection_eventcb(struct bufferevent *bev, short events, void *ctx);
	static void connection_readcb(struct bufferevent *bev, void *ctx);

	typedef std::function<void(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *ctx)> f_listener_callback;
	typedef std::function<void(struct bufferevent *bev, void *ctx)> f_connection_writecb;
	typedef std::function<void(struct bufferevent *bev, short events, void *ctx)> f_connection_eventcb;
	typedef std::function<void(struct bufferevent *bev, void *ctx)> f_connection_readcb;
};
}

#endif
