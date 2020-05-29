# EventServer
libevent开发server端

1.handler 仿android实现

2.threadpool实现

3.libevent 多线程io

-------------------------            |
payload protocol(json or other)      | \
-------------------------            |  业务层逻辑
mqtt                             |   | /
-------------------------        |   |
TLS                              | 
-------------------------        | 
1个acceptor                      | 
10 个 base loop（write & read）  | \
1 个read thread pool             | |  接入层
1个write thread pool             |/ 
-------------------------        | 
TCP                              | 
-------------------------        | 
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
