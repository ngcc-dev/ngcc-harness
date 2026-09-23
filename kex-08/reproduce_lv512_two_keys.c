/* Build and run command is in kex-08/report.md. */
#include <stdio.h>
#include <string.h>

#include "bundleprotocols_internal.h"

int main(void)
{
    secretkey_t first, second, third;

    make_SecretKey(first, NULL);
    make_SecretKey(second, NULL);
    make_SecretKey(third, NULL);

    if (memcmp(first, third, sizeof first) != 0 ||
        memcmp(first, second, sizeof first) == 0) {
        puts("NOT-CONFIRMED kex-08-2");
        return 1;
    }

    puts("ATTACK kex-08-2 NIIKE-lv512 CONFIRMED: two-key cycle");
    return 0;
}
