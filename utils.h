#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>
#include <stdio.h>

#include "webslides.h"

typedef void (*progress_t)(int);

int base64encode(const void* data_buf, size_t dataLength, char* result,
                 size_t resultSize);
char* encode_file_base64(char* fname);
char* read_file(char* fname);
char* replace_string_first(char* input, char* search, char* replace);
char* encode_array(SlideInfo* info, int offset, int len, int b64,
                   progress_t cb);
char* encode_array_base64(char* array, size_t len);

#endif
