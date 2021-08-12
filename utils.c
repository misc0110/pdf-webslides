#include "utils.h"

#include <stdlib.h>
#include <string.h>

#include "webslides.h"

// ---------------------------------------------------------------------------
int base64encode(const void *data_buf, size_t dataLength, char *result,
                 size_t resultSize) {
  const char base64chars[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const uint8_t *data = (const uint8_t *)data_buf;
  size_t resultIndex = 0;
  size_t x;
  uint32_t n = 0;
  int padCount = dataLength % 3;
  uint8_t n0, n1, n2, n3;

  for (x = 0; x < dataLength; x += 3) {
    n = ((uint32_t)data[x]) << 16;

    if ((x + 1) < dataLength) n += ((uint32_t)data[x + 1]) << 8;
    if ((x + 2) < dataLength) n += data[x + 2];

    n0 = (uint8_t)(n >> 18) & 63;
    n1 = (uint8_t)(n >> 12) & 63;
    n2 = (uint8_t)(n >> 6) & 63;
    n3 = (uint8_t)n & 63;

    if (resultIndex >= resultSize) return 1;
    result[resultIndex++] = base64chars[n0];
    if (resultIndex >= resultSize) return 1;
    result[resultIndex++] = base64chars[n1];

    if ((x + 1) < dataLength) {
      if (resultIndex >= resultSize) return 1;
      result[resultIndex++] = base64chars[n2];
    }
    if ((x + 2) < dataLength) {
      if (resultIndex >= resultSize) return 1;
      result[resultIndex++] = base64chars[n3];
    }
  }
  if (padCount > 0) {
    for (; padCount < 3; padCount++) {
      if (resultIndex >= resultSize) return 1;
      result[resultIndex++] = '=';
    }
  }
  if (resultIndex >= resultSize) return 1;
  result[resultIndex] = 0;
  return 0;
}

// ---------------------------------------------------------------------------
char *read_file(char *fname) {
  FILE *f = fopen(fname, "rb");
  if (!f) {
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  size_t s = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *in = calloc(s + 1, 1);
  fread(in, s, 1, f);
  fclose(f);
  return in;
}

// ---------------------------------------------------------------------------
char *encode_file_base64(char *fname) {
  FILE *f = fopen(fname, "rb");
  fseek(f, 0, SEEK_END);
  size_t s = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *in = malloc(s);
  fread(in, s, 1, f);
  fclose(f);
  char *out = malloc(s * 2 + 8);
  base64encode(in, s, out, s * 2 + 8);
  free(in);
  return out;
}

// ---------------------------------------------------------------------------
char *encode_array_base64(char *array, size_t len) {
  char *out = malloc(len * 2 + 8);
  base64encode(array, len, out, len * 2 + 8);
  return out;
}

// ---------------------------------------------------------------------------
char *replace_string_first(char *input, char *search, char *replace) {
  size_t search_len = strlen(search);
  size_t input_len = strlen(input);
  size_t replace_len = strlen(replace);
  char *nbuff = malloc(input_len - search_len + replace_len + 1);
  size_t i, out = 0;
  for (i = 0; i < input_len; i++) {
    if (i < input_len - search_len &&
        strncmp(input + i, search, search_len) == 0) {
      for (size_t j = 0; j < replace_len; j++) {
        nbuff[out++] = replace[j];
      }
      i += search_len - 1;
    } else {
      nbuff[out++] = input[i];
    }
  }
  nbuff[out] = 0;
  free(input);
  return nbuff;
}

// ---------------------------------------------------------------------------
char *encode_array(SlideInfo *info, int offset, int len, int b64,
                   progress_t cb) {
  // calculate size
  size_t rlen = 0;
  for (int i = 0; i < len; i++) {
    char *d = ((char **)(info + i))[offset];
    if (d) {
      rlen += strlen(d) * (b64 + 1) + 8;
    } else {
      rlen += 8;
    }
  }
  char *buff = malloc(rlen);
  strcpy(buff, "[");

  for (int i = 0; i < len; i++) {
    char *data = ((char **)(info + i))[offset];
    char *b64_data = data;
    if (b64) {
      if (!data) data = "";
      b64_data = calloc(strlen(data) * 2 + 8, 1);
      base64encode(data, strlen(data), b64_data, strlen(data) * 2 + 8);
    }
    strcat(buff, "\"");
    strcat(buff, b64_data);
    strcat(buff, "\"");
    if (i != len - 1) strcat(buff, ", ");
    if (cb) cb(i);
  }

  strcat(buff, "]\n");
  return buff;
}

void append_elem(char **orig, const char *append, const char *split) {
  char *tmp = NULL;

  if (*orig == NULL) {
    *orig = calloc(strlen(append) + strlen(split) + 1, sizeof(char));
    strcat(*orig, split);
    strcat(*orig, append);
  } else {
    tmp = calloc(strlen(*orig) + 1, sizeof(char));
    memcpy(tmp, *orig, strlen(*orig));
    *orig = calloc(strlen(*orig) + strlen(split) + strlen(append) + 1, sizeof(char));
    strcat(*orig, tmp);
    strcat(*orig, split);
    strcat(*orig, append);
    free(tmp);
  }
}
