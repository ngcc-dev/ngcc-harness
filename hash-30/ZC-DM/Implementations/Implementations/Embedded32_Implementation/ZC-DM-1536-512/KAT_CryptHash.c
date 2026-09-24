/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#if defined(_WIN32)
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif
#include "drng.h"
#include "CryptHash_AlgorithmInstance.h"

#define SEED_LEN_BYTES 64
#define MSG_LEN_BITS_2_12 (4096ULL)
#define MSG_LEN_BITS_2_13 (8192ULL)
#define MSG_LEN_BITS_2_23 (8388608ULL)
#define MSG_LEN_BITS_2_33 (8589934592ULL)
#define KAT_SUCCESS 0
#define KAT_FILE_OPERATE_FAILED -1
#define KAT_MEMORY_ALLOCATION_FAILED -2
#define KAT_CRYPTHASH_FAILED -3
#define KAT_ALGORITHM_INSTANCE_NAME_INVALID -4

#define HIGH_N_BIT_MASK(N) ((unsigned char)((~0U) << (8 - (N))))

static int validate_algorithm_instance_name(const char *algorithm_instance_name);
static int create_directory(const char *path);
static int progress_bar(int total, const char *test_name);
static void fprint_message_full(FILE *fp, const unsigned char *msg, unsigned long long msg_len_bits);
static void fprint_message_partial(FILE *fp, const unsigned char *msg, unsigned long long msg_len_bits, const unsigned char *seed);
static void fprint_digest(FILE *fp, unsigned char *digest, int digest_len_bits);
static int gen_KAT_2_12(const char *algorithm_instance_name, const int digest_len_bits);
static int gen_KAT_2_23(const char *algorithm_instance_name, const int digest_len_bits);
static int gen_KAT_2_33(const char *algorithm_instance_name, const int digest_len_bits);
static int gen_KAT_Loop(const char *algorithm_instance_name, const int digest_len_bits);

int main()
{
    clock_t start_time, end_time;
    double time_elapsed;
    int rt = KAT_SUCCESS;
    start_time = clock();

    if (validate_algorithm_instance_name(ALGORITHM_INSTANCE))
    {
        fprintf(stderr, "ERROR: Invalid algorithm instance name. Only letters, numbers, '-' or '_' are permitted.\n");
        rt = KAT_ALGORITHM_INSTANCE_NAME_INVALID;
        goto end;
    }

    // Generate the message digests for messages of lengths ranging from 0 to 2^12 bits
    progress_bar(4, "KAT_2_12");
    rt = gen_KAT_2_12(ALGORITHM_INSTANCE, DIGEST_BIT_LENGTH);
    if (KAT_SUCCESS != rt)
    {
        goto end;
    }

    // Generate the message digest for a message of length 2^23 bits
    progress_bar(4, "KAT_2_23");
    rt = gen_KAT_2_23(ALGORITHM_INSTANCE, DIGEST_BIT_LENGTH);
    if (KAT_SUCCESS != rt)
    {
        goto end;
    }

    // Generate the message digest for a message of length 2^33 bits
    progress_bar(4, "KAT_2_33");
    rt = gen_KAT_2_33(ALGORITHM_INSTANCE, DIGEST_BIT_LENGTH);
    if (KAT_SUCCESS != rt)
    {
        goto end;
    }

    // Generate the message digest for a message of length 2^13 bits by running Loop Test
    //***********************Loop Test***********************//
    // a. h_0 = CryptHash(m_0);
    // b. Loop i \in [0:1,000,000) Step 1
    //        m_i = m_i <<< digest_len_bits;
    //        m_(i+1) = m_i XOR (h_i||000...000_(2^13 - digest_len_bits));
    //        h_(i+1) = CryptHash(m_(i+1));
    // c. Output h_1000000
    //*******************************************************//
    progress_bar(4, "KAT_Loop");
    rt = gen_KAT_Loop(ALGORITHM_INSTANCE, DIGEST_BIT_LENGTH);
    if (KAT_SUCCESS != rt)
    {
        goto end;
    }

    end_time = clock();
    time_elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    progress_bar(4, "");
    printf("\nFiles have been saved in the 'output' folder within the working directory.");
    printf("\nTotal duration: %.0lf s\n", time_elapsed);
end:
    return rt;
}

static int validate_algorithm_instance_name(const char *algorithm_instance_name)
{
    int rt = EXIT_SUCCESS;
    const int MAX_LENGTH = 64;
    char c = '\0';
    
    if (strlen(algorithm_instance_name) > MAX_LENGTH)
    {
        rt = KAT_ALGORITHM_INSTANCE_NAME_INVALID;
        goto end;
    }

    for (int i = 0; algorithm_instance_name[i] != '\0'; i++)
    {
        c = algorithm_instance_name[i];
        if (!(isalnum(c) || '-' == c || '_' == c))
        {
            rt = KAT_ALGORITHM_INSTANCE_NAME_INVALID;
            break;
        }
    }
end:
    return rt;
}

static int create_directory(const char *path)
{
#if defined(_WIN32)
    if (0 == _mkdir(path))
        return EXIT_SUCCESS;
#else
    if (0 == mkdir(path, 0755))
        return EXIT_SUCCESS;
#endif

    if (errno == EEXIST)
    {
#if defined(_WIN32)
        if (0 == _access(path, 0))
            return EXIT_SUCCESS;
#else
        if (0 == access(path, F_OK))
            return EXIT_SUCCESS;
#endif
    }

    return EXIT_FAILURE;
}

static int progress_bar(int total, const char *test_name)
{
    static int number = 0;
    if (total <= 0 || number > total)
        return EXIT_FAILURE;
    const int rate = number * 100 / total;
    static char ProgressBar[50 + 1];

    memset(ProgressBar, 0, sizeof(ProgressBar));
    memset(ProgressBar, '#', rate / 2);
    printf("[%-50s] [%3d%%]  ", ProgressBar, rate);
    if (number <= (total - 1))
    {
        printf("Doing:\"%s\"   \r", test_name);
    }
    else
    {
        printf("SUCCESS!           \r");
    }
    number++;
    fflush(stdout);
    return EXIT_SUCCESS;
}

static void fprint_message_full(FILE *fp, const unsigned char *msg, unsigned long long msg_len_bits)
{
    // Print complete message
    fprintf(fp, "Msg_Len = %llu\n", msg_len_bits);
    fprintf(fp, "Msg = ");
    for (unsigned long long i = 0ULL; i < msg_len_bits / 8; i++)
    {
        fprintf(fp, "%02X", msg[i]);
    }
    if (msg_len_bits % 8)
    {
        fprintf(fp, "%02X", (HIGH_N_BIT_MASK(msg_len_bits % 8)) & (msg[msg_len_bits / 8]));
    }
    fprintf(fp, "\n");
}

static void fprint_message_partial(FILE *fp, const unsigned char *msg, unsigned long long msg_len_bits, const unsigned char *seed)
{
    // Print the complete "Msg_Seed" and part of the message
    fprintf(fp, "Msg_Len = %llu\n", msg_len_bits);
    fprintf(fp, "Msg_Seed = ");
    if (NULL != seed)
    {
        for (unsigned long long i = 0; i < SEED_LEN_BYTES; i++)
        {
            fprintf(fp, "%02X", seed[i]);
        }
    }
    fprintf(fp, "\n");
    fprintf(fp, "Msg_Exp = ");
    for (int i = 0; i < 8; i++)
    {
        fprintf(fp, "%02X", msg[i]);
    }
    fprintf(fp, " .... ");
    for (int i = 0; i < 8; i++)
    {
        fprintf(fp, "%02X", msg[msg_len_bits / 8 - 8 + i]);
    }
    fprintf(fp, "\n");
}

static void fprint_digest(FILE *fp, unsigned char *digest, int digest_len_bits)
{
    fprintf(fp, "Dst_Len = %d\n", digest_len_bits);
    fprintf(fp, "Dst = ");
    if (NULL != digest)
    {
        for (int i = 0; i < digest_len_bits / 8; i++)
        {
            fprintf(fp, "%02X", digest[i]);
        }
    }
    fprintf(fp, "\n");
}

static int gen_KAT_2_12(const char *algorithm_instance_name, const int digest_len_bits)
{
    unsigned char *msg = NULL;
    unsigned long long msg_len_bits;
    unsigned char *digest = NULL;
    char filename[96] = "KAT_2_12_";
    const char *dir_name = "output";
    char file_path[128] = "";
    int rt = KAT_SUCCESS;
    unsigned char seed[SEED_LEN_BYTES];
    FILE *fp;
    DRNG_ctx drng_msg_2_12;

    strcat(filename, algorithm_instance_name);
    strcat(filename, ".txt");
    sprintf(file_path, "%s/%s", dir_name, filename);
    if (0 != create_directory(dir_name))
    {
        fprintf(stderr, "\nERROR: Generate folder \"%s\" failed at %s, line %d. \n", dir_name, __FILE__, __LINE__);
        rt = KAT_FILE_OPERATE_FAILED;
        goto end;
    }
    fp = fopen(file_path, "wb");
    if (NULL == fp)
    {
        fprintf(stderr, "\nERROR: Generate \"%s\" failed at %s, line %d. \n", filename, __FILE__, __LINE__);
        rt = KAT_FILE_OPERATE_FAILED;
        goto end;
    }

    // "KAT_2_12" repeats 8 times as seed
    for (unsigned long long i = 0; i < sizeof(seed) / 8; i++)
    {
        memcpy(seed + 8 * i, filename, 8);
    }
    msg = (unsigned char *)malloc(MSG_LEN_BITS_2_12 / 8);
    if (NULL == msg)
    {
        fprintf(stderr, "\nERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
        rt = KAT_MEMORY_ALLOCATION_FAILED;
        goto cleanup;
    }
    memset(msg, 0, MSG_LEN_BITS_2_12 / 8);
    digest = (unsigned char *)malloc(digest_len_bits / 8);
    if (NULL == digest)
    {
        fprintf(stderr, "\nERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
        rt = KAT_MEMORY_ALLOCATION_FAILED;
        goto cleanup;
    }
    memset(digest, 0, digest_len_bits / 8);
    init_random_number(&drng_msg_2_12, seed, sizeof(seed));
    for (msg_len_bits = 0ULL; msg_len_bits <= MSG_LEN_BITS_2_12; msg_len_bits++)
    {
        // Random
        memset(msg, 0, MSG_LEN_BITS_2_12 / 8);
        get_random_number(&drng_msg_2_12, msg, msg_len_bits);
        fprint_message_full(fp, msg, msg_len_bits);
        if (CryptHash(digest_len_bits, msg, msg_len_bits, digest))
        {
            fprintf(stderr, "\nERROR: \"CryptHash()\" returns a non-zero value when generating \"%s\" at %s, line %d. \n", filename, __FILE__, __LINE__);
            rt = KAT_CRYPTHASH_FAILED;
            goto cleanup;
        }
        else
        {
            fprint_digest(fp, OUTPUT_BLANK_TEST_VECTORS ? NULL : digest, digest_len_bits);
        }
        fprintf(fp, "\n");
    }
cleanup:
    free(digest);
    free(msg);
    if (0 != fclose(fp))
    {
        fprintf(stderr, "\nERROR: Generate \"%s\" failed at %s, line %d. \n", filename, __FILE__, __LINE__);
        rt = KAT_FILE_OPERATE_FAILED;
        goto end;
    }
end:
    return rt;
}

static int gen_KAT_2_23(const char *algorithm_instance_name, const int digest_len_bits)
{
    unsigned char *msg = NULL;
    const unsigned long long msg_len_bits = MSG_LEN_BITS_2_23;
    unsigned char *digest = NULL;
    char filename[96] = "KAT_2_23_";
    const char *dir_name = "output";
    char file_path[128] = "";
    int rt = KAT_SUCCESS;
    unsigned char seed[SEED_LEN_BYTES];
    FILE *fp;
    DRNG_ctx drng_msg_2_23;

    strcat(filename, algorithm_instance_name);
    strcat(filename, ".txt");
    sprintf(file_path, "%s/%s", dir_name, filename);
    if (0 != create_directory(dir_name))
    {
        fprintf(stderr, "\nERROR: Generate folder \"%s\" failed at %s, line %d. \n", dir_name, __FILE__, __LINE__);
        rt = KAT_FILE_OPERATE_FAILED;
        goto end;
    }
    fp = fopen(file_path, "wb");
    if (NULL == fp)
    {
        fprintf(stderr, "\nERROR: Generate \"%s\" failed at %s, line %d. \n", filename, __FILE__, __LINE__);
        rt = KAT_FILE_OPERATE_FAILED;
        goto end;
    }

    // "KAT_2_23" repeats 8 times as seed
    for (unsigned long long i = 0; i < sizeof(seed) / 8; i++)
    {
        memcpy(seed + 8 * i, filename, 8);
    }
    msg = (unsigned char *)malloc(MSG_LEN_BITS_2_23 / 8);
    if (NULL == msg)
    {
        fprintf(stderr, "\nERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
        rt = KAT_MEMORY_ALLOCATION_FAILED;
        goto cleanup;
    }
    memset(msg, 0, MSG_LEN_BITS_2_23 / 8);
    digest = (unsigned char *)malloc(digest_len_bits / 8);
    if (NULL == digest)
    {
        fprintf(stderr, "\nERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
        rt = KAT_MEMORY_ALLOCATION_FAILED;
        goto cleanup;
    }
    memset(digest, 0, digest_len_bits / 8);
    init_random_number(&drng_msg_2_23, seed, sizeof(seed));
    // All 0
    memset(msg, 0, msg_len_bits / 8);
    fprint_message_partial(fp, msg, msg_len_bits, NULL);
    if (CryptHash(digest_len_bits, msg, msg_len_bits, digest))
    {
        fprintf(stderr, "\nERROR: \"CryptHash()\" returns a non-zero value when generating \"%s\" at %s, line %d. \n", filename, __FILE__, __LINE__);
        rt = KAT_CRYPTHASH_FAILED;
        goto cleanup;
    }
    else
    {
        fprint_digest(fp, OUTPUT_BLANK_TEST_VECTORS ? NULL : digest, digest_len_bits);
    }
    fprintf(fp, "\n");
    // All 1
    memset(msg, 0xFF, msg_len_bits / 8);
    fprint_message_partial(fp, msg, msg_len_bits, NULL);
    if (CryptHash(digest_len_bits, msg, msg_len_bits, digest))
    {
        fprintf(stderr, "\nERROR: \"CryptHash()\" returns a non-zero value when generating \"%s\" at %s, line %d. \n", filename, __FILE__, __LINE__);
        rt = KAT_CRYPTHASH_FAILED;
        goto cleanup;
    }
    else
    {
        fprint_digest(fp, OUTPUT_BLANK_TEST_VECTORS ? NULL : digest, digest_len_bits);
    }
    fprintf(fp, "\n");
    // Random
    memset(msg, 0, (msg_len_bits + 7) / 8);
    get_random_number(&drng_msg_2_23, msg, msg_len_bits);
    fprint_message_partial(fp, msg, msg_len_bits, seed);
    if (CryptHash(digest_len_bits, msg, msg_len_bits, digest))
    {
        fprintf(stderr, "\nERROR: \"CryptHash()\" returns a non-zero value when generating \"%s\" at %s, line %d. \n", filename, __FILE__, __LINE__);
        rt = KAT_CRYPTHASH_FAILED;
        goto cleanup;
    }
    else
    {
        fprint_digest(fp, OUTPUT_BLANK_TEST_VECTORS ? NULL : digest, digest_len_bits);
    }
    fprintf(fp, "\n");
cleanup:
    free(digest);
    free(msg);
    if (0 != fclose(fp))
    {
        fprintf(stderr, "\nERROR: Generate \"%s\" failed at %s, line %d. \n", filename, __FILE__, __LINE__);
        rt = KAT_FILE_OPERATE_FAILED;
        goto end;
    }
end:
    return rt;
}

static int gen_KAT_2_33(const char *algorithm_instance_name, const int digest_len_bits)
{
    unsigned char *msg = NULL;
    const unsigned long long msg_len_bits = MSG_LEN_BITS_2_33;
    unsigned char *digest = NULL;
    char filename[96] = "KAT_2_33_";
    const char *dir_name = "output";
    char file_path[128] = "";
    int rt = KAT_SUCCESS;
    unsigned char seed[SEED_LEN_BYTES];
    FILE *fp;
    DRNG_ctx drng_msg_2_33;

    strcat(filename, algorithm_instance_name);
    strcat(filename, ".txt");
    sprintf(file_path, "%s/%s", dir_name, filename);
    if (0 != create_directory(dir_name))
    {
        fprintf(stderr, "\nERROR: Generate folder \"%s\" failed at %s, line %d. \n", dir_name, __FILE__, __LINE__);
        rt = KAT_FILE_OPERATE_FAILED;
        goto end;
    }
    fp = fopen(file_path, "wb");
    if (NULL == fp)
    {
        fprintf(stderr, "\nERROR: Generate \"%s\" failed at %s, line %d. \n", filename, __FILE__, __LINE__);
        rt = KAT_FILE_OPERATE_FAILED;
        goto end;
    }

    // "KAT_2_33" repeats 8 times as seed
    for (unsigned long long i = 0; i < sizeof(seed) / 8; i++)
    {
        memcpy(seed + 8 * i, filename, 8);
    }
    msg = (unsigned char *)malloc(MSG_LEN_BITS_2_33 / 8);
    if (NULL == msg)
    {
        fprintf(stderr, "\nERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
        rt = KAT_MEMORY_ALLOCATION_FAILED;
        goto cleanup;
    }
    memset(msg, 0, MSG_LEN_BITS_2_33 / 8);
    digest = (unsigned char *)malloc(digest_len_bits / 8);
    if (NULL == digest)
    {
        fprintf(stderr, "\nERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
        rt = KAT_MEMORY_ALLOCATION_FAILED;
        goto cleanup;
    }
    memset(digest, 0, digest_len_bits / 8);
    init_random_number(&drng_msg_2_33, seed, sizeof(seed));
    // All 0
    memset(msg, 0, msg_len_bits / 8);
    fprint_message_partial(fp, msg, msg_len_bits, NULL);
    if (CryptHash(digest_len_bits, msg, msg_len_bits, digest))
    {
        fprintf(stderr, "\nERROR: \"CryptHash()\" returns a non-zero value when generating \"%s\" at %s, line %d. \n", filename, __FILE__, __LINE__);
        rt = KAT_CRYPTHASH_FAILED;
        goto cleanup;
    }
    else
    {
        fprint_digest(fp, OUTPUT_BLANK_TEST_VECTORS ? NULL : digest, digest_len_bits);
    }
    fprintf(fp, "\n");
    // All 1
    memset(msg, 0xFF, msg_len_bits / 8);
    fprint_message_partial(fp, msg, msg_len_bits, NULL);
    if (CryptHash(digest_len_bits, msg, msg_len_bits, digest))
    {
        fprintf(stderr, "\nERROR: \"CryptHash()\" returns a non-zero value when generating \"%s\" at %s, line %d. \n", filename, __FILE__, __LINE__);
        rt = KAT_CRYPTHASH_FAILED;
        goto cleanup;
    }
    else
    {
        fprint_digest(fp, OUTPUT_BLANK_TEST_VECTORS ? NULL : digest, digest_len_bits);
    }
    fprintf(fp, "\n");
    // Random
    memset(msg, 0, (msg_len_bits + 7) / 8);
    get_random_number(&drng_msg_2_33, msg, msg_len_bits);
    fprint_message_partial(fp, msg, msg_len_bits, seed);
    if (CryptHash(digest_len_bits, msg, msg_len_bits, digest))
    {
        fprintf(stderr, "\nERROR: \"CryptHash()\" returns a non-zero value when generating \"%s\" at %s, line %d. \n", filename, __FILE__, __LINE__);
        rt = KAT_CRYPTHASH_FAILED;
        goto cleanup;
    }
    else
    {
        fprint_digest(fp, OUTPUT_BLANK_TEST_VECTORS ? NULL : digest, digest_len_bits);
    }
    fprintf(fp, "\n");
cleanup:
    free(digest);
    free(msg);
    if (0 != fclose(fp))
    {
        fprintf(stderr, "\nERROR: Generate \"%s\" failed at %s, line %d. \n", filename, __FILE__, __LINE__);
        rt = KAT_FILE_OPERATE_FAILED;
        goto end;
    }
end:
    return rt;
}

static int gen_KAT_Loop(const char *algorithm_instance_name, const int digest_len_bits)
{
    unsigned char *msg = NULL;
    const unsigned long long msg_len_bits = MSG_LEN_BITS_2_13;
    unsigned char *buffer = NULL;
    unsigned char *digest = NULL;
    char filename[96] = "KAT_Loop_";
    const char *dir_name = "output";
    char file_path[128] = "";
    int rt = KAT_SUCCESS;
    unsigned char seed[SEED_LEN_BYTES];
    FILE *fp;
    DRNG_ctx drng_msg_2_13;

    strcat(filename, algorithm_instance_name);
    strcat(filename, ".txt");
    sprintf(file_path, "%s/%s", dir_name, filename);
    if (0 != create_directory(dir_name))
    {
        fprintf(stderr, "\nERROR: Generate folder \"%s\" failed at %s, line %d. \n", dir_name, __FILE__, __LINE__);
        rt = KAT_FILE_OPERATE_FAILED;
        goto end;
    }
    fp = fopen(file_path, "wb");
    if (NULL == fp)
    {
        fprintf(stderr, "\nERROR: Generate \"%s\" failed at %s, line %d. \n", filename, __FILE__, __LINE__);
        rt = KAT_FILE_OPERATE_FAILED;
        goto end;
    }

    // "KAT_Loop" repeats 8 times as seed
    for (unsigned long long i = 0; i < sizeof(seed) / 8; i++)
    {
        memcpy(seed + 8 * i, filename, 8);
    }
    msg = (unsigned char *)malloc(msg_len_bits / 8);
    if (NULL == msg)
    {
        fprintf(stderr, "\nERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
        rt = KAT_MEMORY_ALLOCATION_FAILED;
        goto cleanup;
    }
    buffer = (unsigned char *)malloc(digest_len_bits / 8);
    if (NULL == buffer)
    {
        fprintf(stderr, "\nERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
        rt = KAT_MEMORY_ALLOCATION_FAILED;
        goto cleanup;
    }
    digest = (unsigned char *)malloc(digest_len_bits / 8);
    if (NULL == digest)
    {
        fprintf(stderr, "\nERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
        rt = KAT_MEMORY_ALLOCATION_FAILED;
        goto cleanup;
    }
    init_random_number(&drng_msg_2_13, seed, sizeof(seed));
    get_random_number(&drng_msg_2_13, msg, msg_len_bits);
    fprint_message_full(fp, msg, msg_len_bits);
    if (CryptHash(digest_len_bits, msg, msg_len_bits, digest))
    {
        fprintf(stderr, "\nERROR: \"CryptHash()\" returns a non-zero value when generating \"%s\" at %s, line %d. \n", filename, __FILE__, __LINE__);
        rt = KAT_CRYPTHASH_FAILED;
        goto cleanup;
    }
    else
    {
        for (int i = 0; i < 1000000; i++)
        {
            memcpy(buffer, msg, digest_len_bits / 8);
            memmove(msg, msg + digest_len_bits / 8, (msg_len_bits - digest_len_bits) / 8);
            memcpy(msg + (msg_len_bits - digest_len_bits) / 8, buffer, digest_len_bits / 8);
            for (int j = 0; j < digest_len_bits / 8; j++)
            {
                msg[j] ^= digest[j];
            }
            CryptHash(digest_len_bits, msg, msg_len_bits, digest);
        }
        fprint_digest(fp, OUTPUT_BLANK_TEST_VECTORS ? NULL : digest, digest_len_bits);
    }
    fprintf(fp, "\n");
cleanup:
    free(digest);
    free(buffer);
    free(msg);
    if (0 != fclose(fp))
    {
        fprintf(stderr, "\nERROR: Generate \"%s\" failed at %s, line %d. \n", filename, __FILE__, __LINE__);
        rt = KAT_FILE_OPERATE_FAILED;
        goto end;
    }
end:
    return rt;
}