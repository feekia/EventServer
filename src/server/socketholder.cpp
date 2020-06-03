#include <signal.h>
#include "socketholder.h"
#include <array>
using namespace std;

socketholder::socketholder() : isStop(false)
{
    for (int i = 0; i < LOOP_MAX; i++)
    {
        watcher_group[i] = obtain_event_base();
    }
}
socketholder::~socketholder()
{
    for (int i = 0; i < LOOP_MAX; i++)
    {
        event_base_loopexit(watcher_group[i].get(), nullptr);
    }
    for (int i = 0; i < LOOP_MAX; i++)
    {
        if (watcher_thread[i].joinable()){
            watcher_thread[i].join();
        }
    }
}

void socketholder::onConnect(evutil_socket_t fd)
{
    if (isStop)
    {
        cout << " socketholder is stop" << endl;
        return;
    }
    auto id = fd % LOOP_MAX;
    cout << "fd is: " << fd << endl;
    auto base = watcher_group[id].get();
    auto socket_event = obtain_event(base, -1, 0, nullptr, nullptr);
    auto event = socket_event.get();

    event_assign(event, base, fd, EV_READ | EV_PERSIST, onRead, this);
    if (!event || event_add(event, nullptr) < 0)
    {
        cout << "onConnect Could not create/add a socket event!" << endl;
        close(fd);
        return;
    }
    eventset[id].emplace(fd, std::move(socket_event));
    if (!watcher_thread[id].joinable())
    {
        cout << "thread is not joinable: " << id << endl;
        watcher_thread[id] = std::thread([this, id]() {
            // 没有事件直接退出
            event_base_dispatch(watcher_group[id].get());
            cout << "in loop id: " << id << endl;
            // watcher_thread[id].swap(watcher_thread[id]);
            watcher_thread[id].detach();
        });
    }
}

#include <memory.h>
#define BUF_SIZE 1024
void socketholder::onRead(evutil_socket_t socket_fd, short events, void *ctx)
{
    char buffer[BUF_SIZE] = {0};
    memset(buffer, 0x0, sizeof(buffer));

    /*
	 * read the client data
	 */
    int size = TEMP_FAILURE_RETRY(read(socket_fd, (void *)buffer, BUF_SIZE));
    if (0 == size || -1 == size)
    { //说明socket关闭
        cout << "remote socket is close read size is " << size << " for socket: " << socket_fd << endl;
        socketholder *holder = (socketholder *)ctx;
        if (holder != nullptr)
        {
            auto pair = holder->eventset[socket_fd % LOOP_MAX].find(socket_fd);
            if(pair->second != nullptr){
                event_del(pair->second.get());
                holder->eventset[socket_fd % LOOP_MAX].erase(socket_fd);
            }
        }

        close(socket_fd);
         cout << "close socket fd: " <<  socket_fd <<  endl;

        return;
    }
   
    /*
     * respone to the client
     */
    int ret = TEMP_FAILURE_RETRY(write(socket_fd, (void *)buffer, size));
    if (ret <= 0)
        cout << "Send response failed" << endl;
    else
        cout << "Send response success ret: " << ret << endl;
}
void socketholder::onWrite(evutil_socket_t socket_fd, short events, void *ctx)
{
}

void socketholder::onDisconnect(evutil_socket_t fd)
{
}

void socketholder::stop()
{
    isStop = true;
}
