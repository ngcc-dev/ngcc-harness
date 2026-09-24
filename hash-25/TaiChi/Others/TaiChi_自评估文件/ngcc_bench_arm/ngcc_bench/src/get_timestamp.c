#include <time.h>
#include <stdio.h>

#define DATE_STR_LEN    32

char *get_timestamp(const char *date_format) {
    static char date_str[DATE_STR_LEN];
    time_t now = time(NULL);
    struct tm tm_info;

    if(now == (time_t)-1) {
        fprintf(stderr, "Failure to obtain the current time.\n");
        snprintf(date_str, DATE_STR_LEN, "unknown_time");
        return date_str;
    }

    if(gmtime_r(&now, &tm_info) == NULL) {
        fprintf(stderr, "Failure to convert the current time.\n");
        snprintf(date_str, DATE_STR_LEN, "unknown_time");
        return date_str;
    }

    if(strftime(date_str, DATE_STR_LEN, date_format, &tm_info) == 0) {
        fprintf(stderr, "Failure to format the date string.\n");
        snprintf(date_str, DATE_STR_LEN, "unknown_time");
    }

    return date_str;
}