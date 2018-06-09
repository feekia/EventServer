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

	bool sendMessageAtTime(Message& msg, long uptimeMillis);
	bool sendMessage(Message& msg);
	bool sendEmptyMessage(int what);

	void removeMessages(int what);
	void removeAllMessages();

	void stopSafty(bool stopSafty);

	static bool sortMessageQueue(std::vector<Message> _q);
	static bool insertMessage(std::vector<Message> _q, Message &m);

	void handleMessage(Message& msg);

	/*
	 * for msgQueue sorted when insert,
	 * ascending order
	 */
	template<class T>
	class KeyComp {
	public:
		bool operator()(const T& t1,const T& t2) const {
			return (t1 > t2);
		}

	};

	template<class T>
	bool comp(const T &a, const T &b){
	    return a > b;
	}

private:
	std::vector<Message> msg_Q;

	std::multimap<std::chrono::system_clock::time_point, Message, KeyComp<std::chrono::system_clock::time_point>> msgQueue;
	std::mutex queue_mutex;
	std::condition_variable condition;
	std::thread looper;
	bool stop;
	bool stopWhenEmpty;
};

Handler::Handler():stop(false),stopWhenEmpty(false){
	looper(
		[this](){
			for(;;)
			{
				Message msg;
				{
					std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait_until(lock, this->msg_Q.back().when, [this]{ return this->stop || this->stopWhenEmpty || !this->msg_Q.empty(); });

					if(this->stopWhenEmpty && this->msg_Q.empty())
						return;
					if(stop){
						msg_Q.clear();
						return;
					}

					msg = std::move(msg_Q.back());

					msg_Q.pop_back();

				}
				this->handleMessage(msg);
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
	msg_Q.clear();

}



bool Handler::sendMessageAtTime(Message& msg, long uptimeMillis){
	if(uptimeMillis < 0 || msg == nullptr)
		return false;

	auto t = std::chrono::system_clock::now() + std::chrono::milliseconds(uptimeMillis);
	msg.setWhen(uptimeMillis);

	std::unique_lock<std::mutex> lock(queue_mutex);
	auto i = find(msg_Q.begin(),msg_Q.end(),msg);
	if(i != nullptr)
		msg_Q.erase(i);

	msg_Q.push_back(msg);
	std::sort(msg_Q.begin(), msg_Q.end(),comp<Message>);
	condition.notify_one();
	return true;
}
bool Handler::sendMessage(Message& msg){
	if(msg == nullptr)
		return false;

	std::unique_lock<std::mutex> lock(queue_mutex);
	auto i = find(msg_Q.begin(),msg_Q.end(),msg);
	if(i != nullptr)
		msg_Q.erase(i);

	msg_Q.push_back(msg);
	std::sort(msg_Q.begin(), msg_Q.end(),comp<Message>);
	condition.notify_one();
	return true;
}

bool Handler::sendEmptyMessage(int what){
	if(what < 0)
		return false;

	Message msg(what);

	std::unique_lock<std::mutex> lock(queue_mutex);

	auto i = find(msg_Q.begin(),msg_Q.end(),what);
	if(i != nullptr)
		msg_Q.erase(i);

	msg_Q.push_back(msg);
	std::sort(msg_Q.begin(), msg_Q.end(),comp<Message>);
	condition.notify_one();
	return true;
}
void Handler::removeMessages(int what){
	if(what < 0)
		return;

	std::unique_lock<std::mutex> lock(queue_mutex);

	auto i = find(msg_Q.begin(),msg_Q.end(),what);
	if(i != nullptr)
		msg_Q.erase(i);
	condition.notify_one();
}

void Handler::removeAllMessages(){
	std::unique_lock<std::mutex> lock(queue_mutex);
	msg_Q.clear();
}

void Handler::stopSafty(bool stopSafty){
	std::unique_lock<std::mutex> lock(queue_mutex);
	if(stopSafty){
		stopWhenEmpty = true;
	}else{
		stop = true;
	}
}
#endif
