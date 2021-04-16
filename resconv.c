#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

char* replace_name(char* in) {
    char* out = strdup(in);
    for(int i = 0; i < strlen(out); i++) {
        if(out[i] == '/' || out[i] == '.') out[i] = '_';
    }
    return out;
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }
    
    FILE* f = fopen(argv[1], "rb");
    if(!f) {
        printf("Could not open file %s\n", argv[1]);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* data = malloc(len + 1);
    fread(data, len, 1, f);
    fclose(f);
     
    char* name = replace_name(argv[1]);
    printf("unsigned char %s[] = {", name);
    for(int i = 0; i < (int)len; i++) {
        printf("0x%02x", (unsigned char)(data[i]));
        if(i < (int)len - 1) {
            printf(", ");
        }
    }
    printf(", 0};\nunsigned int %s_len = %d;\n", name, (int)len);
    fflush(stdout);
    free(name);
    free(data);
    return 0;
}
