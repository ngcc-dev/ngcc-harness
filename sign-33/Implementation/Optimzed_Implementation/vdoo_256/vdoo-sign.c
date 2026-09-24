#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "vdoo_config.h"
#include "vdoo_keypair.h"
#include "api.h"
#include "utils.h"
#include "vdoo_config.h"


int main( int argc , char ** argv )
{
    printf( "%s\n", CRYPTO_ALGNAME );

    printf("sk size: %lu\n", CRYPTO_SECRETKEYBYTES );
    printf("pk size: %lu\n",  CRYPTO_PUBLICKEYBYTES );
    printf("hash size: %d\n", HASH_LEN );
    printf("signature size: %d\n\n", CRYPTO_BYTES );

    if( !(3 == argc) ) {
        printf("Usage:\n\n\tvdoo-sign sk_file_name file_to_be_signed\n\n");
        return -1;
    }

    uint8_t *_sk = (uint8_t*)malloc( CRYPTO_SECRETKEYBYTES );

    FILE * fp;
    int r = 0;

    fp = fopen( argv[1] , "r");
    if( NULL == fp ) {
        printf("fail to open secret key file.\n");
        return -1;
    }
    r = byte_fget( fp ,  _sk , CRYPTO_SECRETKEYBYTES );
    fclose( fp );
    if( CRYPTO_SECRETKEYBYTES != r ) {
        printf("fail to load key file.\n");
        return -1;
    }

    unsigned char * msg = NULL;
    unsigned long long mlen = 0;
    r = byte_read_file( &msg , &mlen , argv[2] );
    if( 0 != r ) {
        printf("fail to read message file.\n");
        return -1;
    }

    unsigned char * signature = malloc( mlen + CRYPTO_BYTES );
    if( NULL == signature ) {
        printf("alloc memory for signature buffer fail.\n");
        return -1;
    }

    unsigned long long smlen = 0;
    r = crypto_sign( signature, &smlen, msg , mlen , _sk );
    if( 0 != r ) {
        printf("sign() fail.\n");
        return -1;
    }

    byte_fdump( stdout , CRYPTO_ALGNAME " signature"  , signature + mlen , CRYPTO_BYTES );
    printf("\n");

    free( msg );
    free( signature );
    free( _sk );

    return 0;
}

