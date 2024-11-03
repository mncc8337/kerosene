#pragma once

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdatomic.h"

#include "time.h"

#include "fat_type.h"

// filename limit including null char
#define FILENAME_LIMIT 256

#define MAX_DISK 5

#define FS_FLAG_VALID 1
#define FS_FLAG_DIRECTORY 2
#define FS_FLAG_HIDDEN 4

#define FS_NODE_IS_VALID(node) ((node).flags & FS_FLAG_VALID)
#define FS_NODE_IS_DIR(node) ((node).flags & FS_FLAG_DIRECTORY)
#define FS_NODE_IS_HIDDEN(node) ((node).flags & FS_FLAG_HIDDEN)

#define FS_NODE_FLAG_SET(node, flag) ((node)->flags |= flag)
#define FS_NODE_FLAG_UNSET(node, flag) ((node)->flags &= ~flag)

typedef enum {
    ERR_FS_SUCCESS,
    ERR_FS_FAILED,
    ERR_FS_BAD_CLUSTER,
    ERR_FS_NOT_FOUND,
    ERR_FS_DIR_NOT_EMPTY,
    ERR_FS_CALLBACK_STOP,
    ERR_FS_EXIT_NATURALLY,
    ERR_FS_INVALID_FSINFO,
    ERR_FS_NOT_FILE,
    ERR_FS_NOT_DIR,
    ERR_FS_UNKNOWN_FS,
    ERR_FS_IN_USE,
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

typedef struct fs_node {
    char name[FILENAME_LIMIT];
    struct fs* fs;
    struct fs_node* parent_node;
    uint32_t start_cluster;
    uint32_t flags;
    uint16_t creation_milisecond;
    time_t creation_timestamp;
    time_t modified_timestamp;
    time_t accessed_timestamp;
    uint32_t size;
    // filesystem infomation
    // may change when i add support for other filesystem
    uint32_t parent_cluster;
    unsigned int parent_cluster_index;
} fs_node_t;

typedef struct fs {
    fs_type_t type;
    partition_entry_t partition;
    struct fs_node root_node;

    // type-depend field
    union {
        struct {
            uint8_t infotable1[512];
            uint8_t infotable2[512];
        } info;
        struct {
            fat32_bootrecord_t bootrec;
            fat32_fsinfo_t fsinfo;
        } fat32_info;
    };
} fs_t;

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
bool fs_mngr_init();
fs_type_t fs_detect(partition_entry_t part);
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
uint32_t fat32_allocate_clusters(fs_t* fs, size_t cluster_count);
FS_ERR fat32_free_cluster_chain(fs_t* fs, uint32_t start_cluster);
uint32_t fat32_expand_cluster_chain(fs_t* fs, uint32_t end_cluster, size_t cluster_count);
FS_ERR fat32_cut_cluster_chain(fs_t* fs, uint32_t start_cluster);
uint32_t fat32_get_last_cluster_of_chain(fs_t* fs, uint32_t start_cluster);
uint32_t fat32_copy_cluster_chain(fs_t* fs, uint32_t start_cluster);

FS_ERR fat32_read_dir(fs_node_t* parent, bool (*callback)(fs_node_t));
FS_ERR fat32_read_file(fs_t* fs, uint32_t* start_cluster, uint8_t* buffer, size_t size, int cluster_offset);
FS_ERR fat32_write_file(fs_t* fs, uint32_t* start_cluster, uint8_t* buffer, size_t size, int cluster_offset);

fs_node_t fat32_add_entry(fs_node_t* parent, char* name, uint32_t start_cluster, uint8_t attr, size_t size);
FS_ERR fat32_remove_entry(fs_node_t* parent, fs_node_t remove_node, bool remove_content);
FS_ERR fat32_update_entry(fs_node_t* node);
fs_node_t fat32_mkdir(fs_node_t* parent, char* name, uint32_t start_cluster, uint8_t attr);

FS_ERR fat32_init(partition_entry_t part, int id);
