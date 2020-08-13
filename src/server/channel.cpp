#include "channel.h"
#include "socketholder.h"
channel::channel(std::weak_ptr<socketholder> &&h, evutil_socket_t _fd) : fd(_fd), holder(h), stop(false)
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
    struct timeval tv = {30, 0};
    int ret = event_add(rEvent.get(), &tv); // add read event into event_baseF
    if (ret != 0)
    {
        cout << "listenWatcher event error :" << fd << endl;
    }
}
void channel::addReadEvent(size_t timeout)
{
    struct timeval tv = {20, 0};
    if (timeout > 0)
    {
        tv.tv_sec = timeout;
    }

    int ret = event_add(rEvent.get(), &tv);
    if (ret != 0)
    {
        cout << "add read event error :" << fd << endl;
    }
}

void channel::addWriteEvent(size_t timeout)
{
    struct timeval tv = {10, 0};
    if (timeout > 0)
    {
        tv.tv_sec = timeout;
    }
    int ret = event_add(wEvent.get(), &tv);
    if (ret != 0)
    {
        cout << "add write event error :" << fd << endl;
    }
}

bool channel::send(char *buffer, size_t l)
{
    if (stop == true)
    {
        return false;
    }
    std::unique_lock<std::mutex> lock(cMutex);
    if (stop == true)
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
    // cout << "onChannelRead start " << fd << endl;
    if (events & EV_TIMEOUT)
    {
        // 判斷心跳是否超時
        if (isHeartBrakeExpired())
        {
            {
                std::unique_lock<std::mutex> lock(cMutex);
                cout << "onRead timeout Expired " << fd << endl;
                removeRWEvent();
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
                        cout << "shutdown write in onChannelRead, socket:  " << fd << endl;
                        shutdown(fd, SHUT_WR);
                        state = SHUTDOWN;
                    }
                    else
                    {
                        wBuf.writesocket(fd);
                        addWriteEvent(WRITETIMEOUT);
                    }
                    addReadEvent(READTIMEOUT);
                    return;
                }
                else if (state == SHUTDOWN)
                {
                    cout << "nomal close, socket:  " << fd << endl;
                    removeRWEvent();
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
        {
            std::unique_lock<std::mutex> lock(cMutex);
            stop = true;
            cout << "remote socket is close " << fd << endl;
            removeRWEvent();
        }
        hld->onDisconnect(fd);
        close(fd);
        state = CLOSE;
        return;
    }
    // TODO: decode read buffer
    // rBuf.toString();

    if (stop == true && state == INIT)
    {
        cout << "shutdown write mode IN READ" << fd << endl;
        shutdown(fd, SHUT_WR);
        state = SHUTDOWN;
        rBuf.reset();
        addReadEvent(READTIMEOUT);
        return;
    }
    else if (stop == true && state == SHUTDOWN)
    {
        cout << "shutdown state IN READ: " << fd << endl;
#if 0
        rBuf.reset();
        addReadEvent(READTIMEOUT);
#else
        removeRWEvent();
        hld->onDisconnect(fd);
        close(fd);
        state = CLOSE;
#endif
        return;
    }
    send(rBuf.readbegin(), rBuf.size());
    // wBuf.append(rBuf.readbegin(), rBuf.size());
    // wBuf.writesocket(socket_fd);
    // addWriteEvent(WRITETIMEOUT);
    rBuf.reset();
    addReadEvent(READTIMEOUT);
}

void channel::onChannelWrite(short events, void *ctx)
{
    // cout << "onChannelWrite start: " << fd << endl;
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
            cout << "write timeout close socket" << endl;
            removeRWEvent();
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
        // cout << "finish write :" << fd << endl;
    }
    if (stop == true && state == INIT)
    {
        cout << "shutdown write mode" << endl;
        shutdown(fd, SHUT_WR);
        state = SHUTDOWN;
    }
    else if (stop == true)
    {
        cout << "close socket in  onChannelWrite" << endl;
        removeRWEvent();
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