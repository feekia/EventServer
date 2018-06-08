#include <sys/types.h>
#include <sys/socket.h>
#include <vector>
#include <string>
#include <algorithm>
#include <poll.h>
#include <thread>
#include <string.h>
#include <errno.h>
#include <iostream>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include "Server.h"

using namespace std;
#define MAX_OUTPUT (512*1024)

static const string MESSAGE = "This is server!";

#define FUNC_ENTRER cout << "in func: " << __func__ << endl

namespace vivo {
Server::Server(struct event_base *base, int port):mBase(base)
{
	memset(&mSin, 0, sizeof(mSin));
	mSin.sin_family = AF_INET;
	mSin.sin_port = htons(port);
}

Server::~Server(){

}
void Server::start_server(){
	mRaii_listener = std::move(obtain_evconnlistener(mBase, listener_callback, this,
		    LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr*)&mSin, sizeof(mSin)));
	FUNC_ENTRER;
}

void Server::listener_callback(struct evconnlistener *listener, evutil_socket_t fd,
	    struct sockaddr *sa, int socklen, void *ctx){

	Server *server = (Server *)ctx;
	struct event_base *base = server->mBase;
	cout << "in func: " << __func__ << endl;
	server->mRaii_bufferevent = obtain_bufferevent(base, fd, BEV_OPT_CLOSE_ON_FREE);
	struct bufferevent *bev = server->mRaii_bufferevent.get();
	if (!bev) {
		cout << "Error constructing bufferevent!" << endl;
		event_base_loopbreak(base);
		return;
	}
	bufferevent_setcb(bev, connection_readcb, connection_writecb, connection_eventcb, server);
	bufferevent_enable(bev, EV_WRITE|EV_READ|EV_PERSIST);
//	bufferevent_disable(bev, EV_READ);

//	bufferevent_write(bev, MESSAGE.c_str(), strlen(MESSAGE.c_str()));
//	bufferevent_decrement_read_limit(bev, MAX_OUTPUT);	//limit read buffer size
	bufferevent_setwatermark(server->mRaii_bufferevent.get(), EV_WRITE,
			0,
			0);
	FUNC_ENTRER;
};

void Server::connection_writecb(struct bufferevent *bev, void *ctx){
	struct evbuffer *output = bufferevent_get_output(bev);
	if (evbuffer_get_length(output) == 0) {
		cout << "flushed answer !" << endl;
	}

	FUNC_ENTRER;
}

void Server::connection_eventcb(struct bufferevent *bev, short events, void *ctx){
	if (events & BEV_EVENT_EOF) {
		cout << "Connection closed." << endl;
	} else if (events & BEV_EVENT_ERROR) {
		cout << "Got an error on the connection: "<< strerror(errno) << endl;
	} else if(events & BEV_EVENT_CONNECTED){
		cout << "Connection: BEV_EVENT_CONNECTED "<< endl;
	}
	/* None of the other events can happen here, since we haven't enabled
	 * timeouts */
	FUNC_ENTRER;
}

void Server::connection_readcb(struct bufferevent *bev, void *ctx){

	Server *server = (Server *)ctx;
	struct evbuffer *src;
	char buf[1024];
	size_t len;
	src = bufferevent_get_input(bev);
	len = evbuffer_get_length(src);
	if (!server->mRaii_bufferevent.get()) {
		evbuffer_drain(src, len);
		return;
	}

	if (evbuffer_remove(src, buf, len) > 0) {
		cout << "In server read: " << buf << endl;
	}

	bufferevent_write(server->mRaii_bufferevent.get(), buf, len);
	cout << "read the data:" << len << endl;
}

}

