/*
 * QingLuan Digital Signature Scheme
 * selftest.h - Self-test functions (GM/T 0028 compliance)
 */

#ifndef QINGLUAN_SELFTEST_H
#define QINGLUAN_SELFTEST_H

int crypto_sign_selftest(void);

int crypto_sign_selftest_detailed(int *hash_ok, int *drbg_ok,
                                  int *keygen_ok, int *sign_ok,
                                  int *verify_ok);

#endif /* QINGLUAN_SELFTEST_H */
