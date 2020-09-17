
## EventServer （基于libevent开发server & client）
1. handler 仿android实现
2. threadpool实现
3. libevent 多线程io

## 框架层次设计
1. payload protocol(json or other) (未开发)
2. mqtt (未开发)
3. TLS(准备开发)
4. 基础长连接网络框架


## 读写及缓存设计：（参考buffer.h）
    接收：自定义读读缓存设计
    写数据：自定义写缓存设计

## 注意事项
1. libevent需要 2.1.8以上的版本，否则不支持该宏：EVLOOP_NO_EXIT_ON_EMPTY
2. GCC 需要支持C++14 请自行安装高版本的GCC
3. 编译脚本生成使用的是automake autoconf ，请百度自行安装相应工具。
  
## 编译
    ./autogen.sh
    ./configure
    make

## clean
    make clean

    如使用以下命令，则需要重新执行 autogen.sh 以及 configure
    make distclean

## 执行（server 和 client 在同一台主机）
    ./src/server/EventServer 9950
    ./src/client/EventClient 127.0.0.1 9950

目前在同一台主机上测试，由于端口分配的限制，只能测试5万个链接，5个client线程分别轮询1万个socket 进行发送数据，服务端会原样返回client发过去的数据。

说明：
    server负责监听链接，以及在收到client数据时，将数据重新发回client
    每个client 创建一万个链接，并用2个线程轮流给1万个socket发送数据，1个线程负责收数据

测试结果：
    4core CPU, 8GB 内存
    峰值并发连接2.9K/s; 
    5万连接，32bytes 数据收发;
    峰值QPS:25w 稳定QPS:15w

## 长连接场景线程池设计

1. 一个线程负责acceptor，然后分发到 8个loop（8个线程承担）;分发方式 base[fd % 8] 
2. 开辟一个长度为 size(loop 数量的 2倍或者3倍)的线程池
3. fd的read 、write、timeout 指定到线程池对应线程执行， 线程指定方式：thread[fd % size]

author：afreeliyunfeil@163.com
