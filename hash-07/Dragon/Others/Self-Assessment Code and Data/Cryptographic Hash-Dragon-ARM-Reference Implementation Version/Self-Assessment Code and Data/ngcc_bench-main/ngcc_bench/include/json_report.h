#ifndef JSON_REPORT_H
#define JSON_REPORT_H

#include "cli_types.h"

enum { LANG_ZH = 0, LANG_EN = 1 };

/* Write a single JSON report in the specified language to the given path. */
int write_json_report(const cli_options_t *opts,
                      const run_report_t *report,
                      int overall_failed,
                      int lang,
                      const char *out_path);

/* Write both .zh and .en JSON reports based on opts->json_out_path. */
int write_json_reports(const cli_options_t *opts,
                       const run_report_t *report,
                       int overall_failed);

#endif
