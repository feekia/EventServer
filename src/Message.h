#pragma once
#ifndef MESSAGE_H
#define MESSAGE_H
#include "Runnable.h"
class Message{
public:
    int what;
    int arg1;
    int arg2;

	Message();
	Message(int what);
	Message(int what, int arg1);
	Message(int what, int arg1, int arg2);
	~Message();
	static Message* getPostMessage(Runnable r);

private:

	long when;
	Runnable callback;


};
#endif
