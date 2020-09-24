//============================================================================
// Name        : EventSever.cpp
// Author      : liyunfei   afreeliyunfeil@163.com
// Version     :
// Copyright   : Your copyright notice
// Description : EventSever in C++, Ansi-style
//============================================================================

#include <iostream>
#include <memory>
#include <stdio.h>
#include <event2/event-config.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/util.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <event2/thread.h>
#include <csignal>
#include <arpa/inet.h>

#include "events.h"
#include "Handler.h"
#include "Message.h"

using namespace std;

#if 1
#include "acceptor.h"

void errHandler(int err)
{
	cout << "error in lv: " << err << endl;
}

static acceptor *pacceptor = nullptr;
void sigHandler(int sig)
{
	cout << "sigHandler sig: " << sig << endl;
	if(pacceptor == nullptr){
		return;
	}
	switch (sig)
	{
	case SIGINT:
		pacceptor->stop();
		break;
	case SIGPIPE:
		break;	
	default:
		break;
	}
}

int main(int argc, char **argv)
{

	if (argc < 2)
	{
		printf("Usage:port,example:8080 \n");
		return -1;
	}
	int port = atoi(argv[1]);
	std::signal(SIGINT, sigHandler);
	std::signal(SIGPIPE, sigHandler);
	// event_enable_debug_logging(EVENT_DBG_ALL);
	event_set_fatal_callback(errHandler);
	evthread_use_pthreads();
	pacceptor = new acceptor([]() {
		cout << " break acceptor in callback" << endl;
	});
	pacceptor->init(port);

	cout << "EventServer startup" << endl;
	pacceptor->wait();
	cout << "main thread exit" << endl;
	delete pacceptor;
	pacceptor = nullptr;
	std::this_thread::sleep_for(std::chrono::seconds(2));
	return 0;
}
#elif 0

class myHandler : public Handler
{
	virtual void handleMessage(Message &msg) override
	{
		Handler::handleMessage(msg);
		switch (msg.m_what)
		{
		case 0:

			break;

		case 1:
			break;

		case 2:
			break;

		case 3:
			break;

		case 4:
			break;

		case 5:
			break;

		default:
			break;
		}

		cout << "IN myHandler case: " << msg.m_what << endl;
	}
};
int main(int argc, char **argv)
{
	cout << "IN main" << endl;

	myHandler hdlr;
	for (int i = 0; i < 6; i++)
	{
		hdlr.sendEmptyMessage(i, 100 * i);
	}

	hdlr.postAtTime([]() {
		cout << "IN POST call back" << endl;
	},
					230);

	hdlr.stopSafty(true);
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::seconds(100000));
	}
	return 1;
}
#endif
