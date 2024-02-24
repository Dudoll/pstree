#ifndef __JOEL_ERRNO_H_
#define __JOEL_ERRNO_H_

#include <stdint.h>

#define jerrno_t int32_t

#define OK 0
#define E_NOK (-1)
#define E_NOMEM (-2)
#define E_UNKNOWN_PARA (-127)

#endif // define __JOEL_ERRNO_H_