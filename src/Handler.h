#pragma once
#ifndef HANDLER_H
#define HANDLER_H

#include <chrono>
#include <map>

#include "Message.h"
class Handler{
public:
	Handler();
	virtual ~Handler();
	virtual bool postAtTime(Runnable r, long uptimeMillis);
	virtual bool post(Runnable r){
		return postAtTime(r, 0);
	}
	bool sendMessageAtTime(Message&& msg, long uptimeMillis);
	bool sendMessage(Message&& msg);

	void removeMessages(int what);
	void removeCallbacksAndMessages();

	void stopSafty(bool stop);

	static bool sortMessageQueue(std::vector<Message> _q);
	static bool insertMessage(std::vector<Message> _q, Message &m);



private:
	std::vector<Message> queue;
	std::multimap<std::chrono::system_clock::time_point, std::shared_ptr<Message>> msgQueue;
	std::mutex queue_mutex;
	std::condition_variable condition;
	std::thread looper;
	bool stop;
};

Handler::Handler():stop(false){
	looper(
		[this](){
			for(;;)
			{
				std::function<void()> task;

				{
					std::unique_lock<std::mutex> lock(this->queue_mutex);
//                    this->condition.wait_for(lock, std::chrono::milliseconds(1), [this]{ return this->stop || !this->msgQueue.empty(); });
                    this->condition.wait_until(lock, this->msgQueue.begin()->first, [this]{ return this->stop || !this->msgQueue.empty(); });

					if(this->stop && this->msgQueue.empty())
						return;
					auto msg_ptr = this->msgQueue.begin()->second;
					*msg_ptr->callback();
					msgQueue.erase(msgQueue.begin());
				}

				task();
			}
		});
}
Handler::~Handler(){
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		stop = true;
	}
	condition.notify_all();
	looper.join();

}
bool Handler::postAtTime(Runnable r, long uptimeMillis){
	if(uptimeMillis < 0 || r == nullptr)
		return false;
	 auto t = std::chrono::system_clock::now() + std::chrono::milliseconds(uptimeMillis);
	 auto msg = std::make_shared<Message>();
	 std::shared_ptr<Message> dd  = msg;
	 msgQueue.insert(std::make_pair(t, msg));
	return true;
}
#endif
