#include "tools.h"

#include <string.h>

uint64_t TM_start = 0;
uint64_t TM_end = 0;
uint64_t *TM_mem = NULL;
uint64_t *TM_mem1 = NULL;
uint64_t *TM_mem2 = NULL;
uint64_t *TM_mem3 = NULL;

static size_t timing_capacity = 0;

static int compare_u64(const void *left, const void *right) {
  const uint64_t a = *(const uint64_t *) left;
  const uint64_t b = *(const uint64_t *) right;
  return (a > b) - (a < b);
}

static int resize_timing_array(uint64_t **array, size_t count) {
  uint64_t *replacement = (uint64_t *) realloc(*array,
                                                count * sizeof(uint64_t));
  if(replacement == NULL && count != 0) {
    return -1;
  }
  *array = replacement;
  if(count != 0) {
    memset(*array, 0, count * sizeof(uint64_t));
  }
  return 0;
}

int tools_prepare_timing(size_t loop_count, int series_count) {
  if(loop_count > timing_capacity) {
    if(resize_timing_array(&TM_mem, loop_count) != 0 ||
       resize_timing_array(&TM_mem1, loop_count) != 0 ||
       resize_timing_array(&TM_mem2, loop_count) != 0 ||
       resize_timing_array(&TM_mem3, loop_count) != 0) {
      return -1;
    }
    timing_capacity = loop_count;
  } else {
    if(series_count == 1 && loop_count != 0) {
      memset(TM_mem, 0, loop_count * sizeof(uint64_t));
    } else if(series_count == 3 && loop_count != 0) {
      memset(TM_mem1, 0, loop_count * sizeof(uint64_t));
      memset(TM_mem2, 0, loop_count * sizeof(uint64_t));
      memset(TM_mem3, 0, loop_count * sizeof(uint64_t));
    }
  }
  return 0;
}

void print_bytes(const uint8_t *arr, int32_t len) {
  if(len >= 1) {
    printf("0x");
  }
  for(int32_t i = 0; i < len; ++i) {
    printf("%02x", arr[i]);
  }
}

void PrintLog(const char *message, const char *file, int line,
              const char *function) {
  FILE *output = fopen(LOGFILE, "a");
  if(output == NULL) {
    ErrorInfo("Failed to open the file %s.", LOGFILE);
    return;
  }

  time_t now = time(NULL);
  const char *timestamp = ctime(&now);
  if(timestamp != NULL) {
    fputs(timestamp, output);
  }
  fprintf(output, "File: %s, Line: %d, Function: %s. %s\n",
          file, line, function, message);
  fflush(output);
  fclose(output);
}

static void analyze_one(uint64_t *values, int loop, const char *label) {
  uint64_t average = 0;
  qsort(values, (size_t) loop, sizeof(uint64_t), compare_u64);
  for(int i = 0; i < loop; ++i) {
    average += values[i];
  }
  average /= (uint64_t) loop;

  printf("Running Time of <%s>:\n\tLoop\t%10d times\n"
         "\tMinimum\t%10" PRIu64 " cycles\n"
         "\tMaximum\t%10" PRIu64 " cycles\n"
         "\tMedian\t%10" PRIu64 " cycles\n"
         "\tAverage\t%10" PRIu64 " cycles\n",
         label, loop, values[0], values[loop - 1], values[loop >> 1],
         average);
}

void Analyis_TM(int loop, const char *label) {
  analyze_one(TM_mem, loop, label);
}

void Analyis_TM_3(int loop, const char *label1, const char *label2,
                  const char *label3) {
  analyze_one(TM_mem1, loop, label1);
  analyze_one(TM_mem2, loop, label2);
  analyze_one(TM_mem3, loop, label3);
}
