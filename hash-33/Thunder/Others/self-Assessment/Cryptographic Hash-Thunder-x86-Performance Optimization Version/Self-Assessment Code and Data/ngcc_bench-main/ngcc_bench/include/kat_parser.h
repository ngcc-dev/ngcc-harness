#ifndef KAT_PARSER_H
#define KAT_PARSER_H

#include <stddef.h>

typedef struct {
    char *name;
    unsigned char *data;
    size_t len;
} ngcc_kat_field_t;

typedef struct {
    unsigned int count;
    ngcc_kat_field_t *fields;
    size_t field_count;
} ngcc_kat_vector_t;

typedef struct {
    ngcc_kat_vector_t *vectors;
    size_t count;
} ngcc_kat_file_t;

int ngcc_kat_parse_file(const char *path, ngcc_kat_file_t *out_kat);
const ngcc_kat_field_t *ngcc_kat_get_field(const ngcc_kat_vector_t *vec, const char *name);
const ngcc_kat_field_t *ngcc_kat_get_field_any(const ngcc_kat_vector_t *vec,
                                               const char *const *names,
                                               size_t name_count);
void ngcc_kat_free(ngcc_kat_file_t *kat);

#endif
