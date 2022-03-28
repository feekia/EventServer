#pragma once
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>

namespace es {
inline int setNonBlock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    assert(flags >= 0);

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
inline int setBlock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    assert(flags >= 0);
    return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

inline int setAddrReuse(int fd) {
    int flag = true;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof flag);
}

inline int setNoDelay(int fd) {
    int flag = true;
    return setsockopt(fd, SOL_SOCKET, TCP_NODELAY, &flag, sizeof flag);
}

inline int addFlag(int fd, int flag) {
    int ret = fcntl(fd, F_GETFD);
    return fcntl(fd, F_SETFD, ret | flag);
}

} // namespace es
