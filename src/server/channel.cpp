#include "channel.h"
#include "socketholder.h"
channel::channel(std::weak_ptr<socketholder> &&h, evutil_socket_t _fd) : fd(_fd), holder(h)
{
    state.store(INIT);
    std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
    std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(d);
    timestamp = sec.count();
}

channel::~channel()
{
}

void channel::listenWatcher(raii_event &&revent, raii_event &&wevent)
{
    rEvent = std::move(revent);
    wEvent = std::move(wevent);
    struct timeval tv ={ 30, 0 };
    event_add(rEvent.get(), &tv); // add read event into event_baseF
}
void channel::addReadEvent(size_t timeout)
{
    struct timeval tv ={ 20, 0 };
    if (timeout > 0)
    {
        tv.tv_sec = timeout;
    }

    event_add(rEvent.get(), &tv);
}

void channel::addWriteEvent(size_t timeout)
{
    struct timeval tv ={ 10, 0 };
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
    if (stop)
    {
        return false;
    }
    wBuf.append(buffer, l);
    wBuf.writesocket(fd);
    addWriteEvent(WRITETIMEOUT);
    return true;
}
void channel::onDisconnect(evutil_socket_t fd)
{
    std::shared_ptr<socketholder> hld = holder.lock();
    if (hld == nullptr)
    {
        return;
    }
    hld->onDisconnect(fd);
}

void channel::onChannelRead(short events, void *ctx)
{
    std::shared_ptr<socketholder> hld = holder.lock();
    if (hld == nullptr)
    {
        return;
    }
    cout << "onChannelRead start " << fd << endl;
    if (events & EV_TIMEOUT)
    {
        // 判斷心跳是否超時
        if (isHeartBrakeExpired())
        {
            {
                std::unique_lock<std::mutex> lock(cMutex);
                cout << "onRead timeout Expired " << fd << endl;
                removeWriteEvent();
                removeReadEvent();

            }
            hld->onDisconnect(fd);
            close(fd);
            state = CLOSE;
        }
        else
        {
            cout << "onRead timeout " << fd << " write buffer size " << wBuf.size() << endl;
            if (stop == true)
            {
                if (state == INIT)
                {
                    std::unique_lock<std::mutex> lock(cMutex);
                    if (wBuf.size() == 0)
                    {
                        shutdown(fd, SHUT_WR);
                        state = SHUTDOWN;
                        addReadEvent(READTIMEOUT);
                    }
                    else
                    {
                        wBuf.writesocket(fd);
                        addWriteEvent(WRITETIMEOUT);
                        addReadEvent(READTIMEOUT);
                    }
                    return;
                }
                else if (state == SHUTDOWN)
                {
                    removeReadEvent();
                    removeWriteEvent();
                    hld->onDisconnect(fd);
                    close(fd);
                    state = CLOSE;
                }
                return;
            }

            addReadEvent(READTIMEOUT);
        }

        return;
    }

    updateHearBrakeExpired();
    if (state == CLOSE)
    {
        cout << "when read task run, socket is close,so return" << endl;
        return;
    }
    auto size = rBuf.readsocket(fd);
    if (0 == size || -1 == size)
    {
        cout << "remote socket is close for socket: " << fd << endl;
        {
            std::unique_lock<std::mutex> lock(cMutex);
            stop = true;
            removeReadEvent();
            removeWriteEvent();
        }
        hld->onDisconnect(fd);
        close(fd);
        state = CLOSE;
        return;
    }
    // TODO: decode read buffer
    rBuf.toString();
    send(rBuf.readbegin(), rBuf.size());
    // wBuf.append(rBuf.readbegin(), rBuf.size());
    // wBuf.writesocket(socket_fd);
    // addWriteEvent(WRITETIMEOUT);
    rBuf.reset();
    addReadEvent(READTIMEOUT);
}

void channel::onChannelWrite(short events, void *ctx)
{
    std::shared_ptr<socketholder> hld = holder.lock();
    if (hld == nullptr)
    {
        return;
    }
    if (events & EV_TIMEOUT)
    {
        {
            std::unique_lock<std::mutex> lock(cMutex);
            stop = true;
            cout << "write timeout" << endl;
            removeWriteEvent();
            removeReadEvent();
        }
        hld->onDisconnect(fd);
        close(fd);
        state = CLOSE;
        return;
    }
    updateHearBrakeExpired();

    if (state == CLOSE)
    {
        cout << "when write task run, socket is close,so return" << endl;
        return;
    }
    {
        std::unique_lock<std::mutex> lock(cMutex);
        if (wBuf.size() > 0)
        {
            auto size = wBuf.writesocket(fd);
            addWriteEvent(WRITETIMEOUT);
            cout << "not finish write" << endl;
            return;
        }
    }
    if (stop && state == INIT)
    {
        //shutdown write mode
        shutdown(fd, SHUT_WR);
        state = SHUTDOWN;
    }
    else if (stop)
    {
        removeWriteEvent();
        removeReadEvent();
        hld->onDisconnect(fd);
        close(fd);
        state = CLOSE;
    }
}

void channel::closeSafty()
{
    std::unique_lock<std::mutex> lock(cMutex);
    stop = true;
    if (wBuf.size() <= 0)
    {
        cout << "closeSafty :" << fd << endl;
        shutdown(fd, SHUT_WR);
        state = SHUTDOWN;
    }
}