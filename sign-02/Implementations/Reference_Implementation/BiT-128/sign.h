/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#ifndef SIGN_H
#define SIGN_H

int bit_sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
                   unsigned char *sk, unsigned long long *sk_len_bytes);

int bit_sig_sign(unsigned char *sk, unsigned long long sk_len_bytes,
                 const unsigned char *m, unsigned long long mlen,
                 unsigned char *sig, unsigned long long *sig_len_bytes);

int bit_sig_verify(const unsigned char *sig, unsigned long long siglen,
                   const unsigned char *m, unsigned long long mlen,
                   const unsigned char *pk, unsigned long long pk_len_bytes);

#endif
