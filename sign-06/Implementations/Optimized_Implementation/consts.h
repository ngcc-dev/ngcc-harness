#ifndef CONSTS_H
#define CONSTS_H

#include "params.h"

#define _8XQ          0
#define _8XQINV       8
#define _8XDIV_QINV  16
#define _8XDIV       24
#define _ZETAS_QINV  32
#define _ZETAS      328

/* The C ABI on MacOS exports all symbols with a leading
 * underscore. This means that any symbols we refer to from
 * C files (functions) can't be found, and all symbols we
 * refer to from ASM also can't be found.
 *
 * This define helps us get around this
 */
#if defined(__WIN32__) || defined(__APPLE__)
#define decorate(s) _##s
#define _cdecl(s) decorate(s)
/* Windows/Mac 下的连接逻辑，注意这里通常不需要在参数前加 ## */
#define cdecl(s) _cdecl(COMPASS_SIG_NAMESPACE(s))
#else
/* Linux 下的正确定义：直接将 s 传递给命名空间宏 */
#define cdecl(s) COMPASS_SIG_NAMESPACE(s)
#endif
#ifndef __ASSEMBLER__

#include "align.h"

typedef ALIGNED_INT32(624) qdata_t;

// #define qdata COMPASS_SIG_NAMESPACE(qdata)
extern const qdata_t qdata;

#endif
#endif
