#include "tcp_server.h"
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

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    r         = setAddrReuse(listenFd_);
    assert(r == 0);
    r = addFlag(listenFd_, FD_CLOEXEC);
    assert(r == 0);
    r = ::bind(listenFd_, (struct sockaddr *)&addr_, sizeof(struct sockaddr));
    if (r) {
        ::close(listenFd_);
        spdlog::error("bind to {} failed {} {}", "address", errno, strerror(errno));
        listenFd_ = -1;
        return errno;
    }
    r = listen(listenFd_, 60);
    assert(r == 0);
    spdlog::info("fd {} listening at {}", listenFd_, "address");

    acceptHandler_ = new AcceptHandler(listenFd_);
    acceptHandler_->onAccept([this](int cfd) {
        TcpConnectionPtr tcp(new TcpConnection);
        IoOps *          ops = getOps();
        tcp->attachIoOps(ops, cfd);
        newConnectionCb_(tcp);
    });
    r = acceptPlexer_->addAccept(acceptHandler_);
    assert(r == 0);

    return 0;
}

TcpServerPtr TcpServer::startServer(EventWorkGroup *b, EventWorkGroup *w, const std::string &host, unsigned short port,
                                    bool reusePort) {
    TcpServerPtr p(new TcpServer(b, w));
    int          r = p->bind(host, port, reusePort);
    if (r) {
        spdlog::error("bind to {}:{} failed {} {}", host.c_str(), port, errno, strerror(errno));
    }
    return r == 0 ? p : NULL;
}
} // namespace es