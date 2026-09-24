#include <string.h>

unsigned long long kex_get_passes_num(void) { return 3; }
unsigned long long kex_get_pk_len_bytes(void) { return 1; }
unsigned long long kex_get_sk_len_bytes(void) { return 1; }
unsigned long long kex_get_sta_len_bytes(void) { return 1; }
unsigned long long kex_get_stb_len_bytes(void) { return 1; }
unsigned long long kex_get_ss_len_bytes(void) { return 1; }
unsigned long long kex_get_total_msg_len_bytes(void) { return 1; }

int kex_init_a(unsigned char *pka, unsigned long long *pka_len_bytes,
               unsigned char *ska, unsigned long long *ska_len_bytes,
               unsigned char *sta, unsigned long long *sta_len_bytes) {
    pka[0] = 0x01;
    ska[0] = 0x02;
    sta[0] = 0x03;
    *pka_len_bytes = 1;
    *ska_len_bytes = 1;
    *sta_len_bytes = 1;
    return 0;
}

int kex_init_b(unsigned char *pkb, unsigned long long *pkb_len_bytes,
               unsigned char *skb, unsigned long long *skb_len_bytes,
               unsigned char *stb, unsigned long long *stb_len_bytes) {
    pkb[0] = 0x01;
    skb[0] = 0x02;
    stb[0] = 0x03;
    *pkb_len_bytes = 1;
    *skb_len_bytes = 1;
    *stb_len_bytes = 1;
    return 0;
}

int kex_generate_pass1_msg_a(unsigned char *ska, unsigned long long ska_len_bytes,
                             unsigned char *pkb, unsigned long long pkb_len_bytes,
                             unsigned char *sta, unsigned long long *sta_len_bytes,
                             unsigned char *m1, unsigned long long *m1_len_bytes) {
    (void) ska;
    (void) ska_len_bytes;
    (void) pkb;
    (void) pkb_len_bytes;
    (void) sta;
    *sta_len_bytes = 1;
    m1[0] = 0x11;
    *m1_len_bytes = 1;
    return 0;
}

int kex_generate_pass2_msg_b(unsigned char *skb, unsigned long long skb_len_bytes,
                             unsigned char *pka, unsigned long long pka_len_bytes,
                             unsigned char *m1, unsigned long long m1_len_bytes,
                             unsigned char *stb, unsigned long long *stb_len_bytes,
                             unsigned char *m2, unsigned long long *m2_len_bytes) {
    (void) skb;
    (void) skb_len_bytes;
    (void) pka;
    (void) pka_len_bytes;
    (void) m1;
    (void) m1_len_bytes;
    (void) stb;
    *stb_len_bytes = 1;
    m2[0] = 0x22;
    *m2_len_bytes = 1;
    return 0;
}

int kex_generate_pass3_msg_a(unsigned char *ska, unsigned long long ska_len_bytes,
                             unsigned char *pkb, unsigned long long pkb_len_bytes,
                             unsigned char *m2, unsigned long long m2_len_bytes,
                             unsigned char *sta, unsigned long long *sta_len_bytes,
                             unsigned char *m3, unsigned long long *m3_len_bytes) {
    (void) ska;
    (void) ska_len_bytes;
    (void) pkb;
    (void) pkb_len_bytes;
    (void) m2;
    (void) m2_len_bytes;
    (void) sta;
    *sta_len_bytes = 1;
    m3[0] = 0x33;
    *m3_len_bytes = 1;
    return 0;
}

int kex_derive_ss_a(unsigned char *ska, unsigned long long ska_len_bytes,
                    unsigned char *pkb, unsigned long long pkb_len_bytes,
                    unsigned char *mb, unsigned long long mb_len_bytes,
                    unsigned char *sta, unsigned long long sta_len_bytes,
                    unsigned char *ssa, unsigned long long *ssa_len_bytes) {
    (void) ska;
    (void) ska_len_bytes;
    (void) pkb;
    (void) pkb_len_bytes;
    (void) mb;
    (void) mb_len_bytes;
    (void) sta;
    (void) sta_len_bytes;
    ssa[0] = 0x09;
    *ssa_len_bytes = 1;
    return 0;
}

int kex_derive_ss_b(unsigned char *skb, unsigned long long skb_len_bytes,
                    unsigned char *pka, unsigned long long pka_len_bytes,
                    unsigned char *ma, unsigned long long ma_len_bytes,
                    unsigned char *stb, unsigned long long stb_len_bytes,
                    unsigned char *ssb, unsigned long long *ssb_len_bytes) {
    (void) skb;
    (void) skb_len_bytes;
    (void) pka;
    (void) pka_len_bytes;
    (void) ma;
    (void) ma_len_bytes;
    (void) stb;
    (void) stb_len_bytes;
    ssb[0] = 0x09;
    *ssb_len_bytes = 1;
    return 0;
}
