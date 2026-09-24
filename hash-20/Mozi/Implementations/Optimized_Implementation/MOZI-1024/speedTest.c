#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef DIGEST_MAX_BYTES
#define DIGEST_MAX_BYTES 128
#endif

int CryptHash(int digest_len_bits,
              const unsigned char *msg,
              unsigned long long msg_len_bits,
              unsigned char *digest);

static double now_seconds(void)
{
#if defined(_WIN32)
    return (double)clock() / CLOCKS_PER_SEC;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#endif
}

static void speedTestOne(
    int digest_len_bits,
    size_t msg_len_bytes,
    int repeat
)
{
    unsigned char *msg = NULL;
    unsigned char digest[DIGEST_MAX_BYTES];
    unsigned char checksum = 0;

    double start, end;
    double elapsed;
    double total_bytes;
    double speed_MBps;

    int i, j;
    int ret;

    int digest_len_bytes = (digest_len_bits + 7) / 8;

    if (digest_len_bytes > DIGEST_MAX_BYTES) {
        printf("digest buffer too small: need %d bytes\n", digest_len_bytes);
        return;
    }

    msg = (unsigned char *)malloc(msg_len_bytes);
    if (msg == NULL) {
        printf("malloc failed\n");
        return;
    }

    /*
     * 填充测试消息。
     * 不建议全 0，因为有些实现可能对特殊输入有优化。
     */
    for (i = 0; i < (int)msg_len_bytes; i++) {
        msg[i] = (unsigned char)(i & 0xff);
    }

    memset(digest, 0, sizeof(digest));

    start = now_seconds();

    for (i = 0; i < repeat; i++) {
        ret = CryptHash(
            digest_len_bits,
            msg,
            (unsigned long long)msg_len_bytes * 8ULL,
            digest
        );

        if (ret != 0) {
            printf("CryptHash failed, ret = %d\n", ret);
            free(msg);
            return;
        }

        /*
         * 防止编译器在极端情况下优化掉 digest 的使用。
         */
        for (j = 0; j < digest_len_bytes; j++) {
            checksum += digest[j];
        }
    }

    end = now_seconds();

    elapsed = end - start;
    total_bytes = (double)msg_len_bytes * (double)repeat;

    /*
     * 这里使用十进制 MB/s，即 1 MB = 1,000,000 bytes。
     */
    speed_MBps = total_bytes / elapsed / 1000000.0;

    printf("Message length : %zu bytes\n", msg_len_bytes);
    printf("Repeat         : %d\n", repeat);
    printf("Total data     : %.2f MB\n", total_bytes / 1000000.0);
    printf("Elapsed time   : %.6f s\n", elapsed);
    printf("Average speed  : %.2f MB/s\n", speed_MBps);
    printf("Checksum       : %02x\n", checksum);
    printf("\n");

    free(msg);
}

void speedTest(int digest_len_bits)
{
    printf("==== CryptHash Speed Test ====\n");
    printf("Digest length  : %d bits\n\n", digest_len_bits);

    /*
     * 测试 1：1000 字节，重复 1000000 次
     */
    speedTestOne(digest_len_bits, 1000, 1000000);

    /*
     * 测试 2：100000000 字节，重复 10 次
     */
    speedTestOne(digest_len_bits, 100000000, 10);
}

int main()
{
    int digest_len_bits = 1024; // 1024 bits = 128 bytes
    speedTest(digest_len_bits);
    return 0;
}