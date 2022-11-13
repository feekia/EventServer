/*
 * event_c_raii.h
 *
 *  Created on: 2018年5月10日
 *      Author: afreeliyunfeil@163.com
 */

#pragma once

#include <event.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <iostream>
#include <memory>

#define MAKE_RAII(type)                                                                                                \
    /* deleter */                                                                                                      \
    struct type##_deleter {                                                                                            \
        void operator()(struct type *ob) { type##_free(ob); }                                                          \
    };                                                                                                                 \
    /* unique ptr typedef */                                                                                           \
    typedef std::unique_ptr<struct type, type##_deleter> raii_##type

MAKE_RAII(event_base);
MAKE_RAII(event);
MAKE_RAII(evconnlistener);
MAKE_RAII(bufferevent);

inline raii_event_base obtain_event_base() {
    struct event_base *  base;
    struct event_config *cfg;
    cfg = event_config_new();
    event_config_require_features(cfg, 0);
    base = event_base_new_with_config(cfg);
    event_config_free(cfg);
    if (!base) {
        std::cout << "config base is nullptr， use event_base_new !" << std::endl;
        base = event_base_new();
    }

    auto result = raii_event_base(base);
    if (!result.get()) throw std::runtime_error("cannot create event_base");
    return result;
}

inline raii_event obtain_event(struct event_base *base, evutil_socket_t s, short events, event_callback_fn cb,
                               void *arg) {
    return raii_event(event_new(base, s, events, cb, arg));
}

inline raii_evconnlistener obtain_evconnlistener(struct event_base *base, evconnlistener_cb cb, void *ptr,
                                                 unsigned flags, int backlog, const struct sockaddr *sa, int socklen) {
    return raii_evconnlistener(evconnlistener_new_bind(base, cb, ptr, flags, backlog, sa, socklen));
}

inline raii_bufferevent obtain_bufferevent(struct event_base *base, evutil_socket_t fd, int options) {
    return raii_bufferevent(bufferevent_socket_new(base, fd, options));
}
