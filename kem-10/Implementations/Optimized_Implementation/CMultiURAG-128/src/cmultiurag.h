/** 
 * \file cmultiurag.h
 * \brief Functions of the CMULTIURAG_PKE IND-CPA scheme
 */

 #ifndef CMULTIURAG_PKE_H
 #define CMULTIURAG_PKE_H
 
 #include "rbc_vec.h"
 #include "rbc_mat.h"
 
 
 void cmultiurag_pke_keygen(uint8_t* pk, uint8_t* sk);
 void cmultiurag_pke_encrypt(rbc_mat U, rbc_mat V, const rbc_vec m, uint8_t* theta, const uint8_t* pk);
 void cmultiurag_pke_decrypt(rbc_vec m, const rbc_mat U, const rbc_mat V, const uint8_t* sk);
 
 #endif
 
 