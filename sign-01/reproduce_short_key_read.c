/* Run 'sign' and 'verify' separately under AddressSanitizer. */
#include <string.h>

#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include "params.h"

DRNG_ctx drng_algorithm;

int main(int argc, char **argv)
{
    unsigned char key[1] = {0};
    unsigned char message[1] = {0};
    unsigned char signature[8192] = {0};
    unsigned long long signature_length = 0;
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "sign") == 0)
        return sig_sign(key, 0, message, 0, signature, &signature_length);
    if (strcmp(argv[1], "verify") == 0)
        return sig_verify(key, 0, signature, SIG_MIN_SIZE_PACKED, message, 0);
    return 2;
}
