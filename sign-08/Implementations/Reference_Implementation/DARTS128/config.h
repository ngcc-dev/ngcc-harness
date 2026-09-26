#ifndef CONFIG_H
#define CONFIG_H

/*************************************************
 *This file is to be included in the implementation files to set the parameters for the signature scheme.
 * We provides three different parameter sets: 128, 256, and 512. The default is 128, which is the most secure and efficient one. 
 * You can change the parameter set by defining the DARTS_MODE macro before including this file. 
 * For example, to use the 128-bit parameter set, you can write: # define DARTS_MODE 128
 **************************************************/

#define DARTS_MODE 128

#define CRYPTO_ALGNAME "DARTS128"
#define DARTS_NAMESPACE(s) cryptolab_DARTS_128_##s
#endif