#pragma once

#include <sys/limits.h>
#include <sys/dirent.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>

#include <time.h>

#define MAX_FS 32
#define MAX_DISK_ID_STRLEN 2
#define RAMFS_DISK (MAX_FS-1)

#define MAX_FILE 128

// this must be a multiply of 4 and is larger than 5
#define RAMFS_DATANODE_SIZE 512

typedef enum {
    FS_EMPTY,
    FS_FAT32,
    FS_EXT2,
    FS_RAMFS
} fs_type_t;

#define FS_FLAG_DIRECTORY (1 << 0)
#define FS_FLAG_HIDDEN (1 << 1)
#define FS_FLAG_PIPE (1 << 2)
#define FS_FLAG_MEMORY (1 << 3)

#define FS_NODE_IS_VALID(node_ptr) ((node_ptr)->flags != 0)
#define FS_NODE_IS_DIR(node_ptr) ((node_ptr)->flags & FS_FLAG_DIRECTORY)
#define FS_NODE_IS_HIDDEN(node_ptr) ((node_ptr)->flags & FS_FLAG_HIDDEN)
#define FS_NODE_IS_PIPE(node_ptr) ((node_ptr)->flags & FS_FLAG_PIPE)

#define FS_NODE_FLAG_SET(node_ptr, flag) ((node_ptr)->flags |= (flag))
#define FS_NODE_FLAG_UNSET(node_ptr, flag) ((node_ptr)->flags &= ~(flag))

#include <sys/filesystem.h>

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

typedef uint32_t ramfs_datanode_entry_t;

typedef struct ramfs_datanode {
    struct ramfs_datanode* next;
    uint8_t data[RAMFS_DATANODE_SIZE];
} ramfs_datanode_t;

typedef struct {
    uint32_t size;
    uint16_t creation_milisecond;
    time_t creation_timestamp;
    time_t modified_timestamp;
    time_t accessed_timestamp;
    uint32_t name_length;
    uint32_t flags;
    union {
        ramfs_datanode_t* datanode_chain; // standard files/directories
        void* mem_addr; // FS_FLAG_MEMORY nodes
    };
} ramfs_node_t;

#include <fat_type.h>

typedef struct fs_node {
    char name[FILENAME_LIMIT];
    struct fs* fs;
    struct fs_node* parent;
    struct fs_node* children;
    struct fs_node* next_sibling;
    uint32_t flags;
    uint16_t creation_milisecond;
    time_t creation_timestamp;
    time_t modified_timestamp;
    time_t accessed_timestamp;
    uint32_t size;

    int32_t refcount;

    // fs depended field
    union {
        struct {
            uint32_t start_cluster;
            uint32_t parent_cluster;
            uint32_t parent_cluster_index;
        } fat32;

        struct {
            uint32_t node_addr;
        } ramfs;
    };
} fs_node_t;

typedef struct {
    fs_node_t* node;
    int32_t current_index;

    // fs depended field
    union {
        struct {
            uint32_t current_cluster;
        } fat32;
        struct {
            uint32_t current_datanode;
        } ramfs;
    };
} directory_iterator_t;

typedef struct fs {
    fs_type_t type;
    partition_entry_t partition;
    struct fs_node root_node;

    // fs-depend field
    union {
        struct {
            uint8_t infotable[1024];
        } info;

        struct {
            fat32_bootrecord_t bootrec;
            fat32_fsinfo_t fsinfo;
        } fat32_info;
    };
} fs_t;

typedef struct {
    fs_node_t* node;
    int mode;

    union {
        int64_t position; // files
        int32_t current_index; // directories (for iteration)
    };

    // fs depended field
    union {
        struct {
            uint32_t current_cluster;
            uint32_t last_cluster;
        } fat32;
        struct {
            uint32_t current_datanode;
            uint32_t last_datanode;
        } ramfs;
    };

    // TODO: add more thing here
} file_description_t;

// mbr.c
bool mbr_load();
partition_entry_t mbr_get_partition_entry(unsigned int id);

// vfs.c
bool vfs_init();
fs_node_t* vfs_get_dev();
fs_node_t* vfs_get_stdout();
fs_node_t* vfs_get_stdin();
fs_node_t* vfs_get_proc_dir();
fs_type_t vfs_detectfs(partition_entry_t* part);
fs_t* vfs_getfs(int id);
bool vfs_is_fs_available(int id);
file_description_t* vfs_get_kernel_file_descriptor_table();
unsigned vfs_get_kernel_file_count();

// vfs_op.c
FS_ERR vfs_find_and_create_node(const char* path, fs_node_t* cwd, fs_node_t** ret_node, const file_mode_t mode, const bool is_file);
void vfs_cleanup_node_tree(fs_node_t* start_node);
int vfs_open(const char* path, const file_mode_t mode);
void vfs_close(int file_descriptor);
int vfs_read(int file_descriptor, uint8_t* buffer, size_t size);
int vfs_write(int file_descriptor, const uint8_t* buffer, size_t size);
int64_t vfs_seek(int file_descriptor, int64_t offset, whence_t whence);
void vfs_seek_syscall(int file_descriptor, uint32_t hoff, uint32_t loff, whence_t whence, int64_t* position);

// fs_op.c
FS_ERR node_setup_directory_iterator(directory_iterator_t* diriter, fs_node_t* node);
FS_ERR node_iterate_directory(directory_iterator_t* diriter, fs_node_t* ret_node);
FS_ERR node_find(fs_node_t* parent, const char* nodename, fs_node_t* ret_node);
FS_ERR node_mkdir(fs_node_t* parent, const char* name, fs_node_t* new_node);
FS_ERR node_touch(fs_node_t* parent, const char* name, fs_node_t* new_node);
FS_ERR node_remove(fs_node_t* parent, fs_node_t* node);
FS_ERR node_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, const char* new_name);
FS_ERR node_move(fs_node_t* node, fs_node_t* new_parent, const char* new_name);

// file_op.c
FS_ERR file_setup_directory_iterator(file_description_t* dir);
FS_ERR file_iterate_directory(file_description_t* dir, dirent_t* dirent);
FS_ERR file_reset(file_description_t* file);
FS_ERR file_open(file_description_t* file, fs_node_t* node, const file_mode_t mode);
FS_ERR file_seek(file_description_t* file, int64_t offset, whence_t whence, int64_t* final_position);
FS_ERR file_write(file_description_t* file, const uint8_t* buffer, size_t size, size_t* actual_write_size);
FS_ERR file_read(file_description_t* file, uint8_t* buffer, size_t size, size_t* actual_read_size);
FS_ERR file_sync(file_description_t* file);

// ramfs.c
ramfs_datanode_t* ramfs_allocate_datanodes(size_t count, bool clear);
ramfs_datanode_t* ramfs_get_last_datanote_of_chain(ramfs_datanode_t* datanode);
FS_ERR ramfs_setup_directory_iterator(directory_iterator_t* diriter, fs_node_t* node);
FS_ERR ramfs_iterate_directory(directory_iterator_t* diriter, fs_node_t* ret_node);
FS_ERR ramfs_add_entry(fs_node_t* parent, const char* name, ramfs_datanode_t* datanode_chain, uint32_t flags, size_t size, fs_node_t* new_node);
FS_ERR ramfs_add_memory_entry(fs_node_t* parent, const char* name, void* mem_addr, size_t mem_size, fs_node_t* new_node);
FS_ERR ramfs_remove_entry(fs_node_t* parent, fs_node_t* remove_node, bool remove_content);
FS_ERR ramfs_update_entry(fs_node_t* node);
FS_ERR ramfs_mkdir(fs_node_t* parent, const char* name, uint32_t flags, fs_node_t* new_node);
FS_ERR ramfs_move(fs_node_t* node, fs_node_t* new_parent, const char* new_name);
FS_ERR ramfs_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, const char* new_name);
FS_ERR ramfs_universal_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, const char* new_name);

FS_ERR ramfs_file_reset(fs_node_t* node);
FS_ERR ramfs_pipe_read(file_description_t* file, uint8_t* buffer, size_t size, size_t* actual_read_size);
FS_ERR ramfs_seek_absolute(file_description_t* file, int64_t seek_position);
FS_ERR ramfs_read(file_description_t* file, uint8_t* buffer, size_t size, size_t* actual_read_size);
FS_ERR ramfs_write(file_description_t* file, const uint8_t* buffer, size_t size, size_t* actual_write_size);

FS_ERR ramfs_init(fs_t* fs);

// fat32.c
uint32_t fat32_allocate_clusters(fs_t* fs, size_t cluster_count, bool clear);
uint32_t fat32_get_last_cluster_of_chain(fs_t* fs, uint32_t start_cluster);

FS_ERR fat32_setup_directory_iterator(directory_iterator_t* diriter, fs_node_t* node);
FS_ERR fat32_iterate_directory(directory_iterator_t* diriter, fs_node_t* ret_node);
FS_ERR fat32_add_entry(fs_node_t* parent, const char* name, uint32_t start_cluster, uint8_t attr, size_t size, fs_node_t* new_node);
FS_ERR fat32_remove_entry(fs_node_t* parent, fs_node_t* remove_node, bool remove_content);
FS_ERR fat32_update_entry(fs_node_t* node);
FS_ERR fat32_mkdir(fs_node_t* parent, const char* name, uint8_t attr, fs_node_t* new_node);
FS_ERR fat32_move(fs_node_t* node, fs_node_t* new_parent, const char* new_name);
FS_ERR fat32_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, const char* new_name);

FS_ERR fat32_file_reset(fs_node_t* node);
FS_ERR fat32_seek_absolute(file_description_t* file, int64_t seek_position);
FS_ERR fat32_read(file_description_t* file, uint8_t* buffer, size_t size, size_t* actual_read_size);
FS_ERR fat32_write(file_description_t* file, const uint8_t* buffer, size_t size, size_t* actual_write_size);

FS_ERR fat32_init(fs_t* fs, partition_entry_t part);
