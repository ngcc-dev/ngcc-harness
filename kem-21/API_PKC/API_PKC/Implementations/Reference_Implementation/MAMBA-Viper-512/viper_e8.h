#ifndef VIPER_E8_H
#define VIPER_E8_H

#include <stdint.h>
#include "viper.h"

void viper_e8_encode(vpoly out, const unsigned char m[VIPER_MSGBYTES]);
void viper_e8_decode(unsigned char m[VIPER_MSGBYTES], const vpoly in);
uint16_t viper_e8_label_to_coeff(unsigned label, unsigned coord);
unsigned viper_e8_coeffs_to_label(const uint16_t coeffs[8]);
const char *viper_e8_labeling_name(void);

#endif
