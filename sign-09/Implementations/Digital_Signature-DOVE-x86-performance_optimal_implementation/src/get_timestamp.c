#include <time.h>
#include <stdio.h>

#define DATE_STR_LEN    32

char *get_timestamp(const char * date_format) {
    static char date_str[DATE_STR_LEN];
    time_t now = time(NULL);
    if (now == (time_t)-1) {
        fprintf(stderr, "Failure to obtain the current time.\n");
    }

    struct tm *tm_info = localtime(&now);
    if (tm_info == NULL) {
        fprintf(stderr, "Failure to convert the current time to local time.\n");
    }
    size_t ret = strftime(date_str, DATE_STR_LEN, date_format, tm_info);
    if (ret == 0) {
        fprintf(stderr, "Failure to format the date string.\n");
    }
    return date_str;
}