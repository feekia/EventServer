#include <signal.h>
#include <memory.h>
#include <array>
#include "socketholder.h"

using namespace std;

void onEvent(evutil_socket_t socket_fd, short events, void *ctx);
socketholder *socketholder::instance = nullptr;
socketholder::socketholder() : isStop(false), pools(24)
{
    for (int i = 0; i < READ_LOOP_MAX; i++)
    {
        rwatchers[i] = obtain_event_base();
        watcher_thread[i] = std::thread([this, i]() {
            event_base_loop(rwatchers[i].get(), EVLOOP_NO_EXIT_ON_EMPTY);
            cout << "in loop id: " << i << endl;
            // std::this_thread::sleep_for(std::chrono::seconds(1));
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
    // cout << "new connection fd: " << fd << endl;
    std::shared_ptr<channel> pChan = std::make_shared<channel>(shared_from_this(), fd);
    chns[id].emplace(fd, pChan);

    auto base = rwatchers[fd % READ_LOOP_MAX].get();
    auto rwevent = obtain_event(base, fd, EV_READ | EV_TIMEOUT, onEvent, pChan.get());
    pChan->listenWatcher(std::move(rwevent));
}

void socketholder::onDisconnect(evutil_socket_t fd)
{
    cout << "socketholder onDisconnect fd: " << fd << endl;
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
    if (pair != chns[fd % READ_LOOP_MAX].end())
    {
        return pair->second->shared_from_this();
    }
    else
    {
        return nullptr;
    }
}

void onEvent(evutil_socket_t socket_fd, short events, void *ctx)
{
    auto sptr = socketholder::getShared_ptr();
    if (sptr == nullptr)
    {
        return;
    }
    try
    {
        auto chan = ((channel *)ctx)->shared_from_this();
        if (sptr->isStop)
        {
            chan->closeSafty();
        }

        // if (chan->isProcing())
        // {
        //     cout << "is proccessing : " << socket_fd << endl;
        //     return;
        // }
        chan->setProcing(true);
        sptr->pools.enqueue([chan, events]() {
            chan->handleEvent(events);
        });
    }
    catch (const std::bad_weak_ptr &e)
    {
        std::cerr << e.what() << '\n';
    }
}