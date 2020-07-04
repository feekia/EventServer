#include "channel.h"
#include "socketholder.h"
channel::channel(std::weak_ptr<socketholder> h, evutil_socket_t _fd) : fd(_fd), holder(h)
{
}

channel::~channel()
{
}

void channel::startWatcher()
{
    std::shared_ptr<socketholder> hld = holder.lock();
    if (hld == nullptr)
    {
        return;
    }
    auto base = hld->rwatchers[fd % READ_LOOP_MAX].get();
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
    holder.lock()->onDisconnect(fd);
}

void channel::onRead(evutil_socket_t socket_fd, short events, void *ctx)
{
    auto chan = ((channel *)ctx)->shared_from_this();
    std::shared_ptr<socketholder> hld = chan->holder.lock();
    if (hld == nullptr)
    {
        return;
    }
    hld->pools.enqueue([chan, socket_fd]() {
        std::unique_lock<std::mutex> lock(chan->cMutex);
        auto size = chan->rBuf.readsocket(socket_fd);
        if (0 == size || -1 == size)
        {
            cout << "remote socket is close for socket: " << socket_fd << endl;
            std::shared_ptr<socketholder> hld = chan->holder.lock();
            if (hld == nullptr)
            {
                return;
            }
            hld->onDisconnect(socket_fd);
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
    auto chan = ((channel *)ctx)->shared_from_this();
    std::shared_ptr<socketholder> hld = chan->holder.lock();
    if (hld == nullptr)
    {
        return;
    }
    hld->pools.enqueue([chan, socket_fd]() {
        std::unique_lock<std::mutex> lock(chan->cMutex);
        auto size = chan->wBuf.writesocket(socket_fd);
        if (chan->wBuf.size() > 0)
        {
            // data dosn't finish write, add event to watcher,when socket is writeable callback this function
            chan->addWriteEvent(0);
        }
    });
}

void channel::closeSafty()
{
    std::unique_lock<std::mutex> lock(cMutex);
    stop = true;
    if (wBuf.size() == 0 && rBuf.size() == 0)
    {
        removeReadEvent();
        removeWriteEvent();
        close(fd);

        std::shared_ptr<socketholder> hld = holder.lock();
        if (hld == nullptr)
        {
            return;
        }
        hld->onDisconnect(fd);
    }
}