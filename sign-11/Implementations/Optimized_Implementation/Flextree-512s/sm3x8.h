#ifndef SM3X8_H
#define SM3X8_H

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
              unsigned char *digest7);

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
                unsigned char *output7);

#endif
