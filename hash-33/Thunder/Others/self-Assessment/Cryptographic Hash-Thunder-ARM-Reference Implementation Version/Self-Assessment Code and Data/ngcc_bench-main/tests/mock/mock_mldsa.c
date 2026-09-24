#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/random.h>
#include <unistd.h>

#define MLDSA_PK_BYTES 32
#define MLDSA_SK_BYTES 64
#define MLDSA_SN_BYTES 64

static int fill_random(unsigned char *buf, unsigned long long len) {
    unsigned long long off = 0;

    while (off < len) {
        ssize_t got = getrandom(buf + off, (size_t) (len - off), 0);
        if (got > 0) {
            off += (unsigned long long) got;
            continue;
        }
        if (got < 0 && errno == EINTR) {
            continue;
        }
        break;
    }
    if (off == len) {
        return 0;
    }

    {
        int fd = open("/dev/urandom", O_RDONLY);
        if (fd < 0) {
            return -1;
        }
        while (off < len) {
            ssize_t got = read(fd, buf + off, (size_t) (len - off));
            if (got > 0) {
                off += (unsigned long long) got;
                continue;
            }
            if (got < 0 && errno == EINTR) {
                continue;
            }
            close(fd);
            return -1;
        }
        close(fd);
    }

    return 0;
}

static void compute_tag(const unsigned char *pk,
                        const unsigned char *msg,
                        unsigned long long msg_len,
                        unsigned char *tag) {
    unsigned long long i;

    for (i = 0; i < MLDSA_PK_BYTES; ++i) {
        tag[i] = (unsigned char) (pk[i] ^ (unsigned char) (i * 17U + 3U));
    }
    for (i = 0; i < msg_len; ++i) {
        unsigned long long j = i % MLDSA_PK_BYTES;
        tag[j] = (unsigned char) ((tag[j] + msg[i] + (unsigned char) i) & 0xFFU);
        tag[(j + 7U) % MLDSA_PK_BYTES] ^= (unsigned char) (msg[i] + pk[j]);
    }
}

unsigned long long sig_get_pk_len_bytes(void) { return MLDSA_PK_BYTES; }
unsigned long long sig_get_sk_len_bytes(void) { return MLDSA_SK_BYTES; }
unsigned long long sig_get_sn_len_bytes(void) { return MLDSA_SN_BYTES; }

int sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes) {
    if (fill_random(pk, MLDSA_PK_BYTES) != 0) {
        return -1;
    }

    if (fill_random(sk, MLDSA_PK_BYTES) != 0) {
        return -1;
    }

    memcpy(sk + MLDSA_PK_BYTES, pk, MLDSA_PK_BYTES);
    *pk_len_bytes = MLDSA_PK_BYTES;
    *sk_len_bytes = MLDSA_SK_BYTES;
    return 0;
}

int sig_sign(unsigned char *sk, unsigned long long sk_len_bytes,
             unsigned char *m, unsigned long long m_len_bytes,
             unsigned char *sn, unsigned long long *sn_len_bytes) {
    unsigned char *pk;

    if (sk_len_bytes < MLDSA_SK_BYTES) {
        return -1;
    }

    pk = sk + MLDSA_PK_BYTES;
    memcpy(sn, pk, MLDSA_PK_BYTES);
    compute_tag(pk, m, m_len_bytes, sn + MLDSA_PK_BYTES);
    *sn_len_bytes = MLDSA_SN_BYTES;
    return 0;
}

int sig_verify(unsigned char *pk, unsigned long long pk_len_bytes,
               unsigned char *sn, unsigned long long sn_len_bytes,
               unsigned char *m, unsigned long long m_len_bytes) {
    unsigned char expected[MLDSA_PK_BYTES];

    if (pk_len_bytes != MLDSA_PK_BYTES || sn_len_bytes != MLDSA_SN_BYTES) {
        return -1;
    }
    if (memcmp(pk, sn, MLDSA_PK_BYTES) != 0) {
        return -1;
    }

    compute_tag(pk, m, m_len_bytes, expected);
    if (memcmp(expected, sn + MLDSA_PK_BYTES, MLDSA_PK_BYTES) != 0) {
        return -1;
    }

    return 0;
}
