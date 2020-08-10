## EventServer
libevent开发server端

1.handler 仿android实现

2.threadpool实现

3.libevent 多线程io

## 框架层次设计
payload protocol(json or other) 
mqtt
TLS
1个acceptor thread
8 个 event_base loop
write 和 read分别用不同的event，
write数据时对应的event有需要时才监控
1 个read write thread pool


## 读写及缓存设计：
接收：
读完当前的socket数据（读过程中，如果buffer不够长，则重新分配）
检测长度是否够
如果数据的长度 >= 一个完整数据包的长度，则开始解析
如果解析完上一包数据，剩余的长度还能作为一包处理，则继续解析，直到数据不够一包，或者数据解析完。
当数据不够一包时，将数据挪到buffer的首地址。
将read事件add到base中。
（超时未读到足够数据怎么处理，错误数据怎么处理：断开）
如果buffer刚好被解析完，释放该buffer

如果接收到心跳数据怎么办？数据包比较小，如果用buffer来存储，消耗会比较大。

写数据
检测当前fd是否有写任务，如果有，则将数据添加到末尾。
如果没有则先进行写，如果写到一半报：EAGAIN，则将剩余数据添加到buffer中，等到EV_WRITE事件重新将数据写出去。
如果写的时候，又有其他线程想写怎么办？ 能加锁来操作么？答案：可以，因为非阻塞写，写只是把数据拷贝进内核，
内存的缓存满了，就拷贝不进去了，此时可以监听EV_WRITE事件，操作过程不会消耗多少时间，是可以采用锁来操作的。
                  
  
## 编译
./autofen.sh

./configure

make

## clean

make distclean

## 执行
./src/server/EventServer

./src/client/EventClient

说明：echo服务器简单实现

author：afreeliyunfeil@163.com
