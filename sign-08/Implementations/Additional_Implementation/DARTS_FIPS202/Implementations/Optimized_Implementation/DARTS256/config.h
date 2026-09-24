#ifndef CONFIG_H
#define CONFIG_H

/*************************************************
 *This file is to be included in the implementation files to set the parameters for the signature scheme.
 * We provides three different parameter sets: 128, 256, and 512. The default is 128, which is the most secure and efficient one. 
 * You can change the parameter set by defining the SIGN_MODE macro before including this file. 
 * For example, to use the 128-bit parameter set, you can write: # define SIGN_MODE 128
 **************************************************/

#define SIGN_MODE 256
    
#define CRYPTO_ALGNAME "SIGN256"
#define SIGN_NAMESPACETOP sign256
#define DARTS_NAMESPACE(s) cryptolab_sign256_##s
#endif