#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <thread>

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#include <event2/event.h>
#include <event2/listener.h>

#include "ThreadPool.h"
#include "events.h"
#include "wrapper.h"
#include "ClientSocketAcceptor.h"

using namespace std;

#define BUF_SIZE 1024

ClientSocketAcceptor::ClientSocketAcceptor(){}
ClientSocketAcceptor::~ClientSocketAcceptor(){
	cout << " Exit the ClientSocketAcceptor" << endl;
	std::unique_lock<std::mutex> lock(syncMutex);
	for(auto i=baseMap.begin();i != baseMap.end(); i++){
//		event_base_loopexit(i->second.get(),nullptr);
		event_base_loopbreak(i->second.get());
	}
}

void ClientSocketAcceptor::connectionListener(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *ctx){
	if (ctx == nullptr)
		return;

	ClientSocketAcceptor *acceptor = (ClientSocketAcceptor *)ctx;
	std::unique_lock<std::mutex> lock(acceptor->syncMutex);

	auto raii_base = obtain_event_base();
	auto base = raii_base.get();

	auto raii_socket_event = obtain_event(base, -1, 0, nullptr, nullptr);
	auto event = raii_socket_event.get();

	event_assign(event, base, fd, EV_READ|EV_PERSIST, ClientSocketAcceptor::onRead,acceptor);

	if (!event || event_add(event, nullptr) < 0) {
		cout << "Could not create/add a socket event!" << endl;
		close(fd);
		return;
	}
	acceptor->baseMap.emplace(make_pair(fd, std::move(raii_base)));
	acceptor->eventMap.emplace(make_pair(fd, std::move(raii_socket_event)));
	acceptor->taskMap.emplace(make_pair(fd, [](){
		// TODO: ADD YOUR CODE
		// 数据处理任务尚未实现
	}));

	/*
	 * 起一个新线程运行一个event io loop
	 */
	std::thread loop([acceptor](evutil_socket_t fd){
		// TODO: ADD YOUR CODE
		if(acceptor->baseMap.find(fd) != acceptor->baseMap.end()){
			cout << "Thread in the loop fd: " << fd << endl;
			event_base_dispatch(acceptor->baseMap.find(fd)->second.get());

			/*
			 * event looper 退出，将map清空。
			 */
			std::unique_lock<std::mutex> lock(acceptor->syncMutex);
			acceptor->baseMap.erase(fd);
			acceptor->eventMap.erase(fd);
			acceptor->taskMap.erase(fd);
			close(fd);
		}

		cout << "Thread exit fd: " << fd << endl;
	}, fd);

	loop.detach();
}
/*
 * 读取客户端数据，并返回给客户端（echo实现）
 */
void ClientSocketAcceptor::onRead(evutil_socket_t socket_fd, short events, void *ctx)
{
	if(ctx == nullptr)
		return;

	char buffer[BUF_SIZE];

	memset(buffer, 0 , BUF_SIZE);

	/*
	 * read the client data
	 */
	int size = TEMP_FAILURE_RETRY(read(socket_fd, (void *)buffer, BUF_SIZE));
    if(0 == size || -1 == size){//说明socket关闭
        cout<<"remote socket is close read size is " << size << " for socket: "<< socket_fd <<endl;
        auto acceptor = (ClientSocketAcceptor*)ctx;
        std::unique_lock<std::mutex> lock(acceptor->syncMutex);
		event_base_loopexit(acceptor->baseMap.find(socket_fd)->second.get(),
				nullptr);
        return;
    }
    cout << __func__ << " from fd:" << socket_fd<< " context is: " << buffer  << endl;

    /*
     * respone to the client
     */
    int ret = TEMP_FAILURE_RETRY(write(socket_fd, (void *)buffer, size));
    if (ret <= 0)
        cout << "Send response failed" << endl;
    else
    	cout << "Send response success ret: " << ret << endl;
}

