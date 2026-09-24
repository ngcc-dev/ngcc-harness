#include <stdio.h>
#include <string.h>
#include "bench_local.h"

int static_mem_parse(const char *lib_name, struct mem_profile *mem_prof)
{
    char cmd[512];
    char line[512];
    FILE *fp;
    unsigned long long text = 0, data = 0, bss = 0;
    int parsed = 0;

    mem_prof->text_size = 0;
    mem_prof->data_size = 0;
    mem_prof->bss_size = 0;

    if (lib_name == NULL || lib_name[0] == '\0')
        return 1;

    snprintf(cmd, sizeof(cmd), "size \"%s\" 2>/dev/null", lib_name);
    fp = popen(cmd, "r");
    if (!fp)
        return 1;

    while (fgets(line, sizeof(line), fp)) {
        unsigned long long t, d, b;
        if (sscanf(line, "%llu %llu %llu", &t, &d, &b) == 3) {
            text += t;
            data += d;
            bss += b;
            parsed++;
        }
    }
    pclose(fp);

    mem_prof->text_size = (size_t)text;
    mem_prof->data_size = (size_t)data;
    mem_prof->bss_size = (size_t)bss;
    return parsed > 0 ? 0 : 1;
}
