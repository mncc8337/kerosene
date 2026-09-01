#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

static inline bool is_space(int c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\v' || c == '\f';
}

typedef struct {
    int fd;
    int ungot_char;
} scan_stream_t;

static int stream_getc(scan_stream_t* s) {
    if(s->ungot_char != EOF) {
        int c = s->ungot_char;
        s->ungot_char = EOF;
        return c;
    }
    unsigned char c;
    int r = read(s->fd, &c, 1);
    if(r <= 0) return EOF;
    return c;
}

static void stream_ungetc(scan_stream_t* s, int c) {
    s->ungot_char = c;
}

int vdscanf(int fd, const char* restrict format, va_list parameters) {
    scan_stream_t stream = { fd, EOF };
    int assigned = 0;
    
    while(*format != '\0') {
        if(is_space(*format)) {
            format++;
            int c;
            while(is_space(c = stream_getc(&stream))) { }
            if(c != EOF) stream_ungetc(&stream, c);
            continue;
        }
        
        if(*format != '%') {
            int c = stream_getc(&stream);
            if(c != *format) {
                if(c != EOF) stream_ungetc(&stream, c);
                break;
            }
            format++;
            continue;
        }
        
        format++; // skip '%'
        if(*format == '%') {
            int c = stream_getc(&stream);
            if(c != '%') {
                if(c != EOF) stream_ungetc(&stream, c);
                break;
            }
            format++;
            continue;
        }
        
        // formats: c, s, d, x
        // for s, d, x we should skip leading whitespace first.
        if(*format == 's' || *format == 'd' || *format == 'x') {
            int c;
            while(is_space(c = stream_getc(&stream))) { }
            if(c != EOF) stream_ungetc(&stream, c);
        }
        
        if(*format == 'c') {
            int c = stream_getc(&stream);
            if(c == EOF) break;
            char* ptr = va_arg(parameters, char*);
            if(ptr) *ptr = (char)c;
            assigned++;
            format++;
        } else if(*format == 's') {
            char* ptr = va_arg(parameters, char*);
            int c;
            int count = 0;
            while((c = stream_getc(&stream)) != EOF && !is_space(c)) {
                if(ptr) ptr[count] = (char)c;
                count++;
            }
            if(c != EOF) stream_ungetc(&stream, c);
            if(ptr) ptr[count] = '\0';
            if(count == 0 && c == EOF) break;
            assigned++;
            format++;
        } else if(*format == 'd') {
            int c = stream_getc(&stream);
            if(c == EOF) break;
            bool neg = false;
            if(c == '-') {
                neg = true;
                c = stream_getc(&stream);
            } else if(c == '+') {
                c = stream_getc(&stream);
            }
            
            if(c < '0' || c > '9') {
                if(c != EOF) stream_ungetc(&stream, c);
                break;
            }
            
            int num = 0;
            while(c >= '0' && c <= '9') {
                num = num * 10 + (c - '0');
                c = stream_getc(&stream);
            }
            if(c != EOF) stream_ungetc(&stream, c);
            
            int* ptr = va_arg(parameters, int*);
            if(ptr) *ptr = neg ? -num : num;
            assigned++;
            format++;
        } else if(*format == 'x') {
            int c = stream_getc(&stream);
            if(c == EOF) break;
            
            if(c == '0') {
                int next = stream_getc(&stream);
                if(next == 'x' || next == 'X') {
                    c = stream_getc(&stream);
                } else {
                    stream_ungetc(&stream, next);
                }
            }
            
            bool valid = false;
            unsigned int num = 0;
            while(true) {
                if(c >= '0' && c <= '9') { num = num * 16 + (c - '0'); valid = true; }
                else if(c >= 'a' && c <= 'f') { num = num * 16 + (c - 'a' + 10); valid = true; }
                else if(c >= 'A' && c <= 'F') { num = num * 16 + (c - 'A' + 10); valid = true; }
                else break;
                c = stream_getc(&stream);
            }
            if(c != EOF) stream_ungetc(&stream, c);
            
            if(!valid) break;
            
            unsigned int* ptr = va_arg(parameters, unsigned int*);
            if(ptr) *ptr = num;
            assigned++;
            format++;
        } else {
            // unknown format, just break
            break;
        }
    }
    
    return assigned;
}
