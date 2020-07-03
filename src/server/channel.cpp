#include "channel.h"
#include "socketholder.h"
channel::channel(std::shared_ptr<socketholder> h, evutil_socket_t _fd) : fd(_fd), holder(h)
{
}

channel::~channel()
{
}

void channel::startWatcher()
{
    auto base = holder->rwatchers[fd % READ_LOOP_MAX].get();
    rEvent = obtain_event(base, -1, 0, nullptr, nullptr);
    auto revent = rEvent.get();
    event_assign(revent, base, fd, EV_READ, onRead, this);

    event_add(rEvent.get(), nullptr);
}

void channel::addReadEvent(size_t timeout)
{
    event_add(rEvent.get(), nullptr);
}

void channel::addWriteEvent(size_t timeout)
{
    auto base = holder->rwatchers[fd % READ_LOOP_MAX].get();
    wEvent = obtain_event(base, -1, 0, nullptr, nullptr);
    auto wevent = wEvent.get();
    event_assign(wevent, base, fd, EV_WRITE, onWrite, this);
    event_add(wEvent.get(), nullptr);
}

void channel::onDisconnect(evutil_socket_t fd)
{
    holder->onDisconnect(fd);
}

void channel::onRead(evutil_socket_t socket_fd, short events, void *ctx)
{
    socketholder *holder = (socketholder *)ctx;
#if 0
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
        auto pair = holder->eventset[socket_fd % READ_LOOP_MAX].find(socket_fd);
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

#endif
    auto chan = ((channel *)ctx)->shared_from_this();
    chan->holder->pools.enqueue([chan, socket_fd]() {
        auto size = chan->rBuf.readsocket(socket_fd);
        if (0 == size || -1 == size)
        {
            cout << "remote socket is close for socket: " << socket_fd << endl;
            chan->holder->onDisconnect(socket_fd);
            return;
        }
        // TODO: decode read buffer
        chan->rBuf.toString();
        chan->rBuf.reset();
        event_add(chan->rEvent.get(), nullptr);
    });
}
void channel::onWrite(evutil_socket_t socket_fd, short events, void *ctx)
{
}