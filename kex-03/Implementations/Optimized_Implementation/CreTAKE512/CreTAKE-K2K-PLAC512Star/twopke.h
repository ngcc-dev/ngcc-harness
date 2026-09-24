
void twopke_enc(const unsigned char *pk1, const unsigned char *pk2, const unsigned char *m, const unsigned char *seed1, const unsigned char *seed2, unsigned char *c);

void twopke_dec(const unsigned char *sk1, const unsigned char *sk2, const unsigned char *c, unsigned char *m);