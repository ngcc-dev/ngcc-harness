#include "auxfunc.h"
#include "sm3x8.h"

#include <string.h>

int sm3hashx8(int digest_len_bits,
              const unsigned char *msg0,
              const unsigned char *msg1,
              const unsigned char *msg2,
              const unsigned char *msg3,
              const unsigned char *msg4,
              const unsigned char *msg5,
              const unsigned char *msg6,
              const unsigned char *msg7,
              unsigned long long msg_len_bits,
              unsigned char *digest0,
              unsigned char *digest1,
              unsigned char *digest2,
              unsigned char *digest3,
              unsigned char *digest4,
              unsigned char *digest5,
              unsigned char *digest6,
              unsigned char *digest7)
{
    int rc = 0;
    const unsigned char *msg[8] = {msg0, msg1, msg2, msg3,
                                   msg4, msg5, msg6, msg7};
    unsigned char *digest[8] = {digest0, digest1, digest2, digest3,
                                digest4, digest5, digest6, digest7};
    int lane_rc[8] = {0};

    for (unsigned int i = 0; i < 8; i++) {
        unsigned int src = i;

        if (digest_len_bits == 256) {
            for (unsigned int j = 0; j < i; j++) {
                if (lane_rc[j] == 0 && msg[i] == msg[j]) {
                    src = j;
                    break;
                }
            }
        }

        if (src != i) {
            if (digest[i] != digest[src]) {
                memcpy(digest[i], digest[src], 32);
            }
            continue;
        }

        lane_rc[i] = sm3hash(digest_len_bits, msg[i], msg_len_bits, digest[i]);
        rc |= lane_rc[i];
    }

    return rc;
}

int pseudoXOFx8(unsigned long long output_len_bits,
                const unsigned char *msg0,
                const unsigned char *msg1,
                const unsigned char *msg2,
                const unsigned char *msg3,
                const unsigned char *msg4,
                const unsigned char *msg5,
                const unsigned char *msg6,
                const unsigned char *msg7,
                unsigned long long msg_len_bits,
                unsigned char *output0,
                unsigned char *output1,
                unsigned char *output2,
                unsigned char *output3,
                unsigned char *output4,
                unsigned char *output5,
                unsigned char *output6,
                unsigned char *output7)
{
    int rc = 0;
    const unsigned char *msg[8] = {msg0, msg1, msg2, msg3,
                                   msg4, msg5, msg6, msg7};
    unsigned char *output[8] = {output0, output1, output2, output3,
                                output4, output5, output6, output7};
    const unsigned long long output_len_bytes = (output_len_bits + 7u) >> 3;
    int lane_rc[8] = {0};

    for (unsigned int i = 0; i < 8; i++) {
        unsigned int src = i;

        for (unsigned int j = 0; j < i; j++) {
            if (lane_rc[j] == 0 && msg[i] == msg[j]) {
                src = j;
                break;
            }
        }

        if (src != i) {
            if (output[i] != output[src]) {
                memcpy(output[i], output[src], (size_t)output_len_bytes);
            }
            continue;
        }

        lane_rc[i] = pseudoXOF(output_len_bits, msg[i], msg_len_bits, output[i]);
        rc |= lane_rc[i];
    }

    return rc;
}
