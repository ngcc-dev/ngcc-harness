#ifndef BDD_ASM_H
#define BDD_ASM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ARM assembly implementation of BDD_4
void BDD_4_asm(int16_t *w, const int16_t *t);
void BDD_8_asm(int16_t *w, const int16_t *t);
void BDD_16_asm(int16_t *w, const int16_t *t);
void BDD_asm(int16_t *w, const int16_t *t);

#ifdef __cplusplus
}
#endif

#endif // BDD_ASM_H
