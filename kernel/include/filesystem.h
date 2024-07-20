#pragma once

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"

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

// structure that are parsed using memcpy need attribute packed

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

// https://wiki.osdev.org/FAT

typedef struct {
    uint8_t null[3];
    uint8_t OEM_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t FATs;
    uint16_t rootdir_entries;
    uint16_t total_sectors; // if it is 0 then check large_sector_count
    uint8_t media_type;
    uint16_t sectors_per_FAT;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t large_sector_count;
} __attribute__((packed)) FAT_BPB_t; // 36 bytes

typedef struct {
    uint8_t driver_number;
    uint8_t windows_flags;
    uint8_t signature; // should be 0x28 or 0x29
    uint32_t volume_id_serial;
    uint8_t label[11];
    uint8_t system_identifier[8];
    uint8_t bootcode[448];
    uint16_t boot_signature; // should be 0xaa55
} __attribute__((packed)) FAT_EBPB_t; // 476 bytes

typedef struct {
    uint32_t sectors_per_FAT;
    uint16_t flags;
    uint16_t FAT_version;
    uint32_t rootdir_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_bootsector_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t windows_flags;
    uint8_t signature; // should be 0x28 or 0x29
    uint32_t volume_id_serial;
    uint8_t label[11];
    uint8_t system_identifier[8]; // should be "FAT32   "
    uint8_t bootcode[420];
    uint16_t boot_signature; // should be 0xaa55
} __attribute__((packed)) FAT32_EBPB_t; // 476 bytes

// typedef struct {
//     uint32_t lead_signature; // should be 0x41615252
//     uint8_t reserved1[480];
//     uint32_t mid_signature; // should be 0x61417272
//     uint32_t last_known_free_cluster; // should be range check, if 0xffffffff then compute
//     uint32_t available_clusters_start; // should be range check, if 0xffffffff then start at 2
//     uint8_t reserved2[12];
//     uint32_t trail_signature; // should be 0xaa550000
// } __attribute__((packed)) FAT32_FSINFO_t; // 4096 bytes

typedef struct {
    FAT_BPB_t bpb;
    FAT_EBPB_t ebpb;
} __attribute__((packed)) FAT_BOOT_RECORD_t; // 512 bytes

typedef struct {
    FAT_BPB_t bpb;
    FAT32_EBPB_t ebpb;
} __attribute__((packed)) FAT32_BOOT_RECORD_t; // 512 bytes

typedef struct {
    uint8_t filename[11];
    uint8_t attr;
    uint8_t reserved;
    uint8_t centisecond;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_number_high;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t first_cluster_number_low;
    uint32_t size;
} __attribute__((packed)) FAT_DIRECTORY_ENTRY; // 32 bytes

typedef struct {
    uint8_t order;
    uint16_t chars_1[5];
    uint8_t attr; // should be 0x0f
    uint8_t long_entry_type;
    uint8_t checksum;
    uint16_t chars_2[6];
    uint16_t zero;
    uint16_t chars_3[2];
} __attribute__((packed)) FAT_LFN; // 32 bytes

typedef struct {
    FAT32_BOOT_RECORD_t bootrec;
} __attribute__((packed)) FAT32_INFO_TABLE;

typedef struct {
    char name[32];
    bool valid;
    uint32_t start_cluster;
    uint32_t parent_cluster;
    uint8_t attr;
    uint32_t size;
    uint8_t centisecond;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
} FS_NODE;

typedef struct {
    FS_TYPE type;
    partition_entry_t partition;
    uint8_t info_table[512];
} FILESYSTEM;

// mbr.c
bool mbr_load();
partition_entry_t mbr_get_partition_entry(unsigned int id);

// fsmngr.c
FS_TYPE fs_detect(partition_entry_t part);
void fs_add(FILESYSTEM fs, int id);
FILESYSTEM* fs_get(int id);

// file_op.c
bool file_set_current_dir(FS_NODE node);
void file_set_current_cluster(uint32_t cluster);
bool file_list_dir(FILESYSTEM* fs, bool (*callback)(FS_NODE));
FS_NODE file_open(FILESYSTEM* fs, const char* filename);
bool file_read(FILESYSTEM* fs, FS_NODE node, uint8_t* buff);

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
bool fat32_read_dir(FAT32_BOOT_RECORD_t* bootrec, uint32_t start_cluster, uint32_t sector_offset, bool (*callback)(FS_NODE));
void fat32_read_file(FAT32_BOOT_RECORD_t* bootrec, uint32_t start_cluster, uint32_t sector_offset, uint8_t* buffer);
void fat32_init(partition_entry_t part, int id);
