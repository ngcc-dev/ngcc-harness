#include "ngcc_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>

#define NGCC_LOG_PREFIX "【NGCC】 "

static int g_ngcc_log_opened = 0;

static void ngcc_log_ensure_open(void) {
    if (!g_ngcc_log_opened) {
        openlog("ngcc_bench", LOG_PID | LOG_NDELAY, LOG_USER);
        g_ngcc_log_opened = 1;
    }
}

static void ngcc_log_vwrite(int priority, const char *fmt, va_list ap) {
    char msg[2048];

    if (fmt == NULL) {
        return;
    }

    ngcc_log_ensure_open();
    vsnprintf(msg, sizeof(msg), fmt, ap);
    syslog(priority, "%s%s", NGCC_LOG_PREFIX, msg);
}

void ngcc_log_open(void) {
    ngcc_log_ensure_open();
}

void ngcc_log_close(void) {
    if (g_ngcc_log_opened) {
        closelog();
        g_ngcc_log_opened = 0;
    }
}

void ngcc_log_error(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    ngcc_log_vwrite(LOG_ERR, fmt, ap);
    va_end(ap);
}

void ngcc_log_warning(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    ngcc_log_vwrite(LOG_WARNING, fmt, ap);
    va_end(ap);
}

void ngcc_log_info(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    ngcc_log_vwrite(LOG_INFO, fmt, ap);
    va_end(ap);
}
