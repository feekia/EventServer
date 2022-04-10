#include "buffer.h"
#include "es_util.h"
#include "event_work_group.h"
#include "logging.h"
#include "tcp_client.h"
#include "tcp_connection.h"
#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

using namespace es;

class TcpClientUtil {

public:
    static int connect(const string &ip = "127.0.0.1", int port = 9950) {
        int                fd;
        struct sockaddr_in srv;
        memset(&srv, 0, sizeof(srv));
        srv.sin_family = AF_INET;
        srv.sin_port   = htons(port);
        if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            spdlog::error("create socket error: {}(errno:{})", strerror(errno), errno);
            return -1;
        }

        if (inet_pton(AF_INET, ip.c_str(), &srv.sin_addr) <= 0) {
            spdlog::error("inet_pton error for {}", ip);
            return -1;
        }

        if (::connect(fd, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
            spdlog::error("connect error: %s(errno: %d)\n", strerror(errno), errno);
            return -1;
        }
        return fd;
    }
};

int main(int argc, char **args) {
    EventWorkGroup worker(2);

    unordered_map<int64_t, TcpConnectionPtr> connectionPools;
    TcpClientPtr tcpclient = TcpClient::startClient(&worker, [&](const TcpConnectionPtr &con) {
        connectionPools[con->fd()] = con;
        con->onState([&](const TcpConnectionPtr &con) {
            if (con->state() == TcpConnection::TcpState::kClosed) {
                // clear connectionPools;
            }
            if (con->state() == TcpConnection::TcpState::kConnected) {
                // clear connectionPools;
                Buffer buf;
                buf.append("asdfasdf");
                con->send(buf);
            }
        });
        con->onFreeTimeOut(40 * 1000, [&](const TcpConnectionPtr &con) {
            spdlog::info("Client timeout {}, close !", con->fd());
            con->close();
        });
        con->onRead([&](const TcpConnectionPtr &con) {
            spdlog::info("Client onRead: {}", con->fd());
            con->send(con->getReadBuffer());
        });
    });

    Signal::signal(SIGINT, [&] { worker.exit(); });

    for (int i = 0; i < 5; i++) {
        int fd = TcpClientUtil::connect();
        tcpclient->attach(fd);
    }
    worker.sync();
}