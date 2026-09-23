#ifndef LOONG_CT_UTIL_H
#define LOONG_CT_UTIL_H

int loong_ct_equal(const unsigned char *a, const unsigned char *b,
                   unsigned long long len);
void loong_ct_select(unsigned char *out, const unsigned char *if_one,
                     const unsigned char *if_zero, unsigned long long len,
                     unsigned char mask);
void loong_secure_bzero(void *ptr, unsigned long long len);

#endif
