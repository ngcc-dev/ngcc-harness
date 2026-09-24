#ifndef LOOM_LOC_H
#define LOOM_LOC_H

#include "params.h"

/* --- Session state (Algorithms 2–6) --- */
typedef struct loom_kex_state_st {
  int initiator; /* 1 = initiator, 0 = responder */
  int state;     /* 0 = uninitialized, -1 = initialized, $n = sent/received msg($n)*/
  unsigned char spi[LOOM_SPIBYTES];
  unsigned char self_nonce[LOOM_NONCEBYTES];
  unsigned char peer_nonce[LOOM_NONCEBYTES];
  unsigned char self_exch[LOOM_EXCH_LENMAX]; /*pkkem for initiator; ctkem for responder*/
  unsigned int  self_exchlen;
  unsigned char peer_exch[LOOM_EXCH_LENMAX]; /*ctkem for initiator; pkkem for responder*/
  unsigned int  peer_exchlen;
  struct {
    unsigned char* self_id;
    unsigned int   self_id_len;
    unsigned char* self_privkey;
    unsigned char* self_pubkey;
    unsigned char mk_self[LOOM_MACBYTES];
    unsigned char mk_peer[LOOM_MACBYTES];
  } authenticator;
  struct{
    unsigned char skkem[LOOM_KEM_SKBYTES];
    unsigned char keymat[LOOM_KEM_SSBYTES];
    unsigned char ss[LOOM_KEM_SSBYTES];
  } secret;
} LOOM_KEX_CTX;

#endif
