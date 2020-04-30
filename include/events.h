/*
 * events.h
 *
 *  Created on: 2018年5月10日
 *      Author: Administrator
 */

#ifndef UTILS_EVENTS_H_
#define UTILS_EVENTS_H_

#pragma once

#include <memory>
#include <event.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <iostream>

#define MAKE_RAII(type) \
/* deleter */\
struct type##_deleter {\
    void operator()(struct type* ob) {\
        type##_free(ob);\
        std::cout << "deleter: " << #type << std::endl; \
    }\
};\
/* unique ptr typedef */\
typedef std::unique_ptr<struct type, type##_deleter> raii_##type

MAKE_RAII(event_base);
MAKE_RAII(event);
MAKE_RAII(evconnlistener);
MAKE_RAII(bufferevent);

inline raii_event_base obtain_event_base() {
    auto result = raii_event_base(event_base_new());
    if (!result.get())
        throw std::runtime_error("cannot create event_base");
    return result;
}

inline raii_event obtain_event(struct event_base* base, evutil_socket_t s, short events, event_callback_fn cb, void* arg) {
    return raii_event(event_new(base, s, events, cb, arg));
}

inline raii_evconnlistener obtain_evconnlistener(struct event_base *base, evconnlistener_cb cb, void *ptr, unsigned flags, int backlog, const struct sockaddr *sa, int socklen) {
	return raii_evconnlistener(
			evconnlistener_new_bind(base, cb, ptr, flags, backlog, sa, socklen));
}

inline raii_bufferevent obtain_bufferevent(struct event_base *base, evutil_socket_t fd, int options) {
    return raii_bufferevent(bufferevent_socket_new(base, fd, options));
}

#endif /* UTILS_EVENTS_H_ */
