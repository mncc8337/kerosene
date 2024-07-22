#pragma once

#include "stdint.h"
#include "stdbool.h"

#include "fat_type.h"

// there are some extra fs that might not be implemented
// i added them for completeness
typedef enum {
    FS_EMPTY,
    FS_FAT_12_16,
    FS_FAT32,
    FS_EXT2,
    FS_EXT3,
    FS_EXT4
} FS_TYPE;

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
} __attribute__((packed)) partition_entry_t; // 16 bytes

typedef struct {
    uint8_t bootstrap[446];
    partition_entry_t partition_entry[4];
    uint16_t boot_signature; // should be 0xaa55
} __attribute__((packed)) MBR_t; // 512 bytes

// // https://en.wikipedia.org/wiki/Master_boot_record
// typedef struct {
//     uint8_t bootstrap_part1[218];
//     uint16_t null1; // 0x0000
//     uint8_t original_physical_drive;
//     uint8_t seconds;
//     uint8_t minutes;
//     uint8_t hours;
//     uint8_t bootstrap_part2[216];
//     uint32_t disk_32bit_signature;
//     uint16_t null2; // 0x0000, 0x5a5a if copy-protected
//     partition_entry_t partition_entry[4];
//     uint16_t boot_signature; // should be 0xaa55
// } __attribute__((packed)) modern_MBR_t; // 512 bytes

typedef struct _fs_t fs_t;
typedef struct _fs_node_t fs_node_t;

#define NODE_READ_ONLY 0x01
#define NODE_HIDDEN    0x02
#define NODE_SYSTEM    0x04
#define NODE_VOLUME_ID 0x08
#define NODE_DIRECTORY 0x10
#define NODE_ARCHIVE   0x20

struct _fs_node_t {
    char name[32];
    bool valid;
    struct _fs_t* fs;
    struct _fs_node_t* parent_node;
    uint32_t start_cluster;
    uint8_t attr;
    uint32_t size;
    uint8_t centisecond;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
};

struct _fs_t {
    FS_TYPE type;
    partition_entry_t partition;
    struct _fs_node_t root_node;
    // the filesystem info table
    // FAT12/16/32: boot record
    uint8_t info_table[512];
};

// mbr.c
bool mbr_load();
partition_entry_t mbr_get_partition_entry(unsigned int id);

// fsmngr.c
FS_TYPE fs_detect(partition_entry_t part);
void fs_add(fs_t fs, int id);
fs_t* fs_get(int id);

// file_op.c
bool fs_list_dir(fs_node_t* parent, bool (*callback)(fs_node_t));
fs_node_t fs_find_node(fs_node_t* parent, const char* path);
bool fs_read_node(fs_node_t* node, uint8_t* buff);

// fat32.c
FAT32_BOOT_RECORD_t fat32_get_bootrec(partition_entry_t part);
uint32_t fat32_total_sectors(FAT32_BOOT_RECORD_t* bootrec);
uint32_t fat32_FAT_size(FAT32_BOOT_RECORD_t* bootrec);
uint32_t fat32_first_data_sector(FAT32_BOOT_RECORD_t* bootrec);
uint32_t fat32_first_FAT_sector(FAT32_BOOT_RECORD_t* bootrec);
uint32_t fat32_total_data_sectors(FAT32_BOOT_RECORD_t* bootrec);
uint32_t fat32_total_clusters(FAT32_BOOT_RECORD_t* bootrec);
void fat32_parse_time(uint16_t time, int* second, int* minute, int* hour);
void fat32_parse_date(uint16_t date, int* day, int* month, int* year);
bool fat32_read_dir(fs_node_t* parent, bool (*callback)(fs_node_t));
void fat32_read_file(fs_node_t* node, uint8_t* buffer);
fs_node_t fat32_init(partition_entry_t part, int id);
