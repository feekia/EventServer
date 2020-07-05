#include "channel.h"
#include "socketholder.h"
channel::channel(std::weak_ptr<socketholder> h, evutil_socket_t _fd) : fd(_fd), holder(h)
{
    state.store(INIT);
    std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
    std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(d);
    timestamp = sec.count();
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
    event_assign(revent, base, fd, EV_READ | EV_TIMEOUT, onRead, this);

    wEvent = obtain_event(base, -1, 0, nullptr, nullptr);
    auto wevent = wEvent.get();
    event_assign(wevent, base, fd, EV_WRITE | EV_TIMEOUT, onWrite, this);
    struct timeval tv = {30, 0};
    event_add(rEvent.get(), &tv); // add read event into event_base
}

void channel::addReadEvent(size_t timeout)
{
    struct timeval tv = {20, 0};
    if (timeout > 0)
    {
        tv.tv_sec = timeout;
    }

    event_add(rEvent.get(), &tv);
}

void channel::addWriteEvent(size_t timeout)
{
    struct timeval tv = {10, 0};
    if (timeout > 0)
    {
        tv.tv_sec = timeout;
    }
    event_add(wEvent.get(), &tv);
}

bool channel::send(char *buffer, size_t l)
{
    if (stop)
    {
        return false;
    }
    std::unique_lock<std::mutex> lock(cMutex);
    wBuf.append(buffer, l);
    wBuf.writesocket(fd);
    addWriteEvent(WRITETIMEOUT);
}
void channel::onDisconnect(evutil_socket_t fd)
{
    holder.lock()->onDisconnect(fd);
}

void channel::onRead(evutil_socket_t socket_fd, short events, void *ctx)
{
    auto chan = ((channel *)ctx)->shared_from_this();
    if (events & EV_TIMEOUT)
    {
        cout << "onRead timeout: " << socket_fd << endl;
        // 判斷心跳是否超時
        if (chan->isHeartBrakeExpired())
        {
            chan->removeWriteEvent();
            chan->removeReadEvent();
            close(socket_fd);
            chan->state = CLOSE;
            std::shared_ptr<socketholder> hld = chan->holder.lock();
            if (hld == nullptr)
            {
                return;
            }
            hld->onDisconnect(socket_fd);
        }

        return;
    }

    chan->updateHearBrakeExpired();
    cout << "onRead: " << socket_fd << endl;
    std::shared_ptr<socketholder> hld = chan->holder.lock();
    if (hld == nullptr)
    {
        return;
    }
    hld->pools.enqueue([chan, socket_fd]() {
        if (chan->state == CLOSE)
        {
            cout << "when read task run, socket is close,so return" << endl;
            return;
        }
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
            close(socket_fd);
            chan->state = CLOSE;
            hld->onDisconnect(socket_fd);
            return;
        }
        // TODO: decode read buffer
        chan->rBuf.toString();
        chan->rBuf.reset();
        chan->addReadEvent(READTIMEOUT);
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
    if (events & EV_TIMEOUT)
    {
        cout << "write timeout" << endl;
        chan->removeWriteEvent();
        chan->removeReadEvent();
        close(socket_fd);
        chan->state = CLOSE;
        hld->onDisconnect(socket_fd);
        return;
    }
    chan->updateHearBrakeExpired();
    hld->pools.enqueue([chan, socket_fd]() {
        if (chan->state == CLOSE)
        {
            cout << "when write task run, socket is close,so return" << endl;
            return;
        }
        std::unique_lock<std::mutex> lock(chan->cMutex);
        auto size = chan->wBuf.writesocket(socket_fd);
        chan->addWriteEvent(WRITETIMEOUT);
        if (chan->wBuf.size() > 0)
        {
            cout << "not finish write" << endl;
            return;
        }
        if (chan->stop && chan->state == INIT)
        {
            //shutdown write mode
            shutdown(socket_fd, SHUT_WR);
            chan->state = SHUTDOWN;
        }
    });
}

void channel::closeSafty()
{
    std::unique_lock<std::mutex> lock(cMutex);
    stop = true;
    if (wBuf.size() <= 0)
    {
        shutdown(fd, SHUT_WR);
        state = SHUTDOWN;
    }
}