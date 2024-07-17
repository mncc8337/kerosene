#pragma once

#include "stdint.h"

#define FS_FILE      0
#define FS_DIRECTORY 1
#define FS_INVALID   2

#define FS_MAXDEV 26

typedef struct {
} MBR_T;

typedef struct {
    char name[32];
    uint32_t flags;
    uint32_t fileLength;
    uint32_t id;
    uint32_t eof;
    uint32_t position;
    uint32_t current_cluster;
    uint32_t device;
} FILE;

typedef struct {
    char name[8];
    FILE (*directory)(const char* dirname);
    void (*mount)();
    void (*read)(FILE* file, unsigned char* buffer, unsigned int length);
    void (*close)(FILE*);
    FILE (*open)(const char* fname);
} FILESYSTEM;

// mbr.c
void load_mbr();

// file_op.c
FILE open_file(const char* fname);
void read_file(FILE* file, unsigned char* buffer, unsigned int length);
void close_file(FILE* file);
void register_fs(FILESYSTEM* fs, unsigned int device_id);
void unregister_fs(FILESYSTEM fs);
void unregister_fs_id(unsigned int device_id);

// device list
FILESYSTEM* __FS__[FS_MAXDEV];
