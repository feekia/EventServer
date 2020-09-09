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
        rc = read(fd, (void *)buffer, 512);
        // if (rSize == 0)
        //     cout << "rSize : " << rSize << endl;
        if (rc > 0)
        {
            rSize += rc;
            append(buffer, rc);
        }
    } while ((rc == -1 && errno == EINTR) || rc > 0);
    
    if(errno == EAGAIN && rc == -1){
        return rSize;
    }else if(errno != 0){
        return rSize > 0 ? rSize : -1;
    }
    return rSize;
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

    if (size() <= 0)
    {
        return 0;
    }
    do
    {
        wSize = send(fd, (void *)readbegin(), size(), MSG_NOSIGNAL);
        if (wSize > 0)
        {
            rc += wSize;
            _read_index += wSize;
        }
    } while ((wSize == -1 && errno == EINTR) || (size() > 0 && wSize > 0));
    // if (wSize == EAGAIN && size() > 0)
    // {
    //     // TODO: event_add
    // }

    if (wSize == -1 && errno == EPIPE)
    {
        return -1;
    }

    return rc;
}
