#pragma once
#ifndef MESSAGE_H
#define MESSAGE_H
#include "Runnable.h"
class Message{
public:
    int m_what;
    int m_arg1;
    int m_arg2;
    std::function<void()> callback;
    std::chrono::system_clock::time_point when;

public:
	Message();
	Message(int what);
	Message(int what, int arg1);
	Message(int what, int arg1, int arg2);
	Message(int what, int arg1, int arg2,long uptimeMillis);

	void setWhen(long uptimeMillis);
	virtual ~Message();

	void setData(void* d){
		data = d;
	}

	void* getData(){
		return data;
	}
	bool operator>(Message& msg){
		return (this->when > msg.when);
	}
	bool operator==(Message& msg){
		return (this->m_what == msg.m_what);
	}

	bool operator==(int what){
		return (this->m_what == what);
	}

private:
	void* data;

};

Message::Message():Message(0, 0, 0, 0){}

Message::Message(int what, int arg1):Message(what, arg1, 0, 0){}

Message::Message(int what, int arg1, int arg2):Message(what, arg1, arg2, 0){}

Message::Message(int what, int arg1, int arg2,long uptimeMillis):m_what(what),m_arg1(arg1),m_arg2(arg2){
	when = std::chrono::system_clock::now() + std::chrono::milliseconds(uptimeMillis);
	data = nullptr;
}

void Message::setWhen(long uptimeMillis){
	when = std::chrono::system_clock::now() + std::chrono::milliseconds(uptimeMillis);
}
Message::~Message(){
	if(data != nullptr){
		delete data;
		data = nullptr;
	}
}
#endif
