#pragma once

#include "stdint.h"
#include "stdbool.h"

#define FS_FILE      0
#define FS_DIRECTORY 1
#define FS_INVALID   2

#define FS_MAXDEV 26

typedef struct {
    uint8_t drive_attribute;
    uint8_t CHS_start_low;
    uint8_t CHS_start_mid;
    uint8_t CHS_start_high;
    uint8_t partition_type;
    uint8_t CHS_end_low;
    uint8_t CHS_end_mid;
    uint8_t CHS_end_high;
    uint32_t LBA_start;
    uint32_t sector_count;
} __attribute__((packed)) partition_entry_t;

// structure of a mordern standard MBR
// https://en.wikipedia.org/wiki/Master_boot_record
typedef struct {
    uint8_t bootstrap_part1[218];
    uint16_t null1; // 0x0000
    uint8_t original_physical_drive;
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t bootstrap_part2[216];
    uint32_t disk_32bit_signature;
    uint16_t null2; // 0x0000, 0x5a5a if copy-protected
    partition_entry_t partition_entry[4];
    uint16_t boot_signature;
} __attribute__((packed)) MBR_t;

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

// typedef struct {
//     char name[8];
//     FILE (*directory)(const char* dirname);
//     void (*mount)();
//     void (*read)(FILE* file, unsigned char* buffer, unsigned int length);
//     void (*close)(FILE*);
//     FILE (*open)(const char* fname);
// } FILESYSTEM;

// mbr.c
bool mbr_load();
partition_entry_t mbr_get_partition_entry(unsigned int id);

// file_op.c
