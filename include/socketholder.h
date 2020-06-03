#ifndef __SOCKET_HOLDER_H__
#define __SOCKET_HOLDER_H__

#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <thread>

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <map>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <event2/event.h>
#include <event2/listener.h>

#include "events.h"
#include "wrapper.h"

#define LOOP_MAX (8)
#define EVENT_BASE_WATCH_MAX (128 * 1024)
using namespace std;
class socketholder
{
private:
	std::array<raii_event_base,LOOP_MAX> watcher_group;
	std::array<std::thread,LOOP_MAX> watcher_thread;
	std::map<string, evutil_socket_t> ids;
	std::mutex syncMutex;
	std::condition_variable condition;
	std::atomic_bool isStop;
	std::array<std::map<evutil_socket_t, raii_event>,LOOP_MAX> eventset;
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
