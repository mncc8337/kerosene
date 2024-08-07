#pragma once

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"

#include "fat_type.h"

// including the null char
// the limit is 256
#define FILENAME_LIMIT 64

typedef enum {
    ERR_FS_FAILED,
    ERR_FS_SUCCESS,
    ERR_FS_BAD_CLUSTER,
    ERR_FS_NOT_FOUND,
    ERR_FS_DIR_NOT_EMPTY,
    ERR_FS_CALLBACK_STOP,
    ERR_FS_EXIT_NATURALLY,
    ERR_FS_INVALID_FSINFO,
    ERR_FS_NOT_FILE,
    ERR_FS_NOT_DIR,
    ERR_FS_UNKNOWN_FS,
    ERR_FS_EOF
} FS_ERR;

typedef enum {
    FS_EMPTY,
    FS_FAT32,
    FS_EXT2
} fs_type_t;

typedef enum {
    FILE_WRITE,
    FILE_APPEND,
    FILE_READ
} FILE_OP;

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
} __attribute__((packed)) mbr_t; // 512 bytes

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

struct _fs_node_t {
    char name[FILENAME_LIMIT];
    bool valid;
    struct _fs_t* fs;
    struct _fs_node_t* parent_node;
    uint32_t start_cluster;
    bool isdir;
    bool hidden;
    uint32_t size;
    uint8_t centisecond;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    // filesystem infomation
    // may change when i add support for other filesystem
    uint32_t parent_cluster;
    unsigned int parent_cluster_index;
};

struct _fs_t {
    fs_type_t type;
    partition_entry_t partition;
    struct _fs_node_t root_node;
    // the filesystem info table 1
    // FAT12/16/32: boot record
    uint8_t info_table1[512];
    // the filesystem info table 2
    // FAT12/16/32: FS info structure
    uint8_t info_table2[512];
};

typedef struct {
    fs_node_t* node;
    bool valid;
    int mode;
    unsigned int position;
    uint32_t current_cluster;
    // TODO: add more thing here
} FILE;

// mbr.c
bool mbr_load();
partition_entry_t mbr_get_partition_entry(unsigned int id);

// fsmngr.c
fs_type_t fs_detect(partition_entry_t part);
void fs_add(fs_t fs, int id);
fs_t* fs_get(int id);

// file_op.c
FS_ERR fs_list_dir(fs_node_t* parent, bool (*callback)(fs_node_t));
fs_node_t fs_find(fs_node_t* parent, const char* path);
fs_node_t fs_mkdir(fs_node_t* parent, char* name);
fs_node_t fs_touch(fs_node_t* parent, char* name);
FS_ERR fs_rm(fs_node_t* node, fs_node_t delete_node);
FS_ERR fs_rm_recursive(fs_node_t*  parent, fs_node_t delete_node);
FS_ERR fs_move(fs_node_t* node, fs_node_t* new_parent, char* new_name);
FS_ERR fs_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, char* new_name);
FS_ERR fs_copy_recursive(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, char* new_name);

FILE file_open(fs_node_t* node, int mode);
FS_ERR file_write(FILE* file, uint8_t* data, size_t size);
FS_ERR file_read(FILE* file, uint8_t* buffer, size_t size);
FS_ERR file_close(FILE* file);

// fat32.c
void fat32_get_bootrec(partition_entry_t part, uint8_t* bootrec);
void fat32_update_bootrecord(fs_t* fs);
void fat32_get_fsinfo(partition_entry_t part, fat32_bootrecord_t* bootrec, uint8_t* fsinfo);
void fat32_update_fsinfo(fs_t* fs);
uint32_t fat32_total_sectors(fat32_bootrecord_t* bootrec);
uint32_t fat32_FAT_size(fat32_bootrecord_t* bootrec);
uint32_t fat32_first_data_sector(fat32_bootrecord_t* bootrec);
uint32_t fat32_first_FAT_sector(fat32_bootrecord_t* bootrec);
uint32_t fat32_total_data_sectors(fat32_bootrecord_t* bootrec);
uint32_t fat32_total_clusters(fat32_bootrecord_t* bootrec);

uint32_t fat32_allocate_clusters(fs_t* fs, size_t cluster_count);
FS_ERR fat32_free_cluster_chain(fs_t* fs, uint32_t start_cluster);
uint32_t fat32_expand_cluster_chain(fs_t* fs, uint32_t end_cluster, size_t cluster_count);
FS_ERR fat32_cut_cluster_chain(fs_t* fs, uint32_t start_cluster);
uint32_t fat32_get_last_cluster_of_chain(fs_t* fs, uint32_t start_cluster);
uint32_t fat32_copy_cluster_chain(fs_t* fs, uint32_t start_cluster);

void fat32_parse_time(uint16_t time, int* second, int* minute, int* hour);
void fat32_parse_date(uint16_t date, int* day, int* month, int* year);

FS_ERR fat32_read_dir(fs_node_t* parent, bool (*callback)(fs_node_t));
FS_ERR fat32_read_file(fs_t* fs, uint32_t* start_cluster, uint8_t* buffer, size_t size, int cluster_offset);
FS_ERR fat32_write_file(fs_t* fs, uint32_t* start_cluster, uint8_t* buffer, size_t size, int cluster_offset);

fs_node_t fat32_add_entry(fs_node_t* parent, char* name, uint32_t start_cluster, uint8_t attr, size_t size);
FS_ERR fat32_remove_entry(fs_node_t* parent, fs_node_t remove_node, bool remove_content);
FS_ERR fat32_update_entry(fs_node_t* node);
fs_node_t fat32_mkdir(fs_node_t* parent, char* name, uint32_t start_cluster, uint8_t attr);

FS_ERR fat32_save_entry();

FS_ERR fat32_init(partition_entry_t part, int id);
