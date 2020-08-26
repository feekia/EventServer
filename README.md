## EventServer
libevent开发server端

1.handler 仿android实现

2.threadpool实现

3.libevent 多线程io

## 框架层次设计
payload protocol(json or other) (未开始)
mqtt (未开始)
TLS(未开始)
1个acceptor thread
8 个线程 event_base loop,write 和 read分别用不同的event，
24个线程进行读写操作


## 读写及缓存设计：
接收：
自定义读读缓存设计

写数据
自定义写缓存设计
  
## 编译
./autofen.sh

./configure

make

## clean

make distclean

## 执行
./src/server/EventServer

./src/client/EventClient

说明：
server负责监听链接，以及在收到client数据时，将数据重新发回client
每个client 创建一万个链接，并用一个线程轮流给1万个socket发送数据，

author：afreeliyunfeil@163.com
