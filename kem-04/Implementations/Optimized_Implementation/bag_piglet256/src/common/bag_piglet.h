#ifndef BAG_PIGLET_H
#define BAG_PIGLET_H

void bag_piglet_pke_keygen(unsigned char *pk, unsigned char *sk,
                           const unsigned char *seed);
void bag_piglet_pke_encrypt(const unsigned char *message,
                            unsigned char *ciphertext,
                            const unsigned char *pk,
                            const unsigned char *seed);
void bag_piglet_pke_decrypt(unsigned char *message,
                            const unsigned char *ciphertext,
                            const unsigned char *sk);

#endif
