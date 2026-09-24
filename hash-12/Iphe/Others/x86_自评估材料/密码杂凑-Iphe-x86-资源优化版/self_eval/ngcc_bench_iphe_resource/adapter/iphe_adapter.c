#include <stdio.h>
#include "registry.h"

int CryptHash_iphe512_res(int, const unsigned char *, unsigned long long, unsigned char *);
int CryptHash_iphe768_res(int, const unsigned char *, unsigned long long, unsigned char *);
int CryptHash_iphe1024_res(int, const unsigned char *, unsigned long long, unsigned char *);

static algorithm_t iphe_algs[] = {
    { "iphe-512-res", "Iphe-512-Resource", "API_CryptHash/libiphe_512.a", 512, 184U, CryptHash_iphe512_res },
    { "iphe-768-res", "Iphe-768-Resource", "API_CryptHash/libiphe_768.a", 768, 152U, CryptHash_iphe768_res },
    { "iphe-1024-res", "Iphe-1024-Resource", "API_CryptHash/libiphe_1024.a", 1024, 120U, CryptHash_iphe1024_res }
};

void register_iphe_algorithms(void)
{
    size_t i;
    for (i = 0; i < sizeof(iphe_algs) / sizeof(iphe_algs[0]); i++) {
        if (registry_algorithm(&iphe_algs[i]) != 0)
            fprintf(stderr, "failed to register %s\n", iphe_algs[i].id);
    }
}
