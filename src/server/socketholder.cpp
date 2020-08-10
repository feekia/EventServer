#include <signal.h>
#include <memory.h>
#include <array>
#include "socketholder.h"

using namespace std;

void onRead(evutil_socket_t socket_fd, short events, void *ctx);
void onWrite(evutil_socket_t socket_fd, short events, void *ctx);
socketholder *socketholder::instance = nullptr;
socketholder::socketholder() : isStop(false), pools(5)
{
    for (int i = 0; i < READ_LOOP_MAX; i++)
    {
        rwatchers[i] = obtain_event_base();
        watcher_thread[i] = std::thread([this, i]() {
            event_base_loop(rwatchers[i].get(), EVLOOP_NO_EXIT_ON_EMPTY);
            cout << "in loop id: " << i << endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        });
    }
}
socketholder::~socketholder()
{
    socketholder::instance = nullptr;
}

void socketholder::onConnect(evutil_socket_t fd)
{
    if (isStop)
    {
        cout << " socketholder is stop" << endl;
        return;
    }
    auto id = fd % READ_LOOP_MAX;
    std::unique_lock<std::mutex> lock(syncMutex[id]);
    cout << "fd is: " << fd << endl;
    std::shared_ptr<channel> pChan = std::make_shared<channel>(shared_from_this(), fd);

    auto base = rwatchers[fd % READ_LOOP_MAX].get();
    auto r_event = obtain_event(base, fd, EV_READ | EV_TIMEOUT, onRead, nullptr);
    auto w_event = obtain_event(base, fd, EV_WRITE | EV_TIMEOUT, onWrite, nullptr);
    pChan->listenWatcher(std::move(r_event), std::move(w_event));
    chns[id].emplace(fd, pChan->shared_from_this());
}

void socketholder::onDisconnect(evutil_socket_t fd)
{
    cout << "socketholder onDisconnect" << endl;
    auto id = fd % READ_LOOP_MAX;
    std::unique_lock<std::mutex> lock(syncMutex[id]);
    chns[id].erase(fd);
    if (isStop && chns[id].size() == 0)
    {
        event_base_loopexit(rwatchers[id].get(), nullptr);
        cout << "onDisconnect exit loop id: " << id << endl;
    }
}
void socketholder::closeIdleChannel()
{
    for (int i = 0; i < READ_LOOP_MAX; i++)
    {
        std::unique_lock<std::mutex> lock(syncMutex[i]);
        if (chns[i].size() == 0)
        {
            event_base_loopexit(rwatchers[i].get(), nullptr);
            cout << "closeIdleChannel exit loop id: " << i << endl;
        }
        else
        {
            for (auto &kv : chns[i])
            {
                kv.second->closeSafty();
            }
        }
    }
}
void socketholder::waitStop()
{

    cout << " socketholer waitStop" << endl;
    isStop = true;
    closeIdleChannel();
    for (int i = 0; i < READ_LOOP_MAX; i++)
    {
        if (watcher_thread[i].joinable())
        {
            watcher_thread[i].join();
        }
    }
}

std::shared_ptr<channel> socketholder::getChannel(evutil_socket_t fd)
{
    auto id = fd % READ_LOOP_MAX;
    std::unique_lock<std::mutex> lock(syncMutex[id]);
    auto pair = chns[fd % READ_LOOP_MAX].find(fd);
    if (pair->second != nullptr)
    {
        return pair->second->shared_from_this();
    }
    else
    {
        return nullptr;
    }
}

void onRead(evutil_socket_t socket_fd, short events, void *ctx)
{
    auto sptr = socketholder::getShared_ptr();
    if (sptr == nullptr)
    {
        cout << "onRead sptr nullptr" << endl;
        return;
    }

    auto chan = sptr->getChannel(socket_fd);
    if (chan == nullptr)
    {
        cout << "onRead chan nullptr" << endl;
        return;
    }

    sptr->pools.enqueue([chan, events]() {
        chan->onChannelRead(events, nullptr);
    });
}
void onWrite(evutil_socket_t socket_fd, short events, void *ctx)
{
    cout << "onWrite start" << endl;
    auto sptr = socketholder::getShared_ptr();
    if (sptr == nullptr)
    {
        return;
    }

    auto chan = sptr->getChannel(socket_fd);
    if (chan == nullptr)
    {
        return;
    }
    sptr->pools.enqueue([chan, events]() {
        chan->onChannelWrite(events, nullptr);
    });
}