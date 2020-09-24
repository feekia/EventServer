#include "channel.h"
#include "socketholder.h"
channel::channel(std::weak_ptr<socketholder> &&h, evutil_socket_t _fd) : fd(_fd), holder(h), stop(false), isClose(false)
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
            cout << "tid: " << std::this_thread::get_id() << " 10s cnt : " << cnt << endl;
            locald_r = std::chrono::system_clock::now().time_since_epoch();
            cnt = 0;
        }
    }

    updateHearBrakeExpired();
    auto size = rBuf.readsocket(fd);
    if (-1 == size)
    {
        // cout << "read socket error : " << strerror(errno) << endl;
        cout << "read socket error close : " << errno <<  endl;
        stop = true;
        handleClose();
        return;
    }
    if (!stop && rBuf.size() > 0)
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

    if (stop && wBuf.size() == 0 && state == INIT)
    {
        shutdown(fd, SHUT_WR);
        state = SHUTDOWN;
        // cout << "shutdown socket on read :" << fd << endl;
        return;
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
    if (stop && wBuf.size() == 0 && state == INIT)
    {
        shutdown(fd, SHUT_WR);
        state = SHUTDOWN;
        return;
    }
}

void channel::onChannelTimeout(short events, void *ctx)
{
    // cout << "onChannelTimeout :" << fd <<endl;
    uint64_t left = heartBrakeLeft();
    if (left < 0 || stop == true)
    {
        cout << "onChannelTimeout :" << fd << endl;
        handleClose();
        return;
    }

    if (wBuf.size() > 0 && state == INIT)
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
    if (stop && wBuf.size() == 0 && state == INIT)
    {
        shutdown(fd, SHUT_WR);
        state = SHUTDOWN;
        return;
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
    if (!stop)
    {
        stop = true;
        event_active(rwEvent.get(), EV_TIMEOUT, 1);
    }
}

void channel::handleEvent(short events)
{
    {
        std::unique_lock<std::mutex> lock(cMutex);
        if (CLOSE == state)
        {
            return;
        }
        if (events & EV_READ)
        {
            onChannelRead(events, nullptr);
        }
        else if (events & EV_WRITE)
        {
            onChannelWrite(events, nullptr);
        }
        else if (events & EV_TIMEOUT)
        {
            onChannelTimeout(events, nullptr);
        }

    }
    // 单独提出来调用 onDisconnect 是为了避免 channel里面的锁和socketholder里面的锁竞争，影响效率或者导致死锁
    if (CLOSE == state)
    {
        if (isClose)
        {
            return;
        }
        isClose = true;
        std::shared_ptr<socketholder> hld = holder.lock();
        if (hld == nullptr)
        {
            cout << "handleClose :" << fd << endl;
        }

        hld->onDisconnect(fd);
    }
}