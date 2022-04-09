

## 工程简介
    此工程致力于打造一个MQTT C++网络库，目前正在设计中，欢迎有经验的朋友指点、讨论。

  
## 编译
    sudo apt install cmake
    sudo apt install clang
    cd build
    cmake ..
    make

## clean
    cd build
    make clean

## 长连接场景线程池设计(M:N模型)

1. 以epoll LT 方式开辟一个线程组(M线程)监听连接(处理10k/s级的瞬时连接峰值) ,将accept 和 fd 入列到其他线程组的逻辑分离，提升连接效率
2. 开辟线程组(N个线程)处理连接事件，同时为每一个线程绑定一个pipe，以便需要写数据时能唤醒loop
3. read 、write、timeout 在统一线程中完成

