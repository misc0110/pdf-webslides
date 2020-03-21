#ifndef _WEBSLIDES_H_
#define _WEBSLIDES_H_


#define TAG_OK "[lg][+][/lg] "
#define TAG_INFO "[ly][*][/ly] "
#define TAG_FAIL "[bw][red][-][/red][/bw] "

typedef struct __attribute__((packed)) {
    char* annotations;
    char* videos;
    char* slide;
} SlideInfo;


// ---------------------------------------------------------------------------
typedef struct {
    int single;
    int presenter;
    char* pdf;
} Options;

#endif
