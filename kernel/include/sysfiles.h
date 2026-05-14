#pragma once

// vfs paths for kernel-managed system files
// only the kernel uses these paths directly
// user processes access them via inherited file descriptors

#define SYSFILE_PATH_DEV    "/dev"
#define SYSFILE_PATH_STDIN  "/dev/stdin"
#define SYSFILE_PATH_STDOUT "/dev/stdout"
#define SYSFILE_PATH_PROC   "/proc"

// predefined file descriptor slots in the kernel FDT
// and inherited by every user process at creation
#define SYSFILE_FD_STDIN  0
#define SYSFILE_FD_STDOUT 1
