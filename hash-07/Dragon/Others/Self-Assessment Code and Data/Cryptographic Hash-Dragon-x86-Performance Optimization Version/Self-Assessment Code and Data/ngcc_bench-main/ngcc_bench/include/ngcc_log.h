#ifndef NGCC_LOG_H
#define NGCC_LOG_H

#if defined(__GNUC__) || defined(__clang__)
#define NGCC_LOG_PRINTF(fmt_index, first_arg) __attribute__((format(printf, fmt_index, first_arg)))
#else
#define NGCC_LOG_PRINTF(fmt_index, first_arg)
#endif

void ngcc_log_open(void);
void ngcc_log_close(void);
void ngcc_log_error(const char *fmt, ...) NGCC_LOG_PRINTF(1, 2);
void ngcc_log_warning(const char *fmt, ...) NGCC_LOG_PRINTF(1, 2);
void ngcc_log_info(const char *fmt, ...) NGCC_LOG_PRINTF(1, 2);

#undef NGCC_LOG_PRINTF

#endif
