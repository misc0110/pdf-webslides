#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef _CP_STRICT
#define _CP_STRICT 0
#endif

#ifndef DEBUG
#define DEBUG (void)
#endif

#define _CP_MAX_COMMAND_LENGTH 16


typedef enum {
    RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, RESET, LIGHTRED, LIGHTGREEN, LIGHTYELLOW, LIGHTBLUE, LIGHTMAGENTA, LIGHTCYAN, LIGHTWHITE
} _cp_ansi__cp_color_t;


typedef enum {
    TAG_OPEN,
    TAG_CLOSE
} _cp_tag_t;

typedef enum {
    FOREGROUND,
    BACKGROUND
} _cp__cp_color_type_t;

typedef struct {
    _cp_ansi__cp_color_t stack[16];
    int ptr;
} _cp_colorstack_t;

typedef struct {
    char raw[_CP_MAX_COMMAND_LENGTH];
    int valid;
    _cp_tag_t tag;
    size_t length;
} _cp_command_t;

typedef struct {
    size_t len;
    size_t capacity;
    char* data;
} _cp_buffer_t;

typedef struct {
    _cp_ansi__cp_color_t color;
    _cp__cp_color_type_t type;
} _cp_color_t;


static const char *_cp_fg_color_strings[] = {
        [RED] = "31",
        [GREEN] = "32",
        [YELLOW] = "33",
        [BLUE] = "34",
        [MAGENTA] = "35",
        [CYAN] = "36",
        [WHITE] = "37",
        [RESET] = "0",
        [LIGHTRED] = "91",
        [LIGHTGREEN] = "92",
        [LIGHTYELLOW] = "93",
        [LIGHTBLUE] = "94",
        [LIGHTMAGENTA] = "95",
        [LIGHTCYAN] = "96",
        [LIGHTWHITE] = "97"
};


static const char *_cp_bg_color_strings[] = {
        [RESET] = "49",
        [RED] = "41",
        [GREEN] = "42",
        [YELLOW] = "43",
        [BLUE] = "44",
        [MAGENTA] = "45",
        [CYAN] = "46",
        [WHITE] = "47",
        [LIGHTRED] = "101",
        [LIGHTGREEN] = "102",
        [LIGHTYELLOW] = "103",
        [LIGHTBLUE] = "104",
        [LIGHTMAGENTA] = "105",
        [LIGHTCYAN] = "106",
        [LIGHTWHITE] = "107"
};


static void _cp_buffer_increase_size(_cp_buffer_t* buffer, size_t size) {
    while(buffer->len + size >= buffer->capacity) {
        if(buffer->capacity < 128) buffer->capacity = 128;
        else buffer->capacity *= 2;
    }
    buffer->data = realloc(buffer->data, buffer->capacity);
    memset(buffer->data + buffer->len, 0, buffer->capacity - buffer->len);
}


static _cp_buffer_t* _cp_buffer_create(size_t len) {
    _cp_buffer_t* b = (_cp_buffer_t*)malloc(sizeof(_cp_buffer_t));
    b->len = 0;
    b->capacity = 0;
    b->data = NULL;
    _cp_buffer_increase_size(b, len);
    return b;
}

static void _cp__cp_buffer_append_char(_cp_buffer_t* buffer, char c) {
    _cp_buffer_increase_size(buffer, 1);
    buffer->data[buffer->len++] = c;
    buffer->data[buffer->len] = 0;
}

static void _cp_buffer_append_string(_cp_buffer_t* buffer, char* str) {
    _cp_buffer_increase_size(buffer, strlen(str));
    strcpy(buffer->data + buffer->len, str);
    buffer->len += strlen(str);
    buffer->data[buffer->len] = 0;
}


static void _cp_colorstack_push(_cp_colorstack_t *stack, _cp_ansi__cp_color_t color) {
    if (stack->ptr >= 16) {
        return;
    }
    stack->stack[stack->ptr++] = color;
}

static _cp_ansi__cp_color_t _cp_colorstack_pop(_cp_colorstack_t *stack) {
    if (stack->ptr <= 0) {
        return RESET;
    }
    return stack->stack[--stack->ptr];
}

static int _cp_parse_command(const char* str, _cp_command_t* cmd) {
    size_t i = 0;
    cmd->valid = 0;
    size_t len = strlen(str);
    if(*str != '[') return 0;

    while(i < len) {
        if(str[i] == ' ') {
            break;
        }
        if(str[i] == ']') {
            cmd->valid = 1;
            break;
        }
        i++;
    }
    if(!cmd->valid) return 0;

    int closing = 0;
    if(len > 1) {
        if(str[1] == '/') {
            cmd->tag = TAG_CLOSE;
            closing = 1;
        }
        else cmd->tag = TAG_OPEN;
    }

    if(i < _CP_MAX_COMMAND_LENGTH) {
        memset(cmd->raw, 0, _CP_MAX_COMMAND_LENGTH);
        memcpy(cmd->raw, str + closing + 1, i - closing - 1);
    }

    cmd->length = strlen(cmd->raw) + 1 + closing;

    return cmd->length;
}

static int _cp_parse_color(const char* str, _cp_color_t* color) {
    if(!strcmp(str, "r") || !strcmp(str, "red")) {
        color->color = RED;
        color->type = FOREGROUND;
    } else if(!strcmp(str, "g") || !strcmp(str, "green")) {
        color->color = GREEN;
        color->type = FOREGROUND;
    } else if(!strcmp(str, "b") || !strcmp(str, "blue")) {
        color->color = BLUE;
        color->type = FOREGROUND;
    } else if(!strcmp(str, "c") || !strcmp(str, "cyan")) {
        color->color = CYAN;
        color->type = FOREGROUND;
    } else if(!strcmp(str, "m") || !strcmp(str, "magenta")) {
        color->color = MAGENTA;
        color->type = FOREGROUND;
    } else if(!strcmp(str, "y") || !strcmp(str, "yellow")) {
        color->color = YELLOW;
        color->type = FOREGROUND;
    } else if(!strcmp(str, "w") || !strcmp(str, "white")) {
        color->color = WHITE;
        color->type = FOREGROUND;
    } else if(!strcmp(str, "lr")) {
        color->color = LIGHTRED;
        color->type = FOREGROUND;
    } else if(!strcmp(str, "lg")) {
        color->color = LIGHTGREEN;
        color->type = FOREGROUND;
    } else if(!strcmp(str, "lb")) {
        color->color = LIGHTBLUE;
        color->type = FOREGROUND;
    } else if(!strcmp(str, "lc")) {
        color->color = LIGHTCYAN;
        color->type = FOREGROUND;
    } else if(!strcmp(str, "lm")) {
        color->color = LIGHTMAGENTA;
        color->type = FOREGROUND;
    } else if(!strcmp(str, "ly")) {
        color->color = LIGHTYELLOW;
        color->type = FOREGROUND;
    } else if(!strcmp(str, "lw")) {
        color->color = LIGHTWHITE;
        color->type = FOREGROUND;
    } else if(!strcmp(str, "br")) {
        color->color = RED;
        color->type = BACKGROUND;
    } else if(!strcmp(str, "bg")) {
        color->color = GREEN;
        color->type = BACKGROUND;
    } else if(!strcmp(str, "bb")) {
        color->color = BLUE;
        color->type = BACKGROUND;
    } else if(!strcmp(str, "bc")) {
        color->color = CYAN;
        color->type = BACKGROUND;
    } else if(!strcmp(str, "bm")) {
        color->color = MAGENTA;
        color->type = BACKGROUND;
    } else if(!strcmp(str, "by")) {
        color->color = YELLOW;
        color->type = BACKGROUND;
    } else if(!strcmp(str, "bw")) {
        color->color = WHITE;
        color->type = BACKGROUND;
    } else if(!strcmp(str, "blr")) {
        color->color = LIGHTRED;
        color->type = BACKGROUND;
    } else if(!strcmp(str, "blg")) {
        color->color = LIGHTGREEN;
        color->type = BACKGROUND;
    } else if(!strcmp(str, "blb")) {
        color->color = LIGHTBLUE;
        color->type = BACKGROUND;
    } else if(!strcmp(str, "blc")) {
        color->color = LIGHTCYAN;
        color->type = BACKGROUND;
    } else if(!strcmp(str, "blm")) {
        color->color = LIGHTMAGENTA;
        color->type = BACKGROUND;
    } else if(!strcmp(str, "bly")) {
        color->color = LIGHTYELLOW;
        color->type = BACKGROUND;
    } else if(!strcmp(str, "blw")) {
        color->color = LIGHTWHITE;
        color->type = BACKGROUND;
    } else {
        return 0;
    }
    return 1;
}

static void _cp_vprintf_color(int enable, const char* fmt, va_list arg) {
    _cp_colorstack_t stack;
    memset(&stack, 0, sizeof(stack));
    _cp_colorstack_t bgstack;
    memset(&bgstack, 0, sizeof(bgstack));

    int i, len = strlen(fmt);
    _cp_buffer_t *fmt_replace = _cp_buffer_create(len);
    _cp_command_t command = {.valid = 0};
    _cp_ansi__cp_color_t color = RESET;
    _cp_ansi__cp_color_t bgcolor = RESET;
    for (i = 0; i < len; i++) {
        i += _cp_parse_command(fmt + i, &command);
        int set_color = 0;
        int set_bgcolor = 0;
        if(command.valid) {
            if(command.tag == TAG_CLOSE) {
                _cp_color_t parsed_color;
                if(_cp_parse_color(command.raw, &parsed_color)) {
                    if(parsed_color.type == FOREGROUND) {
                        set_color = 1;
#if _CP_STRICT
                        if(color != parsed_color.color) {
                            DEBUG("Warning: tag [/%s] does not match!\n", command.raw);
                        } else {
                            color = _cp_colorstack_pop(&stack);
                        }
#else
                        color = _cp_colorstack_pop(&stack);
#endif
                    } else {
                        set_bgcolor = 1;
#if _CP_STRICT
                        if(bgcolor != parsed_color.color) {
                            DEBUG("Warning: tag [/%s] does not match!\n", command.raw);
                        } else {
                            bgcolor = _cp_colorstack_pop(&bgstack);
                        }
#else
                        bgcolor = _cp_colorstack_pop(&bgstack);
#endif
                    }
                } else {
                    command.valid = 0;
                    i -= command.length;
                }
            } else {
                _cp_ansi__cp_color_t last_color = color;
                _cp_ansi__cp_color_t last_bgcolor = bgcolor;
                _cp_color_t parsed_color;
                if(_cp_parse_color(command.raw, &parsed_color)) {
                    if(parsed_color.type == FOREGROUND) {
                        color = parsed_color.color;
                        set_color = 1;
                    } else {
                        bgcolor = parsed_color.color;
                        set_bgcolor = 1;
                    }
                } else {
                    command.valid = 0;
                    i -= command.length;
                }
                if(command.valid) {
                    if(set_color) {
                        _cp_colorstack_push(&stack, last_color);
                    }
                    if(set_bgcolor) {
                        _cp_colorstack_push(&bgstack, last_bgcolor);
                    }
                }
            }
            if(enable && command.valid) {
                if(set_color || set_bgcolor) {
                    char format[32];
                    sprintf(format, "\x1b[%s;%sm", _cp_fg_color_strings[color], _cp_bg_color_strings[bgcolor]);
                    _cp_buffer_append_string(fmt_replace, format);
                }
            }
        }


        if(!command.valid) {
            _cp__cp_buffer_append_char(fmt_replace, fmt[i]);
        }
    }

#if _CP_STRICT
    if(stack.ptr > 0 || bgstack.ptr > 0) {
        DEBUG("Warning: Missing closing tag\n");
        _cp_buffer_append_string(fmt_replace, "\x1b[0m");
    }
#endif

    vfprintf(stdout, fmt_replace->data, arg);
    free(fmt_replace->data);
    free(fmt_replace);

}


void printf_color(int enable, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    _cp_vprintf_color(enable, fmt, ap);
    va_end(ap);
}


static int _cp_spinner_state = 0;
static const char* _cp_spinner_value_0[] = {
    "|",
    "/",
    "-",
    "\\",
    NULL
};

static const char* _cp_spinner_value_1[] = {
  "\x1b(0t\x1b(A",
  "\x1b(0w\x1b(A",
  "\x1b(0u\x1b(A",
  "\x1b(0v\x1b(A",
  NULL
};

static const char* _cp_spinner_value_2[] = {
  "\x1b(0j\x1b(A",
  "\x1b(0m\x1b(A",
  "\x1b(0l\x1b(A",
  "\x1b(0k\x1b(A",
  NULL
};

static const char* _cp_spinner_value_3[] = {
  "\x1b(0m\x1b(A",
  "\x1b(0t\x1b(A",
  "\x1b(0n\x1b(A",
  "\x1b(0u\x1b(A",
  "\x1b(0j\x1b(A",
  NULL
};

static const char* _cp_spinner_value_4[] = {
  "\x1b(0m\x1b(A",
  "\x1b(0x\x1b(A",
  "\x1b(0l\x1b(A",
  "\x1b(0q\x1b(A",
  "\x1b(0k\x1b(A",
  "\x1b(0x\x1b(A",
  "\x1b(0j\x1b(A",
  "\x1b(0q\x1b(A",
  NULL
};

static const char** _cp_spinner_values[] = {
  _cp_spinner_value_0, _cp_spinner_value_1, _cp_spinner_value_2, _cp_spinner_value_3, _cp_spinner_value_4
};

static char** _cp_spinner_value = (char**)_cp_spinner_value_0;

void spinner_start(int color, unsigned int type, const char* fmt, ...) {
    _cp_spinner_value = (char**)_cp_spinner_values[type % (sizeof(_cp_spinner_values) / sizeof(_cp_spinner_values[0]))];
    _cp_spinner_state = 0;
    printf("\x1b[s[%s] ", _cp_spinner_value[_cp_spinner_state]);
    va_list ap;
    va_start(ap, fmt);
    _cp_vprintf_color(color, fmt, ap);
    va_end(ap);
    fflush(stdout);
}


void spinner_update(int color, const char* fmt, ...) {
    _cp_spinner_state++;
    if(!_cp_spinner_value[_cp_spinner_state]) _cp_spinner_state = 0;
    printf("\x1b[u[%s] \x1b[K", _cp_spinner_value[_cp_spinner_state]);
    va_list ap;
    va_start(ap, fmt);
    _cp_vprintf_color(color, fmt, ap);
    va_end(ap);
    fflush(stdout);
}


void spinner_done(int color, const char* fmt, ...) {
    printf("\x1b[u[#] \x1b[K");
    va_list ap;
    va_start(ap, fmt);
    _cp_vprintf_color(color, fmt, ap);
    va_end(ap);
    fflush(stdout);
}


static int _cp_progress_state, _cp_progress_max;
char* _cp_progress_format;

void progress_start(int color, int max, char* fmt) {
    _cp_progress_state = 0;
    _cp_progress_max = max;
    _cp_progress_format = "[g]\x1b(0a\x1b(B[/g]";
    if(fmt != NULL) _cp_progress_format = fmt;
    printf_color(color, "\x1b[s\x1b(0x\x1b(B");
    int i;
    for(i = 0; i < 40; i++) {
        printf(" ");
    }
    printf_color(color, "\x1b(0x\x1b(B %3d%%", 0);
    fflush(stdout);
}


void progress_update(int color) {
    _cp_progress_state++;
    if(_cp_progress_state > _cp_progress_max) {
        _cp_progress_state = _cp_progress_max;
    }
    printf_color(1, "\x1b[u\x1b(0x\x1b(B");
    int i;
    for(i = 0; i < 40 * _cp_progress_state / _cp_progress_max; i++) {
        printf_color(color, _cp_progress_format);
    }
    for(; i < 40; i++) {
        printf(" ");
    }
    printf_color(1, "\x1b(0x\x1b(B %3d%%", 100 * _cp_progress_state / _cp_progress_max);
    if(_cp_progress_state == _cp_progress_max) printf("\n");
    fflush(stdout);
}
