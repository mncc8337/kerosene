#pragma once

#include <stdint.h>

typedef enum {
    ERR_FS_SUCCESS,
    ERR_FS_FAILED,
    ERR_FS_ENTRY_EXISTED,
    ERR_FS_BAD_CLUSTER,
    ERR_FS_NOT_FOUND,
    ERR_FS_TARGET_NOT_FOUND,
    ERR_FS_DIR_NOT_EMPTY,
    ERR_FS_INVALID_FSINFO,
    ERR_FS_MAX_DEPTH_REACHED,
    ERR_FS_NOT_ENOUGH_SPACE,
    ERR_FS_OOM,
    ERR_FS_NOT_FILE,
    ERR_FS_NOT_DIR,
    ERR_FS_NOT_SUPPORTED,
    ERR_FS_EOF
} FS_ERR;

// file open flags
// ---------------
// open with read permission
#define FILE_OPEN_READ (1 << 0)
// open with write permission
#define FILE_OPEN_WRITE (1 << 1)
// truncate the file if opened with write permission
#define FILE_OPEN_TRUNCATE (1 << 2)
// do create a file if it isn't existed
#define FILE_OPEN_CREATE (1 << 3)
// use with FILE_CREATE to yield an error if the file existed
#define FILE_OPEN_EXCLUSIVE (1 << 4)
// yield an error when open a non-directory node
#define FILE_OPEN_ONLYDIR (1 << 5)
// open with append mode
#define FILE_OPEN_APPEND (1 << 6)
// container type
typedef uint16_t file_mode_t;

typedef enum {
    SEEK_ABSOLUTE = 0,
    SEEK_RELATIVE = 1,
    SEEK_BOTTOM = 2,
} whence_t;
