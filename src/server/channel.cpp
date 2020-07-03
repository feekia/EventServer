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

    wEvent = obtain_event(base, -1, 0, nullptr, nullptr);
    auto wevent = wEvent.get();
    event_assign(wevent, base, fd, EV_WRITE, onWrite, this);

    event_add(rEvent.get(), nullptr); // add read event into event_base
    event_add(wEvent.get(), nullptr);
}

void channel::addReadEvent(size_t timeout)
{
    event_add(rEvent.get(), nullptr);
}

void channel::addWriteEvent(size_t timeout)
{

    event_add(wEvent.get(), nullptr);
}

void channel::onDisconnect(evutil_socket_t fd)
{
    holder->onDisconnect(fd);
}

void channel::onRead(evutil_socket_t socket_fd, short events, void *ctx)
{
    cout << " onRead events:  " << events << endl;
    auto chan = ((channel *)ctx)->shared_from_this();
    chan->holder->pools.enqueue([chan, socket_fd]() {
        std::unique_lock<std::mutex> lock(chan->rMutex);
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
        chan->addReadEvent(0);
    });
}
void channel::onWrite(evutil_socket_t socket_fd, short events, void *ctx)
{
    cout << " onWrite events:  " << events << endl;
    auto chan = ((channel *)ctx)->shared_from_this();
    chan->holder->pools.enqueue([chan, socket_fd]() {
        std::unique_lock<std::mutex> lock(chan->wMutex);
        auto size = chan->wBuf.writesocket(socket_fd);
        if(chan->wBuf.size() > 0){
            // data dosn't finish write, add event to watcher,when socket is writeable callback this function
            chan->addWriteEvent(0);
        }
    });
}