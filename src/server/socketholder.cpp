#include "socketholder.h"
socketholder::socketholder():isStop(false)
{
    watcherGroup.reserve(LOOP_MAX);
    pool = std::make_unique<ThreadPool>(LOOP_MAX);
}
socketholder::~socketholder()
{
    std::vector<raii_event_base>::iterator iter=watcherGroup.begin();
    for ( ;iter!=watcherGroup.end();)
    {
        event_base_loopexit(iter->get(), nullptr);
    }
    std::vector<raii_event_base>().swap(watcherGroup);
    events;
    pool = nullptr;
}

void socketholder::onConnect(evutil_socket_t fd)
{
    if(isStop)
    {
        cout << " socketholder is stop" << endl;
        return;
    }
    auto raii_base = obtain_event_base();
	auto base = raii_base.get();

	auto raii_socket_event = obtain_event(base, -1, 0, nullptr, nullptr);
	auto event = raii_socket_event.get();
}
void socketholder::onRead(evutil_socket_t socket_fd, short events, void *ctx)
{

}
void socketholder::onWrite(evutil_socket_t socket_fd, short events, void *ctx)
{

}

void socketholder::onDisconnect(evutil_socket_t fd)
{

}

void socketholder::stop()
{
    isStop = true;
}
