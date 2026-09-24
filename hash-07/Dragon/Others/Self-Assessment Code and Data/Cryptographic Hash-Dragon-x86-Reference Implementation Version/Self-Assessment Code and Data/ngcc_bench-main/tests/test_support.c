#define _POSIX_C_SOURCE 200809L
#include <unistd.h>

#include "test_support.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int append_bytes(char **buf, size_t *len, size_t *cap, const char *chunk, size_t chunk_len) {
    char *next;
    size_t needed;

    if (chunk == NULL || chunk_len == 0) {
        return 0;
    }

    needed = *len + chunk_len + 1;
    if (needed > *cap) {
        size_t next_cap = (*cap == 0) ? 1024 : *cap;
        while (next_cap < needed) {
            next_cap *= 2;
        }
        next = (char *) realloc(*buf, next_cap);
        if (next == NULL) {
            return -1;
        }
        *buf = next;
        *cap = next_cap;
    }

    memcpy(*buf + *len, chunk, chunk_len);
    *len += chunk_len;
    (*buf)[*len] = '\0';
    return 0;
}

int test_run_command(char *const argv[], test_command_result_t *out_result) {
    int pipefd[2];
    pid_t pid;
    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;
    int status;
    ssize_t nread;
    char chunk[4096];

    if (argv == NULL || argv[0] == NULL || out_result == NULL) {
        return -1;
    }

    memset(out_result, 0, sizeof(*out_result));

    if (pipe(pipefd) != 0) {
        return -1;
    }

    pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0 || dup2(pipefd[1], STDERR_FILENO) < 0) {
            _exit(127);
        }
        close(pipefd[1]);
        execv(argv[0], argv);
        _exit(127);
    }

    close(pipefd[1]);
    while ((nread = read(pipefd[0], chunk, sizeof(chunk))) > 0) {
        if (append_bytes(&buf, &len, &cap, chunk, (size_t) nread) != 0) {
            close(pipefd[0]);
            waitpid(pid, NULL, 0);
            free(buf);
            return -1;
        }
    }
    close(pipefd[0]);

    if (buf == NULL) {
        buf = (char *) malloc(1);
        if (buf == NULL) {
            waitpid(pid, NULL, 0);
            return -1;
        }
        buf[0] = '\0';
    }

    if (waitpid(pid, &status, 0) < 0) {
        free(buf);
        return -1;
    }

    out_result->output = buf;
    out_result->output_len = len;
    if (WIFEXITED(status)) {
        out_result->exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        out_result->exit_code = 128 + WTERMSIG(status);
    } else {
        out_result->exit_code = -1;
    }

    return 0;
}

void test_free_command_result(test_command_result_t *result) {
    if (result == NULL) {
        return;
    }
    free(result->output);
    memset(result, 0, sizeof(*result));
}

int test_read_file(const char *path, char **out_data) {
    FILE *fp;
    long size;
    char *buf;

    if (path == NULL || out_data == NULL) {
        return -1;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return -1;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return -1;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }

    buf = (char *) malloc((size_t) size + 1);
    if (buf == NULL) {
        fclose(fp);
        return -1;
    }
    if (size > 0 && fread(buf, 1, (size_t) size, fp) != (size_t) size) {
        fclose(fp);
        free(buf);
        return -1;
    }
    fclose(fp);
    buf[size] = '\0';
    *out_data = buf;
    return 0;
}

int test_write_file(const char *path, const char *content) {
    FILE *fp;
    size_t len;

    if (path == NULL || content == NULL) {
        return -1;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        return -1;
    }

    len = strlen(content);
    if (len > 0 && fwrite(content, 1, len, fp) != len) {
        fclose(fp);
        return -1;
    }

    if (fclose(fp) != 0) {
        return -1;
    }
    return 0;
}

int test_make_temp_dir(char *tmpl) {
    if (tmpl == NULL) {
        return -1;
    }
    return mkdtemp(tmpl) != NULL ? 0 : -1;
}

static int remove_tree_impl(const char *path) {
    DIR *dir;
    struct dirent *ent;
    struct stat st;
    char child[PATH_MAX];

    if (lstat(path, &st) != 0) {
        return -1;
    }

    if (!S_ISDIR(st.st_mode)) {
        return unlink(path);
    }

    dir = opendir(path);
    if (dir == NULL) {
        return -1;
    }

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        if (snprintf(child, sizeof(child), "%s/%s", path, ent->d_name) >= (int) sizeof(child)) {
            closedir(dir);
            return -1;
        }
        if (remove_tree_impl(child) != 0) {
            closedir(dir);
            return -1;
        }
    }

    closedir(dir);
    return rmdir(path);
}

int test_remove_tree(const char *path) {
    if (path == NULL) {
        return -1;
    }
    if (access(path, F_OK) != 0) {
        return 0;
    }
    return remove_tree_impl(path);
}

int test_mkdir_p(const char *path) {
    char tmp[PATH_MAX];
    char *p;

    if (path == NULL || path[0] == '\0') {
        return -1;
    }
    if (snprintf(tmp, sizeof(tmp), "%s", path) >= (int) sizeof(tmp)) {
        return -1;
    }

    for (p = tmp + 1; *p != '\0'; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0777) != 0 && errno != EEXIST) {
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, 0777) != 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

int test_output_contains(const char *haystack, const char *needle) {
    return haystack != NULL && needle != NULL && strstr(haystack, needle) != NULL;
}

int test_file_contains(const char *path, const char *needle) {
    char *data = NULL;
    int found;

    if (test_read_file(path, &data) != 0) {
        return 0;
    }
    found = test_output_contains(data, needle);
    free(data);
    return found;
}
