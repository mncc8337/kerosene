#pragma once

#include <sys/limits.h>
#include <stdint.h>

#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10 

typedef struct dirent {
    uint32_t d_ino;
    uint8_t d_type;
    char d_name[FILENAME_LIMIT];
} dirent_t;
