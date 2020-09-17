
#include <chrono>
#include <algorithm>
#include <iostream>
#include "Handler.h"
#include "Message.h"
#define LOGENTER (std::cout << "This is FUNCTION " << __func__<<  std::endl)
Handler::Handler():stop(false),stopWhenEmpty(false){
	looper = std::thread(
		[this](){
			for(;;)
			{
				Message msg;
				{
					std::unique_lock<std::mutex> lock(this->queue_mutex);
					if(this->msg_Q.empty()){
						this->condition.wait(lock, [this]{ return this->stop || this->stopWhenEmpty || !this->msg_Q.empty();});
					}else{
						this->condition.wait_until(lock, this->msg_Q.back().when, [this]{ return this->stop || this->stopWhenEmpty || !this->msg_Q.empty(); });
					}

					if(this->stopWhenEmpty && this->msg_Q.empty())
						return;
					if(stop){
						msg_Q.clear();
						return;
					}

					msg = std::move(msg_Q.back());
					msg_Q.pop_back();
				}
				this->dispatchMessage(msg);
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

void Handler::handleMessage(Message& msg){
	std::cout << "IN Handler " << __func__<< " what:" << msg.m_what <<  std::endl;
}

bool Handler::sendMessageAtTime(Message& msg, long uptimeMillis){
	if(uptimeMillis < 0 )
		return false;

	msg.setWhen(uptimeMillis);

	std::unique_lock<std::mutex> lock(queue_mutex);
	auto i = std::find(msg_Q.begin(),msg_Q.end(),msg);
	msg_Q.erase(i);

	msg_Q.push_back(msg);
	std::sort(msg_Q.begin(), msg_Q.end(),std::greater<Message>());
	condition.notify_one();
	return true;
}
bool Handler::sendMessage(Message& msg){
	return false;

	std::unique_lock<std::mutex> lock(queue_mutex);
	auto i = find(msg_Q.begin(),msg_Q.end(),msg);
	if(i != msg_Q.end())
		msg_Q.erase(i);

	msg_Q.push_back(msg);
	std::sort(msg_Q.begin(), msg_Q.end(),std::greater<Message>());
	condition.notify_one();
	return true;
}

bool Handler::sendEmptyMessage(int what){
	return sendEmptyMessage(what ,0);
}

bool Handler::sendEmptyMessage(int what,long uptimeMillis){

	if(what < 0 || uptimeMillis < 0)
		return false;

	Message msg(what);
	msg.setWhen(uptimeMillis);

	std::unique_lock<std::mutex> lock(queue_mutex);

	std::vector<Message>::iterator i = find(msg_Q.begin(),msg_Q.end(),msg);
	if (i != msg_Q.end()){
		msg_Q.erase(i);
	}

	msg_Q.push_back(msg);
//	std::sort(msg_Q.begin(), msg_Q.end(),ValComp<Message>());
	// 跟进时间进行降序排列
	std::sort(msg_Q.begin(), msg_Q.end(),std::greater<Message>());

	condition.notify_one();
	return true;
}

bool Handler::post(std::function<void()> &&f){
	return postAtTime(std::forward<std::function<void()>>(f),0);
}
bool Handler::postAtTime(std::function<void()> &&f, long uptimeMillis){

	if(f == nullptr || uptimeMillis < 0){
		return false;
	}

	std::unique_lock<std::mutex> lock(queue_mutex);
	Message msg;
	msg.setWhen(uptimeMillis);
	msg.setFunction(std::forward<std::function<void()>>(f));
	msg_Q.push_back(msg);
	std::sort(msg_Q.begin(), msg_Q.end(),std::greater<Message>());

	return true;
}

void Handler::removeMessages(int what){
	if(what < 0)
		return;

	std::unique_lock<std::mutex> lock(queue_mutex);

	auto i = find(msg_Q.begin(),msg_Q.end(),what);
	if (i != msg_Q.end()){
		msg_Q.erase(i);
	}

	condition.notify_one();
}

void Handler::removeCallbackAndMessages(){
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


bool Handler::isQuiting(){
	std::unique_lock<std::mutex> lock(queue_mutex);
	if(stop || stopWhenEmpty)
		return true;
	return false;
}

void Handler::dispatchMessage(Message& msg){
	if(msg.task != nullptr){
		msg.task();
	}else{
		if(msg.m_what < 0)
			return;
		handleMessage(msg);
	}
}
