#include "kat_parser.h"
#include "bench_core.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NGCC_KAT_MAX_FIELD_BYTES ((size_t) NGCC_MAX_BUFFER_LEN)
#define NGCC_KAT_MAX_VALUE_TEXT_LEN ((size_t) (2ULL * NGCC_MAX_BUFFER_LEN))
#define NGCC_KAT_MAX_LINE_LEN (NGCC_KAT_MAX_VALUE_TEXT_LEN + 4096U)
#define NGCC_KAT_MAX_VECTORS 1000U

typedef struct {
    unsigned int count;
    int has_count;
    ngcc_kat_field_t *fields;
    size_t field_count;
} kat_builder_t;

static char *trim(char *s) {
    char *end;

    while (*s != '\0' && isspace((unsigned char) *s)) {
        s++;
    }
    if (*s == '\0') {
        return s;
    }

    end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char) *end)) {
        *end = '\0';
        end--;
    }
    return s;
}

static int key_equals_ci(const char *a, const char *b) {
    while (*a != '\0' && *b != '\0') {
        if (tolower((unsigned char) *a) != tolower((unsigned char) *b)) {
            return 0;
        }
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0');
}

static int key_has_space(const char *name) {
    while (*name != '\0') {
        if (isspace((unsigned char) *name)) {
            return 1;
        }
        name++;
    }
    return 0;
}

static int key_is_len_field(const char *name) {
    if (name == NULL || *name == '\0') {
        return 0;
    }
    if (strstr(name, "LEN") != NULL || strstr(name, "Len") != NULL || strstr(name, "len") != NULL) {
        return 1;
    }
    return 0;
}

static int key_is_metadata(const char *name) {
    if (name == NULL || *name == '\0') {
        return 0;
    }
    if (key_equals_ci(name, "COUNT") || key_equals_ci(name, "SEED") || key_equals_ci(name, "COMMENT") ||
        key_equals_ci(name, "ALGORITHM") || key_equals_ci(name, "ALG") || key_equals_ci(name, "ID")) {
        return 1;
    }
    return 0;
}

static int read_full_line(FILE *fp, char **out_line, size_t *out_len) {
    size_t cap = 4096;
    size_t len = 0;
    const size_t max_cap = NGCC_KAT_MAX_LINE_LEN + 1U;
    char *buf;
    char *next;

    if (fp == NULL || out_line == NULL || out_len == NULL) {
        return -1;
    }
    *out_line = NULL;
    *out_len = 0;

    buf = (char *) malloc(cap);
    if (buf == NULL) {
        return -1;
    }

    for (;;) {
        size_t room = cap - len;
        if (room < 2U) {
            size_t next_cap;

            if (cap >= max_cap) {
                free(buf);
                return -1;
            }
            next_cap = cap * 2U;
            if (next_cap < cap || next_cap > max_cap) {
                next_cap = max_cap;
            }
            next = (char *) realloc(buf, next_cap);
            if (next == NULL) {
                free(buf);
                return -1;
            }
            buf = next;
            cap = next_cap;
            room = cap - len;
        }
        if (fgets(buf + len, (int) room, fp) == NULL) {
            if (len == 0U) {
                free(buf);
                return 0;
            }
            break;
        }
        len += strlen(buf + len);
        if (len > NGCC_KAT_MAX_LINE_LEN) {
            free(buf);
            return -1;
        }
        if (len > 0U && buf[len - 1U] == '\n') {
            break;
        }
    }

    *out_line = buf;
    *out_len = len;
    return 1;
}

static char *dup_string(const char *s) {
    size_t n;
    char *p;

    if (s == NULL) {
        return NULL;
    }

    n = strlen(s);
    p = (char *) malloc(n + 1U);
    if (p == NULL) {
        return NULL;
    }
    memcpy(p, s, n + 1U);
    return p;
}

static int hex_value(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

static int parse_hex_bytes(const char *hex, unsigned char **out_buf, size_t *out_len) {
    size_t n = 0;
    size_t i;
    size_t in_len;
    char *normalized = NULL;
    unsigned char *buf;

    if (hex == NULL || out_buf == NULL || out_len == NULL) {
        return -1;
    }

    in_len = strlen(hex);
    if (in_len > NGCC_KAT_MAX_VALUE_TEXT_LEN) {
        return -1;
    }
    normalized = (char *) malloc(in_len + 1U);
    if (normalized == NULL) {
        return -1;
    }

    for (i = 0; i < in_len; ++i) {
        char c = hex[i];
        if (isspace((unsigned char) c) || c == ':' || c == '_' || c == '.') {
            continue;
        }
        if (c == '0' && i + 1U < in_len && (hex[i + 1U] == 'x' || hex[i + 1U] == 'X')) {
            i++;
            continue;
        }
        if (!isxdigit((unsigned char) c)) {
            free(normalized);
            return -1;
        }
        normalized[n++] = c;
    }
    normalized[n] = '\0';

    if ((n % 2U) != 0U) {
        free(normalized);
        return -1;
    }
    if (n == 0U) {
        *out_buf = NULL;
        *out_len = 0;
        free(normalized);
        return 0;
    }
    if ((n / 2U) > NGCC_KAT_MAX_FIELD_BYTES) {
        free(normalized);
        return -1;
    }

    buf = (unsigned char *) malloc(n / 2U);
    if (buf == NULL) {
        free(normalized);
        return -1;
    }

    for (i = 0; i < n; i += 2U) {
        int hi = hex_value(normalized[i]);
        int lo = hex_value(normalized[i + 1U]);
        if (hi < 0 || lo < 0) {
            free(buf);
            free(normalized);
            return -1;
        }
        buf[i / 2U] = (unsigned char) ((hi << 4) | lo);
    }

    *out_buf = buf;
    *out_len = n / 2U;
    free(normalized);
    return 0;
}

static void builder_clear(kat_builder_t *builder) {
    size_t i;

    for (i = 0; i < builder->field_count; ++i) {
        free(builder->fields[i].name);
        free(builder->fields[i].data);
    }
    free(builder->fields);
    memset(builder, 0, sizeof(*builder));
}

static int builder_has_payload(const kat_builder_t *builder) {
    return builder->field_count > 0;
}

static int builder_set_field(kat_builder_t *builder,
                             const char *name,
                             const unsigned char *data,
                             size_t len) {
    size_t i;
    char *name_dup;
    unsigned char *data_dup;

    if (name == NULL || *name == '\0' || (data == NULL && len > 0) || key_has_space(name)) {
        return -1;
    }
    if (len > NGCC_KAT_MAX_FIELD_BYTES) {
        return -1;
    }

    for (i = 0; i < builder->field_count; ++i) {
        if (key_equals_ci(builder->fields[i].name, name)) {
            data_dup = NULL;
            if (len > 0) {
                data_dup = (unsigned char *) malloc(len);
                if (data_dup == NULL) {
                    return -1;
                }
                memcpy(data_dup, data, len);
            }
            free(builder->fields[i].data);
            builder->fields[i].data = data_dup;
            builder->fields[i].len = len;
            return 0;
        }
    }

    name_dup = dup_string(name);
    if (name_dup == NULL) {
        return -1;
    }
    data_dup = NULL;
    if (len > 0) {
        data_dup = (unsigned char *) malloc(len);
        if (data_dup == NULL) {
            free(name_dup);
            return -1;
        }
        memcpy(data_dup, data, len);
    }

    {
        ngcc_kat_field_t *next = (ngcc_kat_field_t *) realloc(builder->fields,
                                                              (builder->field_count + 1U) * sizeof(builder->fields[0]));
        if (next == NULL) {
            free(name_dup);
            free(data_dup);
            return -1;
        }
        builder->fields = next;
    }

    builder->fields[builder->field_count].name = name_dup;
    builder->fields[builder->field_count].data = data_dup;
    builder->fields[builder->field_count].len = len;
    builder->field_count++;
    return 0;
}

static int append_vector(ngcc_kat_file_t *kat, const kat_builder_t *builder, size_t auto_index) {
    ngcc_kat_vector_t *next;
    ngcc_kat_vector_t *vec;
    size_t i;

    if (!builder_has_payload(builder)) {
        return 0;
    }
    if (kat->count >= NGCC_KAT_MAX_VECTORS) {
        return -1;
    }

    next = (ngcc_kat_vector_t *) realloc(kat->vectors, (kat->count + 1U) * sizeof(kat->vectors[0]));
    if (next == NULL) {
        return -1;
    }
    kat->vectors = next;
    vec = &kat->vectors[kat->count];
    memset(vec, 0, sizeof(*vec));

    vec->count = builder->has_count ? builder->count : (unsigned int) auto_index;
    vec->field_count = builder->field_count;
    vec->fields = (ngcc_kat_field_t *) calloc(vec->field_count, sizeof(vec->fields[0]));
    if (vec->fields == NULL) {
        return -1;
    }

    /* Commit the vector to the array now so that ngcc_kat_free() will clean up
     * the partially-filled fields if a subsequent allocation fails. */
    kat->count++;

    for (i = 0; i < vec->field_count; ++i) {
        vec->fields[i].name = dup_string(builder->fields[i].name);
        if (vec->fields[i].name == NULL) {
            return -1;
        }
        vec->fields[i].data = NULL;
        if (builder->fields[i].len > 0U) {
            if (builder->fields[i].len > NGCC_KAT_MAX_FIELD_BYTES) {
                return -1;
            }
            vec->fields[i].data = (unsigned char *) malloc(builder->fields[i].len);
            if (vec->fields[i].data == NULL) {
                return -1;
            }
            memcpy(vec->fields[i].data, builder->fields[i].data, builder->fields[i].len);
        }
        vec->fields[i].len = builder->fields[i].len;
    }

    return 0;
}

int ngcc_kat_parse_file(const char *path, ngcc_kat_file_t *out_kat) {
    FILE *fp;
    char *line = NULL;
    size_t line_len = 0;
    kat_builder_t builder;
    ngcc_kat_file_t kat;
    size_t auto_index = 0;
    int line_status;
    int rc = -1;

    if (path == NULL || out_kat == NULL) {
        return -1;
    }

    memset(&builder, 0, sizeof(builder));
    memset(&kat, 0, sizeof(kat));

    fp = fopen(path, "r");
    if (fp == NULL) {
        return -1;
    }

    while ((line_status = read_full_line(fp, &line, &line_len)) > 0) {
        char *s = trim(line);
        char *eq;
        char *key;
        char *value;

        if (*s == '\0') {
            if (builder_has_payload(&builder)) {
                if (append_vector(&kat, &builder, auto_index) != 0) {
                    free(line);
                    goto out;
                }
                auto_index++;
                builder_clear(&builder);
            }
            free(line);
            continue;
        }
        if (*s == '#' || *s == ';' || (*s == '/' && s[1] == '/')) {
            free(line);
            continue;
        }

        eq = strchr(s, '=');
        if (eq == NULL) {
            free(line);
            continue;
        }

        *eq = '\0';
        key = trim(s);
        value = trim(eq + 1);
        if (*key == '\0') {
            free(line);
            goto out;
        }

        if (key_equals_ci(key, "COUNT")) {
            char *end = NULL;
            unsigned long v;

            if (*value == '\0') {
                free(line);
                goto out;
            }

            if (builder_has_payload(&builder)) {
                if (append_vector(&kat, &builder, auto_index) != 0) {
                    free(line);
                    goto out;
                }
                auto_index++;
                builder_clear(&builder);
            }

            v = strtoul(value, &end, 10);
            if (end == NULL || *end != '\0') {
                free(line);
                goto out;
            }
            builder.count = (unsigned int) v;
            builder.has_count = 1;
        } else {
            unsigned char *buf = NULL;
            size_t len = 0;

            /* Len fields (e.g. Msg_Len, Dst_Len) are decimal integers;
               store them as 8-byte big-endian so callers can extract the value. */
            if (key_is_len_field(key)) {
                char *end = NULL;
                unsigned long long v;
                unsigned char len_buf[8];

                if (*value == '\0') {
                    free(line);
                    continue;
                }
                v = strtoull(value, &end, 10);
                if (end == NULL || *end != '\0') {
                    free(line);
                    continue;
                }
                len_buf[0] = (unsigned char)((v >> 56) & 0xFF);
                len_buf[1] = (unsigned char)((v >> 48) & 0xFF);
                len_buf[2] = (unsigned char)((v >> 40) & 0xFF);
                len_buf[3] = (unsigned char)((v >> 32) & 0xFF);
                len_buf[4] = (unsigned char)((v >> 24) & 0xFF);
                len_buf[5] = (unsigned char)((v >> 16) & 0xFF);
                len_buf[6] = (unsigned char)((v >> 8) & 0xFF);
                len_buf[7] = (unsigned char)(v & 0xFF);
                if (builder_set_field(&builder, key, len_buf, 8) != 0) {
                    free(line);
                    goto out;
                }
                free(line);
                continue;
            }

            if (parse_hex_bytes(value, &buf, &len) != 0) {
                if (key_is_metadata(key)) {
                    free(line);
                    continue;
                }
                free(line);
                continue;
            }
            if (builder_set_field(&builder, key, buf, len) != 0) {
                free(buf);
                free(line);
                goto out;
            }
            free(buf);
        }
        free(line);
    }
    if (line_status < 0) {
        goto out;
    }

    if (builder_has_payload(&builder)) {
        if (append_vector(&kat, &builder, auto_index) != 0) {
            goto out;
        }
        auto_index++;
        builder_clear(&builder);
    }

    if (kat.count == 0) {
        goto out;
    }

    *out_kat = kat;
    memset(&kat, 0, sizeof(kat));
    rc = 0;

out:
    fclose(fp);
    builder_clear(&builder);
    ngcc_kat_free(&kat);
    return rc;
}

const ngcc_kat_field_t *ngcc_kat_get_field(const ngcc_kat_vector_t *vec, const char *name) {
    size_t i;

    if (vec == NULL || name == NULL || *name == '\0') {
        return NULL;
    }

    for (i = 0; i < vec->field_count; ++i) {
        if (key_equals_ci(vec->fields[i].name, name)) {
            return &vec->fields[i];
        }
    }

    return NULL;
}

const ngcc_kat_field_t *ngcc_kat_get_field_any(const ngcc_kat_vector_t *vec,
                                               const char *const *names,
                                               size_t name_count) {
    size_t i;

    if (vec == NULL || names == NULL || name_count == 0U) {
        return NULL;
    }
    for (i = 0; i < name_count; ++i) {
        const ngcc_kat_field_t *field = ngcc_kat_get_field(vec, names[i]);
        if (field != NULL) {
            return field;
        }
    }
    return NULL;
}

void ngcc_kat_free(ngcc_kat_file_t *kat) {
    size_t i;
    size_t j;

    if (kat == NULL) {
        return;
    }

    for (i = 0; i < kat->count; ++i) {
        ngcc_kat_vector_t *vec = &kat->vectors[i];
        for (j = 0; j < vec->field_count; ++j) {
            free(vec->fields[j].name);
            free(vec->fields[j].data);
        }
        free(vec->fields);
    }
    free(kat->vectors);
    memset(kat, 0, sizeof(*kat));
}
