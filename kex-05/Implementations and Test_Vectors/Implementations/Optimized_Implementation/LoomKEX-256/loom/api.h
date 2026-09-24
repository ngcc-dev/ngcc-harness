#ifndef LOOM_API_H
#define LOOM_API_H

#include <stddef.h>
#include "params.h"

typedef struct loom_kex_state_st LOOM_KEX_CTX;

/* --- Parameter sizes (wire / keys) --- */
#define CRYPTO_LOOM_PUBLICKEYBYTES   LOOM_PKBYTES
#define CRYPTO_LOOM_SECRETKEYBYTES   LOOM_SKBYTES
#define CRYPTO_LOOM_SSBYTES          LOOM_KEM_SSBYTES

#define CRYPTO_LOOM_MAX_IDBYTES      LOOM_MAX_IDBYTES
#define CRYPTO_LOOM_IDLENBYTES       LOOM_IDLENBYTES
#define CRYPTO_LOOM_NONCEBYTES       LOOM_NONCEBYTES
#define CRYPTO_LOOM_MACBYTES         LOOM_MACBYTES

#define CRYPTO_LOOM_MSG1BYTES        LOOM_MSG1BYTES
#define CRYPTO_LOOM_MSG2BYTES        LOOM_MSG2BYTES
#define CRYPTO_LOOM_MSG3BYTES        LOOM_MSG3BYTES
#define CRYPTO_LOOM_MSG4BYTES        LOOM_MSG4BYTES

#define CRYPTO_LOOM_KEM_PKBYTES      LOOM_KEM_PKBYTES
#define CRYPTO_LOOM_KEM_SKBYTES      LOOM_KEM_SKBYTES
#define CRYPTO_LOOM_KEM_CTBYTES      LOOM_KEM_CTBYTES

#define CRYPTO_LOOM_ALGNAME          LOOM_INSTANCE_NAME

/* --- LOOM-AKE API (Algorithms 1–6) --- */

LOOM_KEX_CTX * create_ctx(void);
int destroy_ctx(LOOM_KEX_CTX *ctx);

#define crypto_loom_initialize_state LOOM_NAMESPACE(initialize_state)
int crypto_loom_initialize_state(LOOM_KEX_CTX *ctx, 
    unsigned char *pk, unsigned char *sk, 
    int initiator, const uint8_t* idself, size_t idlen);

#define crypto_loom_init LOOM_NAMESPACE(init)
int crypto_loom_init(LOOM_KEX_CTX *ctx, unsigned char *msg1);

#define crypto_loom_respond LOOM_NAMESPACE(respond)
int crypto_loom_respond(LOOM_KEX_CTX *ctx,
                        const unsigned char *msg1,
                        unsigned char *msg2);

#define crypto_loom_auth_init LOOM_NAMESPACE(auth_init)
int crypto_loom_auth_init(LOOM_KEX_CTX *ctx,
                          const unsigned char *msg2,
                          unsigned char *msg3);

#define crypto_loom_auth_resp LOOM_NAMESPACE(auth_resp)
int crypto_loom_auth_resp(LOOM_KEX_CTX *ctx,
                          const unsigned char *pk_peer,
                          const unsigned char *id_peer,
                          size_t id_peer_len,
                          const unsigned char *msg3,
                          unsigned char *msg4);

#define crypto_loom_last_init LOOM_NAMESPACE(last_init)
int crypto_loom_last_init(LOOM_KEX_CTX *ctx,
                        const unsigned char *pk_peer,
                        const unsigned char *id_peer,
                        size_t id_peer_len,
                        const unsigned char *msg4);

#define crypto_loom_finalize LOOM_NAMESPACE(finalize)
int crypto_loom_finalize(const LOOM_KEX_CTX *ctx,
                       unsigned char *ss, size_t *ss_len);

#endif
