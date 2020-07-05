#include <memory>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/event-config.h>
#include <netinet/tcp.h>
#include <thread>
#include <chrono>
#include <event2/thread.h>

#include "events.h"
#include "wrapper.h"

#include <iostream>

using namespace std;

#define BUF_SIZE 1024
#define LOG(X) std::cout << "Client: " << X << std::endl

static void onSignal(evutil_socket_t sig, short events, void *ctx)
{
	if (!ctx)
		return;
	LOG(__func__);
	struct event *sig_event = (struct event *)ctx;
	event_base_loopexit(event_get_base(sig_event), nullptr);
}

static void onRead(evutil_socket_t socket_fd, short events, void *ctx)
{
	LOG(__func__);

	char buffer[BUF_SIZE] = {0};
	int size = TEMP_FAILURE_RETRY(read(socket_fd, buffer, BUF_SIZE));
	if (0 == size || -1 == size)
	{ //说明socket关闭
		cout << "read size is " << size << " for socket: " << socket_fd << endl;
		struct event *read_ev = (struct event *)ctx;
		if (NULL != read_ev)
		{
			event_del(read_ev);
		}
		close(socket_fd);
		return;
	}
	LOG(buffer);
}


static void onTerminal(evutil_socket_t fd, short events, void *ctx)
{
	if (!ctx)
		return;

    char msg[1024];  
  
    int ret = read(fd, msg, sizeof(msg));  
    if( ret <= 0 )  
    {  
        perror("read fail ");  
        exit(1);  
    }  
    int sockfd = *((int*)ctx);  
    ret = write(sockfd, msg, ret); 
}

int main()
{
	int port = 9950;
	struct sockaddr_in my_address;
	memset(&my_address, 0, sizeof(my_address));
	my_address.sin_family = AF_INET;
	my_address.sin_addr.s_addr = htonl(0x7f000001); // 127.0.0.1
	my_address.sin_port = htons(port);
	evthread_use_pthreads();
	auto raii_base = obtain_event_base();

	// set TCP_NODELAY to let data arrive at the server side quickly
	evutil_socket_t fd;
	fd = socket(AF_INET, SOCK_STREAM, 0);

	int enable = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof(enable)) < 0)
	{
		cout << "ERROR: TCP_NODELAY SETTING ERROR!\n";
		close(fd);
		return -1;
	}

	int result = connect(fd, (struct sockaddr *)&my_address, sizeof(struct sockaddr));
	if (result < 0)
	{
		cout << "Connect Error!" << endl;
		close(fd);
		return -1;
	}
	evutil_make_socket_nonblocking(fd);
	cout << "result=" << result << endl;
	auto raii_socket_event = obtain_event(raii_base.get(), -1, 0, nullptr, nullptr);
	event_assign(raii_socket_event.get(), raii_base.get(), fd, EV_READ | EV_PERSIST, onRead, raii_socket_event.get());

	if (!raii_socket_event.get() || event_add(raii_socket_event.get(), nullptr) < 0)
	{
		cout << "Could not create/add a socket event!" << endl;
		close(fd);
		return -1;
	}

	auto raii_signal_event = obtain_event(raii_base.get(), -1, 0, nullptr, nullptr);
	event_assign(raii_signal_event.get(), raii_base.get(), SIGINT, EV_SIGNAL,
				 onSignal, (void *)raii_signal_event.get());
	if (!raii_signal_event.get() || event_add(raii_signal_event.get(), nullptr) < 0)
	{
		cout << "Could not create/add a signal event!" << endl;
		close(fd);
		return -1;
	}


	auto raii_terminal_event = obtain_event(raii_base.get(), -1, 0, nullptr, nullptr);
	event_assign(raii_terminal_event.get(), raii_base.get(), STDIN_FILENO, EV_READ | EV_PERSIST,
				 onTerminal, (void *)&fd);
	if (!raii_terminal_event.get() || event_add(raii_terminal_event.get(), nullptr) < 0)
	{
		cout << "Could not create/add a signal event!" << endl;
		close(fd);
		return -1;
	}


	std::thread work_thread([&raii_base]() {
		event_base_dispatch(raii_base.get());
		cout << " break thread loop" << endl;
	});


	work_thread.join();

	return 0;
}
