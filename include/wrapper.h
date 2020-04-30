#ifndef UTILS_WRAPPER_H
#define UTILS_WRAPPER_H

#define TEMP_FAILURE_RETRY(exp) ({         \
    typeof (exp) _rc;                      \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })


#define LOG(X) std::cout << "in function: " << X << std::endl

#endif
