#include <signal.h>
#include <memory.h>
#include <array>

#include "socketholder.h"

using namespace std;

socketholder::socketholder() : isStop(false), pools(5)
{
    for (int i = 0; i < LOOP_MAX; i++)
    {
        watcher_group[i] = obtain_event_base();
        watcher_thread[i] = std::thread([this, i]() {
            // 没有事件直接退出
            event_base_loop(watcher_group[i].get(), EVLOOP_NO_EXIT_ON_EMPTY);
            // event_base_dispatch(watcher_group[id].get());
            cout << "in loop id: " << i << endl;
            watcher_thread[i].detach();
        });
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
        if (watcher_thread[i].joinable())
        {
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
    std::unique_lock<std::mutex> lock(syncMutex);
    auto id = fd % LOOP_MAX;
    cout << "fd is: " << fd << endl;
    auto base = watcher_group[id].get();
    auto socket_event = obtain_event(base, -1, 0, nullptr, nullptr);
    auto event = socket_event.get();

    event_assign(event, base, fd, EV_READ, onRead, this);
    if (!event || event_add(event, nullptr) < 0)
    {
        cout << "onConnect Could not create/add a socket event!" << endl;
        close(fd);
        return;
    }
    eventset[id].emplace(fd, std::move(socket_event));
}

void socketholder::onRead(evutil_socket_t socket_fd, short events, void *ctx)
{

    socketholder *holder = (socketholder *)ctx;

    /* 在线程池进行读写，操作完之后将事件重新add 到base中 */
    holder->pools.enqueue([holder, socket_fd]() {
#define BUF_SIZE 1024
        char buffer[BUF_SIZE] = {0};
        memset(buffer, 0x0, sizeof(buffer));

        /*
    	 * 将数据读到缓冲区中，然后进行解码（尚未实现）
    	 */
        int size = TEMP_FAILURE_RETRY(read(socket_fd, (void *)buffer, BUF_SIZE));
        if (0 == size || -1 == size)
        {
            cout << "remote socket is close for socket: " << socket_fd << endl;
            if (holder != nullptr)
            {
                holder->onDisconnect(socket_fd);
            }
            return;
        }
        auto pair = holder->eventset[socket_fd % LOOP_MAX].find(socket_fd);
        if (pair->second != nullptr)
        {
            cout << "ADD  event" << endl;
            event_add(pair->second.get(), nullptr);
        }

        /*
    	 * 发送先将数据写到发送队列，再按顺序发送出去。（尚未实现）
    	 */
        int ret = TEMP_FAILURE_RETRY(write(socket_fd, (void *)buffer, size));
        if (ret <= 0)
            cout << "response failed" << endl;
        else
            cout << "response success ret: " << ret << endl;
    });
}
void socketholder::onWrite(evutil_socket_t socket_fd, short events, void *ctx)
{
}

void socketholder::onDisconnect(evutil_socket_t fd)
{
    std::unique_lock<std::mutex> lock(syncMutex);
    auto pair = this->eventset[fd % LOOP_MAX].find(fd);
    if (pair->second != nullptr)
    {
        event_del(pair->second.get());
        this->eventset[fd % LOOP_MAX].erase(fd);
    }
    close(fd);
    cout << "close socket fd: " << fd << endl;
}

void socketholder::stop()
{
    isStop = true;
}
