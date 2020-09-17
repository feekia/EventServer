#pragma once
#ifndef HANDLER_H
#define HANDLER_H

#include <chrono>
#include <map>
#include <mutex>
#include <vector>
#include <thread>
#include <condition_variable>



#include "Message.h"

/*
 * Handler will run in it's own thread, you don't want to care about it.
 * Message will be proccess by the Handler. Two ways to add your task to the Handler.
 * 1. send message to the handler
 * 2. post the task(Function) to handler
 */

class Handler{
public:
	Handler();
	virtual ~Handler();

	bool sendMessageAtTime(Message& msg, long uptimeMillis);
	bool sendMessage(Message& msg);
	bool sendEmptyMessage(int what);
	bool sendEmptyMessage(int what, long uptimeMillis);

	bool post(std::function<void()> &&f);
	bool postAtTime(std::function<void()> &&f, long uptimeMillis);

	void removeMessages(int what);
	void removeCallbackAndMessages();

	void stopSafty(bool stopSafty);

	bool isQuiting();

	virtual void handleMessage(Message& msg);

	void dispatchMessage(Message& msg);

	/*
	 * for msgQueue sorted when insert,
	 * ascending order
	 */
	template<class T>
	class ValComp {
	public:
		bool operator()(const T& t1,const T& t2) const {
			return (t1 < t2);
		}

	};

private:
	std::vector<Message> msg_Q;

	std::mutex queue_mutex;
	std::condition_variable condition;
	std::thread looper;
	bool stop;
	bool stopWhenEmpty;
};

#endif
