
## 代码特性
1. handler 用于异步消息的处理
2. threadpool实现
3. libevent 多线程io
4. timer 调度实现

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

    sudo apt install libevent-dev

    sudo apt install cmake

    sudo apt install clang

    sudo apt-get install libgtest-dev
  
## 编译
    cd build

    cmake ..

    make

## clean
    cd build
    make clean

## 执行（server 和 client 在同一台主机）
    ./build/bin/server 9950
    ./build/bin/client 127.0.0.1 9950

目前在同一台主机上测试，由于端口分配的限制，只能测试5万个链接，5个client线程分别轮询1万个socket 进行发送数据，服务端会原样返回client发过去的数据。

说明：
    server负责监听链接，以及在收到client数据时，将数据重新发回client
    每个client 创建一万个链接，并用2个线程轮流给1万个socket发送数据，1个线程负责收数据

测试结果：
    4core CPU, 8GB 内存
    峰值并发连接2.9K/s; 
    5万连接，32bytes 数据收发;
    峰值QPS:25w 稳定QPS:15w

## 长连接场景线程池设计(M:N模型)

1. 以epoll LT 方式开辟一个线程组(M线程)监听连接(处理10k/s级的瞬时连接峰值) ,将accept 和 fd 入列到其他线程组的逻辑分离，提升连接效率
2. 开辟线程组(N个线程)处理连接事件，同时为每一个线程绑定一个pipe，以便需要写数据时能唤醒loop
3. read 、write、timeout 在统一线程中完成

author：afreeliyunfeil@163.com
