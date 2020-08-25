#include "channel.h"
#include "socketholder.h"
channel::channel(std::weak_ptr<socketholder> &&h, evutil_socket_t _fd) : fd(_fd), holder(h), stop(false)
{
    state.store(INIT);
    std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
    std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(d);
    timestamp = sec.count();
    wFinish = true;
    rFinish = true;
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
    int wpending = event_pending(wEvent.get(), EV_WRITE | EV_TIMEOUT, nullptr);
    if (wpending == (EV_WRITE | EV_TIMEOUT))
    {
        return;
    }
    int ret = event_add(wEvent.get(), &tv);
    if (ret != 0)
    {
        cout << "add write event error :" << fd << endl;
    }
}

int32_t channel::send(char *buffer, size_t l)
{
    std::unique_lock<std::mutex> lock(cMutex);
    if (stop == true)
    {
        return 0;
    }

    wBuf.append(buffer, l);
    if (wFinish == false)
    {
        std::thread([chan = shared_from_this()]() {
            chan->send_internal();
        });
    }

    return l;
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
        if (isHeartBrakeExpired() || stop == true)
        {
            {
                std::unique_lock<std::mutex> lock(cMutex);
                cout << "onRead timeout Expired " << fd << endl;
                stop = true;
                if (wFinish == true)
                {
                    if (state == INIT)
                    {
                        cout << "shutdown write mode IN READ" << fd << endl;
                        shutdown(fd, SHUT_WR);
                        state = SHUTDOWN;
                        addReadEvent(SHUTDOWNTIMEOUT);
                        return;
                    } // else will goto onDisconnect
                }
                else
                {
                    addReadEvent(SHUTDOWNTIMEOUT);
                    return;
                }
            }
            // onDisconnect
            {
                hld->onDisconnect(fd);
                close(fd);
                state = CLOSE;
            }
            return;
        }
    }
    else if (events & EV_READ)
    {
        updateHearBrakeExpired();
        auto size = rBuf.readsocket(fd);
        if (0 == size || -1 == size)
        {
            {
                std::unique_lock<std::mutex> lock(cMutex);
                stop = true;
                cout << "remote socket is close " << fd << endl;
                removeRWEvent();
                wFinish = true;
            }
            hld->onDisconnect(fd);
            close(fd);
            state = CLOSE;
            return;
        }

        // TODO: rBuf notify
        send(rBuf.readbegin(), rBuf.size());
        rBuf.reset();
        if (stop == true)
        {
            {
                std::unique_lock<std::mutex> lock(cMutex);
                if (wFinish == true)
                {
                    if (state == INIT)
                    {
                        cout << "shutdown write mode IN READ" << fd << endl;
                        shutdown(fd, SHUT_WR);
                        state = SHUTDOWN;
                        addReadEvent(SHUTDOWNTIMEOUT);
                        return;
                    } // else will goto onDisconnect
                }
                else
                {
                    addReadEvent(SHUTDOWNTIMEOUT);
                    return;
                }
            }
            // onDisconnect
            {
                hld->onDisconnect(fd);
                close(fd);
                state = CLOSE;
            }
            return;
        }
        else
        {
            addReadEvent(READTIMEOUT);
        }

        return;
    }
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
        std::unique_lock<std::mutex> lock(cMutex);
        stop = true;
        cout << "write timeout close socket" << endl;
        wBuf.reset();
        wFinish = true;
        state = SHUTDOWN;
        return;
    }
    else if (events & EV_WRITE)
    {
        if (state != INIT)
        {
            wFinish = true;
            cout << "socket is closing or closed,so return" << endl;
            return;
        }
        updateHearBrakeExpired();
        int ret = send_internal();
        if (ret == -1)
        { // BADEF
            wFinish = true;
        }
    }
}
void channel::closeSafty()
{
    stop = true;
}