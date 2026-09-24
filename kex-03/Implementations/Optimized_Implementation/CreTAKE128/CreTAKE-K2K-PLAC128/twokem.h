

int twokem_keygen1(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes);

int twokem_keygen2(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes);

int twokem_keygen2_withseed(
    const unsigned char *seed, unsigned long long seed_len_bytes,
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes);

int twokem_enc(
    unsigned char *pk1, unsigned long long pk1_len_bytes,
    unsigned char *pk2, unsigned long long pk2_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes,
    unsigned char *ct, unsigned long long *ct_len_bytes);

int twokem_dec(
	unsigned char *sk1, unsigned long long sk1_len_bytes,
    unsigned char *sk2, unsigned long long sk2_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes); 

