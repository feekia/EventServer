/*
 * wrapper.h
 *
 *  Created on: 2018年5月10日
 *      Author: afreeliyunfeil@163.com
 */

#ifndef UTILS_WRAPPER_H
#define UTILS_WRAPPER_H

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(exp) ({         \
    typeof (exp) _rc;                      \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })

#endif

#endif
