/* Phoenix random number generation
 *
 * Secure random bytes from /dev/urandom or DRNG hardware.
 */

#ifdef SUBMISSION_DRNG

#include <limits.h>
#include <stdlib.h>

#include "drng.h"
#include "randombytes.h"

extern DRNG_ctx drng_algorithm;

/* Fill buffer with random bytes using hardware DRNG */
void randombytes(unsigned char *buf, unsigned long long buf_len)
{
    const unsigned long long max_rq = ULLONG_MAX / 8;

    while (buf_len > 0) {
        unsigned long long rq_size = buf_len < max_rq ? buf_len : max_rq;

        if (get_random_number(&drng_algorithm, buf, rq_size * 8) != 0) {
            abort();
        }

        buf += rq_size;
        buf_len -= rq_size;
    }
}

#else

#include <fcntl.h>
#include <unistd.h>

#include "randombytes.h"

static int urandom_fd = -1;

/* Fill buffer with random bytes from /dev/urandom */
void randombytes(unsigned char *buf, unsigned long long buf_len)
{
    unsigned long long chunk_sz;

    /* Open /dev/urandom on first call */
    if (urandom_fd == -1) {
        for (;;) {
            urandom_fd = open("/dev/urandom", O_RDONLY);
            if (urandom_fd != -1) break;
            sleep(1);
        }
    }

    /* Read in chunks to handle large requests */
    while (buf_len > 0) {
        chunk_sz = (buf_len < 1048576) ? buf_len : 1048576;

        chunk_sz = (unsigned long long)read(urandom_fd, buf, chunk_sz);
        if (chunk_sz < 1) {
            sleep(1);
            continue;
        }

        buf += chunk_sz;
        buf_len -= chunk_sz;
    }
}

#endif