#include "channel.h"
#include "socketholder.h"
channel::channel(std::weak_ptr<socketholder> &&h, evutil_socket_t _fd) : fd(_fd), holder(h), stop(false), isProc(false)
{
    state.store(INIT);
    std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
    std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(d);
    timestamp = sec.count();
}

channel::~channel()
{
}

void channel::listenWatcher(raii_event &&events)
{
    rwEvent = std::move(events);
    struct timeval tv = {HEARTBITTIMEOUT, 0};
    int ret = event_add(rwEvent.get(), &tv); // add read event into event_baseF
    if (ret != 0)
    {
        cout << "listenWatcher event error :" << fd << endl;
        return;
    }
}

int32_t channel::send(char *buffer, size_t l)
{
    std::unique_lock<std::mutex> lock(cMutex);
    if (stop == true)
    {
        return -1;
    }
    wBuf.append(buffer, l);
    wBuf.writesocket(fd);
    updateHearBrakeExpired();
    return l;
}

thread_local int cnt = 0;
thread_local std::chrono::system_clock::duration locald_r;
void channel::onChannelRead(short events, void *ctx)
{
    std::shared_ptr<socketholder> hld = holder.lock();
    if (hld == nullptr)
    {
        return;
    }

    cnt++;
    if (cnt == 1)
    {
        locald_r = std::chrono::system_clock::now().time_since_epoch();
    }

    if (cnt > 20)
    {
        std::chrono::system_clock::duration d = std::chrono::system_clock::now().time_since_epoch();
        std::chrono::milliseconds msec = std::chrono::duration_cast<std::chrono::milliseconds>(d);
        std::chrono::milliseconds localmsec = std::chrono::duration_cast<std::chrono::milliseconds>(locald_r);
        int64_t diff = msec.count() - localmsec.count();
        if (diff > 10 * 1000)
        {
            cout << "qps tid: " << std::this_thread::get_id() << " cnt : " << cnt << endl;
            locald_r = std::chrono::system_clock::now().time_since_epoch();
            cnt = 0;
        }
    }

    updateHearBrakeExpired();
    auto size = rBuf.readsocket(fd);
    if (0 == size || -1 == size)
    {
        cout << "read socket error : " << strerror(errno) << endl; 
        stop = true;
        handleClose();
        return;
    }
    if (!stop)
    {
        wBuf.append(rBuf.readbegin(), rBuf.size());
        rBuf.reset();
        int ret = wBuf.writesocket(fd);
        if (ret == -1)
        {
            cerr << "write error for socket close: " << strerror(errno) << endl;
            handleClose();
            return;
        }
    }

    if (stop)
    {
        if (state == INIT)
        {
            shutdown(fd, SHUT_WR);
            state = SHUTDOWN;
            return;
        }
        else
        {
            handleClose();
            return;
        }
    }
}

void channel::onChannelWrite(short events, void *ctx)
{
    std::shared_ptr<socketholder> hld = holder.lock();
    if (hld == nullptr)
    {
        return;
    }

    if (wBuf.size() > 0)
    {
        updateHearBrakeExpired();
        int ret = wBuf.writesocket(fd);
        if (ret == -1)
        {
            cerr << "write error for socket close: " << strerror(errno) << endl;
            handleClose();
            return;
        }
    }
    if (stop)
    {
        shutdown(fd, SHUT_WR);
        state = SHUTDOWN;
        return;
    }
}

void channel::onChannelTimeout(short events, void *ctx)
{
    uint64_t left = heartBrakeLeft();
    if (left < 0 || stop)
    {
        handleClose();
        return;
    }

    if (wBuf.size() > 0)
    {
        updateHearBrakeExpired();
        int ret = wBuf.writesocket(fd);
        if (ret == -1)
        {
            cerr << "write error for socket close: " << strerror(errno) << endl;
            handleClose();
            return;
        }
    }
}

void channel::handleClose()
{

    int pending = event_pending(rwEvent.get(), EV_WRITE | EV_READ | EV_TIMEOUT | EV_ET | EV_PERSIST, nullptr);
    if (pending != 0)
    {
        event_del(rwEvent.get());
    }

    stop = true;
    std::shared_ptr<socketholder> hld = holder.lock();
    if (hld == nullptr)
    {
        goto exit;
    }
    hld->onDisconnect(fd);

exit:
    close(fd);
    state = CLOSE;
    return;
}

void channel::monitorEvent(short events, int64_t timeout)
{
    event_callback_fn fn = event_get_callback(rwEvent.get());
    void *arg = event_get_callback_arg(rwEvent.get());
    timeval tv = {timeout, 0};

    event_base *base = event_get_base(rwEvent.get());
    event_set(rwEvent.get(), fd, events, fn, arg);
    event_base_set(base, rwEvent.get());
    event_add(rwEvent.get(), &tv);
}
void channel::closeSafty()
{
    stop = true;
}