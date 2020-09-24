#include <signal.h>
#include <memory.h>
#include <array>
#include "socketholder.h"

using namespace std;

void onEvent(evutil_socket_t socket_fd, short events, void *ctx);
socketholder *socketholder::instance = nullptr;
socketholder::socketholder() : isStop(false), pools(READ_LOOP_MAX * 2)
{
    for (int i = 0; i < READ_LOOP_MAX; i++)
    {
        rwatchers[i] = obtain_event_base();
        watcher_thread[i] = std::thread([this, i]() {
            event_base_loop(rwatchers[i].get(), EVLOOP_NO_EXIT_ON_EMPTY);
            cout << "exit loop id: " << i << endl;
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
    event_base *base = nullptr;
    std::shared_ptr<channel> pChan = std::make_shared<channel>(shared_from_this(), fd);
    {
        std::unique_lock<std::mutex> lock(syncMutex[id]);
        chns[id].emplace(fd, pChan);
        base = rwatchers[fd % READ_LOOP_MAX].get();
    }

    auto rwevent = obtain_event(base, fd, EV_WRITE | EV_READ | EV_TIMEOUT | EV_ET | EV_PERSIST, onEvent, pChan.get());
    pChan->listenWatcher(std::move(rwevent));
}

void socketholder::onDisconnect(evutil_socket_t fd)
{
    // cout << "socketholder close fd: " << fd << " for reson: " << strerror(errno) << endl;
    auto id = fd % READ_LOOP_MAX;
    std::unique_lock<std::mutex> lock(syncMutex[id]);
    chns[id].erase(fd); // erase before close ,because system will create the same fd,and insert chns[i].
    close(fd);
    if (isStop && chns[id].size() == 0)
    {
        event_base_loopexit(rwatchers[id].get(), nullptr);
        cout << "stop loop id: " << id << endl;
    }
}

void socketholder::closeIdleChannel()
{
    for (int i = 0; i < READ_LOOP_MAX; i++)
    {
        std::unique_lock<std::mutex> lock(syncMutex[i]);
        std::map<evutil_socket_t, std::shared_ptr<channel>>::iterator it;
        for (it = chns[i].begin(); it != chns[i].end(); it++)
        {
            it->second->closeSafty();
        }
        if (chns[i].size() == 0)
        {
            event_base_loopexit(rwatchers[i].get(), nullptr);
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
    if (sptr == nullptr || ctx == nullptr)
    {
        cout << "args null" << endl;
        return;
    }
    try
    {
        auto chan = ((channel *)ctx)->shared_from_this();
        if (sptr->isStop)
        {
            chan->closeSafty();
        }

        sptr->pools.enqueue([chan, events]() {
            chan->handleEvent(events);
        },
                            socket_fd % sptr->pools.getSize());
    }
    catch (const std::bad_weak_ptr &e)
    {
        std::cerr << e.what() << '\n';
    }
}
