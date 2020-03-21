#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>
#include <stdint.h>

int base64encode(const void* data_buf, size_t dataLength, char* result, size_t resultSize);
char* encode_file_base64(char* fname);

#endif

