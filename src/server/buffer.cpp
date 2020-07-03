#include <unistd.h>
#include "buffer.h"

/*
 * 1.threadpools read into input buffer
 * 2.decode && other proccess
 * 3.event_add
 */
ssize_t buffer::readsocket(evutil_socket_t fd)
{
    ssize_t rSize = 0;
    ssize_t rc = 0;

    char buffer[512] = {0};
    do
    {
        rSize = read(fd, (void *)buffer, 512);
        if (rSize > 0)
        {
            rc += rSize;
            append(buffer, rSize);
        }
    } while ((rSize == -1 && errno == EINTR) || (rSize == 512));

    return rc;
}

/*
 * 1.data pending into output buffer
 * 2.thread pools send
 * 3.if not finish ,than event_add 
 */
ssize_t buffer::writesocket(evutil_socket_t fd)
{

    ssize_t wSize = 0;
    ssize_t rc = 0;

    do
    {
        wSize = write(fd, (void *)readbegin(), size());
        if (wSize > 0)
        {
            rc += wSize;
            _read_index += wSize;
        }
    } while ((wSize == -1 && errno == EINTR) || (size() > 0 && wSize > 0));
    if(wSize == EAGAIN && size() > 0){
        // TODO: event_add
    }

    return rc;
}
