#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include "sysrng.h"

#include <stdint.h>

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>
#include <bcrypt.h>

int
yuanyang_sysrng(void *dst, size_t len)
{
	unsigned char *buf;

	buf = (unsigned char *)dst;
	while (len > 0) {
		ULONG chunk;

		chunk = len > 0xFFFFFFFFu ? 0xFFFFFFFFu : (ULONG)len;
		if (BCryptGenRandom(NULL, buf, chunk,
			BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
		{
			return -1;
		}
		buf += chunk;
		len -= chunk;
	}
	return 0;
}

#elif defined(__linux__)

#include <errno.h>
#include <fcntl.h>
#include <sys/random.h>
#include <unistd.h>

static int
read_urandom(unsigned char *buf, size_t len)
{
	int fd;

	fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
	if (fd < 0) {
		return -1;
	}
	while (len > 0) {
		ssize_t rlen;

		rlen = read(fd, buf, len);
		if (rlen < 0) {
			if (errno == EINTR) {
				continue;
			}
			close(fd);
			return -1;
		}
		if (rlen == 0) {
			close(fd);
			return -1;
		}
		buf += (size_t)rlen;
		len -= (size_t)rlen;
	}
	close(fd);
	return 0;
}

int
yuanyang_sysrng(void *dst, size_t len)
{
	unsigned char *buf;

	buf = (unsigned char *)dst;
	while (len > 0) {
		ssize_t rlen;

		rlen = getrandom(buf, len, 0);
		if (rlen < 0) {
			if (errno == EINTR) {
				continue;
			}
			if (errno == ENOSYS) {
				return read_urandom(buf, len);
			}
			return -1;
		}
		if (rlen == 0) {
			return -1;
		}
		buf += (size_t)rlen;
		len -= (size_t)rlen;
	}
	return 0;
}

#elif defined(__unix__) || defined(__APPLE__)

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int
yuanyang_sysrng(void *dst, size_t len)
{
	unsigned char *buf;
	int fd;

	buf = (unsigned char *)dst;
	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0) {
		return -1;
	}
	while (len > 0) {
		ssize_t rlen;

		rlen = read(fd, buf, len);
		if (rlen < 0) {
			if (errno == EINTR) {
				continue;
			}
			close(fd);
			return -1;
		}
		if (rlen == 0) {
			close(fd);
			return -1;
		}
		buf += (size_t)rlen;
		len -= (size_t)rlen;
	}
	close(fd);
	return 0;
}

#else

int
yuanyang_sysrng(void *dst, size_t len)
{
	(void)dst;
	(void)len;
	return -1;
}

#endif
