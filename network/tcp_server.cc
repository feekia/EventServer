#include "tcp_server.h"
#include "logging.h"
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;
namespace es {

int TcpServer::bind(const std::string &host, unsigned short port, bool reusePort) {
    assert(host.size() > 0);
    struct sockaddr_in addr_;
    int                r = -1;
    char               buf[1024];
    struct hostent     hent;
    struct hostent *   he     = NULL;
    int                herrno = 0;
    memset(&addr_, 0, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_port   = htons(port);

    memset(&hent, 0, sizeof hent);
    r = gethostbyname_r(host.c_str(), &hent, buf, sizeof buf, &he, &herrno);
    if (r == 0 && he && he->h_addrtype == AF_INET) {
        addr_.sin_addr = *reinterpret_cast<struct in_addr *>(he->h_addr);
    } else {
        spdlog::error("gethostbyname_r failed {} {}", errno, strerror(errno));
        return errno;
    }

    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    r          = setAddrReuse(listen_fd_);
    assert(r == 0);
    r = addFlag(listen_fd_, FD_CLOEXEC);
    assert(r == 0);
    r = ::bind(listen_fd_, (struct sockaddr *)&addr_, sizeof(struct sockaddr));
    if (r) {
        ::close(listen_fd_);
        spdlog::error("bind to {} failed {} {}", "address", errno, strerror(errno));
        listen_fd_ = -1;
        return errno;
    }

    accept_handler_ = new AcceptHandler(listen_fd_);
    accept_handler_->onAccept([this](int cfd) {
        TcpConnectionPtr tcp(new TcpConnection);
        IoOps *          ops = getOps();
        tcp->setFd(cfd);
        new_connection_cb_(tcp);
        ops->sendAndWakeup([=]() { tcp->attachIoOps(ops); });
    });
    r = accept_plexer_->addAccept(accept_handler_);
    assert(r == 0);

    r = listen(listen_fd_, 60);
    assert(r == 0);
    spdlog::info("fd {} listening", listen_fd_);
    return 0;
}

} // namespace es