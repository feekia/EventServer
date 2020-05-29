#ifndef __SOCKET_HOLDER_H__
#define __SOCKET_HOLDER_H__

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

#define LOOP_MAX (8)
#define EVENT_BASE_WATCH_MAX (128 * 1024)
using namespace std;
class socketholder
{
private:
	std::vector<raii_event_base> watcherGroup;
	std::vector<std::map<evutil_socket_t, raii_event>> events;
	// std::map<evutil_socket_t, raii_event> events;
	std::map<string, evutil_socket_t> ids;
	std::unique_ptr<ThreadPool> pool;
	std::mutex syncMutex;
	std::atomic_bool isStop;

public:
	socketholder();
	virtual ~socketholder();
	void onConnect(evutil_socket_t fd);
	static void onRead(evutil_socket_t socket_fd, short events, void *ctx);
	static void onWrite(evutil_socket_t socket_fd, short events, void *ctx);
	void onDisconnect(evutil_socket_t fd);
	void stop();
};
#endif /* __SOCKET_HOLDER_H__ */
