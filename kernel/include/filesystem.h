#pragma once

#include <sys/limits.h>
#include <sys/dirent.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>

#include <time.h>

#include <sys/filesystem.h>
#include <semaphore.h>

#define MAX_FILE 128

// this must be a multiply of 4 and is larger than 5
#define RAMFS_DATANODE_SIZE 512

// should be a multiple of RAMFS_DATANODE_SIZE
#define RAMFS_PIPE_SIZE (RAMFS_DATANODE_SIZE * 1)

typedef enum {
    FS_EMPTY,
    FS_FAT32,
    FS_EXT2,
    FS_RAMFS
} fs_type_t;

// fs_node_t.flags structure:
// lower 4 bits: node type
#define FS_NODE_TYPE_MASK 0b1111
#define FS_NODE_TYPE_FILE 0b0000
#define FS_NODE_TYPE_DIRECTORY 0b0001
// #define FS_NODE_TYPE_SYMLINK 0b0010
#define FS_NODE_TYPE_MEMORY 0b0011
#define FS_NODE_TYPE_PIPE 0b0100
#define FS_NODE_TYPE_SEMAPHORE 0b0101
// higher 28 bits: node flags
#define FS_NODE_FLAG_HIDDEN (1 << 4)
#define FS_NODE_FLAG_MOUNTPOINT (1 << 5)

#define FS_NODE_GET_TYPE(node_ptr) ((node_ptr)->flags & FS_NODE_TYPE_MASK)
#define FS_NODE_GET_FLAGS(node_ptr) ((node_ptr)->flags & (~FS_NODE_TYPE_MASK))

#define FS_NODE_IS_FILE(node_ptr) (FS_NODE_GET_TYPE(node_ptr) == FS_NODE_TYPE_FILE)
#define FS_NODE_IS_DIRECTORY(node_ptr) (FS_NODE_GET_TYPE(node_ptr) == FS_NODE_TYPE_DIRECTORY)
#define FS_NODE_IS_SYMLINK(node_ptr) (FS_NODE_GET_TYPE(node_ptr) == FS_NODE_TYPE_SYMLINK)
#define FS_NODE_IS_PIPE(node_ptr) (FS_NODE_GET_TYPE(node_ptr) == FS_NODE_TYPE_PIPE)
#define FS_NODE_IS_SEMAPHORE(node_ptr) (FS_NODE_GET_TYPE(node_ptr) == FS_NODE_TYPE_SEMAPHORE)
#define FS_NODE_IS_MEMORY(node_ptr) (FS_NODE_GET_TYPE(node_ptr) == FS_NODE_TYPE_MEMORY)

#define FS_NODE_IS_HIDDEN(node_ptr) ((node_ptr)->flags & FS_NODE_FLAG_HIDDEN)
#define FS_NODE_IS_MOUNTPOINT(node_ptr) ((node_ptr)->flags & FS_NODE_FLAG_MOUNTPOINT)

#define FS_NODE_FLAG_SET(node_ptr, flag) ((node_ptr)->flags |= (flag))
#define FS_NODE_FLAG_UNSET(node_ptr, flag) ((node_ptr)->flags &= ~(flag))

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
    semaphore_t* lock;
    union {
        ramfs_datanode_t* datanode_chain; // FS_NODE_TYPE_FILE/DIRECTORY
        void* mem_addr; // FS_NODE_TYPE_MEMORY
        semaphore_t* semaphore; // FS_NODE_TYPE_SEMAPHORE
        struct {
            ramfs_datanode_t* first_datanode;
            ramfs_datanode_t* last_datanode;
            semaphore_t* bytes_available;
            semaphore_t* space_available;
            uint32_t head;
            uint32_t tail;
        } pipe; // FS_NODE_TYPE_PIPE
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

    // if FS_NODE_FLAG_MOUNTPOINT is set
    struct fs_node* mount_target;

    uint16_t creation_milisecond;
    time_t creation_timestamp;
    time_t modified_timestamp;
    time_t accessed_timestamp;
    uint32_t size;

    int32_t refcount;

    semaphore_t lock;

    // fs depended field
    union {
        struct {
            uint32_t start_cluster;
            uint32_t parent_cluster;
            uint32_t parent_cluster_index;
        } fat32;

        struct {
            uint32_t node_addr;
            union {
                // ramfs based objects

                semaphore_t* semaphore;

                struct {
                    semaphore_t* bytes_available;
                    semaphore_t* space_available;
                } pipe;
            };
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

typedef struct file_description {
    fs_node_t* node;
    int mode;

    union {
        int64_t position; // files
        int64_t current_index; // directories (for iteration)
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

typedef struct fs {
    fs_type_t type;
    partition_entry_t partition;
    struct fs_node root_node;

    FS_ERR (*remove_entry)(fs_node_t* parent, fs_node_t* remove_node, bool remove_content);
    void (*make_diriter_adapter)(file_description_t* dir, directory_iterator_t* diriter);
    void (*save_diriter_adapter)(file_description_t* dir, directory_iterator_t* diriter);
    FS_ERR (*setup_directory_iterator)(directory_iterator_t* diriter, fs_node_t* node);
    FS_ERR (*iterate_directory)(directory_iterator_t* diriter, fs_node_t* ret_node);
    FS_ERR (*mkdir)(fs_node_t* parent, const char* name, uint32_t flags, fs_node_t* new_node);
    FS_ERR (*node_create)(fs_node_t* parent, const char* name, uint32_t flags, fs_node_t* new_node);
    FS_ERR (*node_copy)(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, const char* new_name);
    FS_ERR (*node_move)(fs_node_t* node, fs_node_t* new_parent, const char* new_name);
    FS_ERR (*node_reset)(fs_node_t* node);
    FS_ERR (*node_sync)(fs_node_t* node);
    FS_ERR (*file_seek_absolute)(file_description_t* file, int64_t seek_position);
    FS_ERR (*file_read)(file_description_t* file, uint8_t* buffer, size_t size, size_t* actual_read_size);
    FS_ERR (*file_write)(file_description_t* file, const uint8_t* buffer, size_t size, size_t* actual_write_size);
    void (*file_open)(file_description_t* file, fs_node_t* node, const file_mode_t mode);

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
fs_t* vfs_get_ramfs();

// vfs_op.c
FS_ERR vfs_find_and_create_node(const char* path, fs_node_t* cwd, fs_node_t** ret_node, const file_mode_t mode, const uint32_t create_flags);
void vfs_cleanup_node_tree(fs_node_t* start_node);
FS_ERR vfs_remove_node(fs_node_t* parent, fs_node_t* node);
int vfs_open(const char* path, const file_mode_t mode);
void vfs_close(int file_descriptor);
uint32_t vfs_lock(regs_t* regs, int file_descriptor);
void vfs_unlock(int file_descriptor);
int vfs_read(int file_descriptor, uint8_t* buffer, size_t size);
int vfs_write(int file_descriptor, const uint8_t* buffer, size_t size);
void vfs_seek(int file_descriptor, uint32_t hoff, uint32_t loff, whence_t whence, int64_t* position);
int vfs_mount(const char* target_path, const char* mount_path);

// node_op.c
FS_ERR node_setup_directory_iterator(directory_iterator_t* diriter, fs_node_t* node);
FS_ERR node_iterate_directory(directory_iterator_t* diriter, fs_node_t* ret_node);
FS_ERR node_find(fs_node_t* parent, const char* nodename, fs_node_t* ret_node);
FS_ERR node_mkdir(fs_node_t* parent, const char* name, uint32_t flags, fs_node_t* new_node);
FS_ERR node_create(fs_node_t* parent, const char* name, uint32_t flags, fs_node_t* new_node);
FS_ERR node_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, const char* new_name);
FS_ERR node_move(fs_node_t* node, fs_node_t* new_parent, const char* new_name);

FS_ERR node_sync(fs_node_t* node);
FS_ERR node_reset(fs_node_t* node);

// file_op.c
FS_ERR file_setup_directory_iterator(file_description_t* dir);
FS_ERR file_iterate_directory(file_description_t* dir, dirent_t* dirent);
FS_ERR file_open(file_description_t* file, fs_node_t* node, const file_mode_t mode);
FS_ERR file_seek(file_description_t* file, int64_t offset, whence_t whence, int64_t* final_position);
FS_ERR file_write(file_description_t* file, const uint8_t* buffer, size_t size, size_t* actual_write_size);
FS_ERR file_read(file_description_t* file, uint8_t* buffer, size_t size, size_t* actual_read_size);

// ramfs.c
ramfs_datanode_t* ramfs_allocate_datanodes(size_t count, bool clear);
ramfs_datanode_t* ramfs_get_last_datanote_of_chain(ramfs_datanode_t* datanode);
void ramfs_make_diriter_adapter(file_description_t* dir, directory_iterator_t* diriter);
void ramfs_save_diriter_adapter(file_description_t* dir, directory_iterator_t* diriter);
FS_ERR ramfs_setup_directory_iterator(directory_iterator_t* diriter, fs_node_t* node);
FS_ERR ramfs_iterate_directory(directory_iterator_t* diriter, fs_node_t* ret_node);
FS_ERR ramfs_add_entry(fs_node_t* parent, const char* name, void* data, uint32_t flags, size_t size, fs_node_t* new_node);
FS_ERR ramfs_add_memory_entry(fs_node_t* parent, const char* name, void* mem_addr, size_t mem_size, fs_node_t* new_node);
FS_ERR ramfs_create_special_node(fs_node_t* parent, const char* name, uint8_t type, fs_node_t* new_node);
FS_ERR ramfs_remove_entry(fs_node_t* parent, fs_node_t* remove_node, bool remove_content);
FS_ERR ramfs_update_entry(fs_node_t* node);
FS_ERR ramfs_mkdir(fs_node_t* parent, const char* name, uint32_t flags, fs_node_t* new_node);
FS_ERR ramfs_node_create(fs_node_t* parent, const char* name, uint32_t flags, fs_node_t* new_node);
FS_ERR ramfs_node_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, const char* new_name);
FS_ERR ramfs_node_move(fs_node_t* node, fs_node_t* new_parent, const char* new_name);
FS_ERR ramfs_universal_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, const char* new_name);

FS_ERR ramfs_node_reset(fs_node_t* node);
FS_ERR ramfs_file_seek_absolute(file_description_t* file, int64_t seek_position);
FS_ERR ramfs_file_read(file_description_t* file, uint8_t* buffer, size_t size, size_t* actual_read_size);
FS_ERR ramfs_file_write(file_description_t* file, const uint8_t* buffer, size_t size, size_t* actual_write_size);
void ramfs_file_open(file_description_t* file, fs_node_t* node, const file_mode_t mode);

FS_ERR ramfs_init(fs_t* fs);

// fat32.c
uint32_t fat32_allocate_clusters(fs_t* fs, size_t cluster_count, bool clear);
uint32_t fat32_get_last_cluster_of_chain(fs_t* fs, uint32_t start_cluster);

void fat32_make_diriter_adapter(file_description_t* dir, directory_iterator_t* diriter);
void fat32_save_diriter_adapter(file_description_t* dir, directory_iterator_t* diriter);
FS_ERR fat32_setup_directory_iterator(directory_iterator_t* diriter, fs_node_t* node);
FS_ERR fat32_iterate_directory(directory_iterator_t* diriter, fs_node_t* ret_node);
FS_ERR fat32_add_entry(fs_node_t* parent, const char* name, uint32_t start_cluster, uint8_t attr, size_t size, fs_node_t* new_node);
FS_ERR fat32_remove_entry(fs_node_t* parent, fs_node_t* remove_node, bool remove_content);
FS_ERR fat32_update_entry(fs_node_t* node);
FS_ERR fat32_mkdir(fs_node_t* parent, const char* name, uint32_t flags, fs_node_t* new_node);
FS_ERR fat32_node_create(fs_node_t* parent, const char* name, uint32_t flags, fs_node_t* new_node);
FS_ERR fat32_node_copy(fs_node_t* node, fs_node_t* new_parent, fs_node_t* copied, const char* new_name);
FS_ERR fat32_node_move(fs_node_t* node, fs_node_t* new_parent, const char* new_name);

FS_ERR fat32_node_reset(fs_node_t* node);
FS_ERR fat32_file_seek_absolute(file_description_t* file, int64_t seek_position);
FS_ERR fat32_file_read(file_description_t* file, uint8_t* buffer, size_t size, size_t* actual_read_size);
FS_ERR fat32_file_write(file_description_t* file, const uint8_t* buffer, size_t size, size_t* actual_write_size);
void fat32_file_open(file_description_t* file, fs_node_t* node, const file_mode_t mode);

FS_ERR fat32_init(unsigned fsid, partition_entry_t* part);
