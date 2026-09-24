/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/


#ifndef _R_CONVERSIONS_H_
#define _R_CONVERSIONS_H_

int convertBinaryToByte(uint8_t* out, const uint8_t* in, uint32_t length);
int convertByteToBinary(uint8_t* out, uint8_t * in,      uint32_t length);
void convert2compact(OUT uint32_t out[DV], IN const uint8_t in[R_SIZE]);

#endif //_R_CONVERSIONS_H_

