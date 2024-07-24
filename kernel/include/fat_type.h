#include "stdint.h"

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
} __attribute__((packed)) fat_bpb_t; // 36 bytes

typedef struct {
    uint8_t driver_number;
    uint8_t windows_flags;
    uint8_t signature; // should be 0x28 or 0x29
    uint32_t volume_id_serial;
    uint8_t label[11];
    uint8_t system_identifier[8];
    uint8_t bootcode[448];
    uint16_t boot_signature; // should be 0xaa55
} __attribute__((packed)) fat_ebpb_t; // 476 bytes

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
} __attribute__((packed)) fat32_ebpb_t; // 476 bytes

typedef struct {
    uint32_t lead_signature; // should be 0x41615252
    uint8_t reserved1[480];
    uint32_t mid_signature; // should be 0x61417272
    uint32_t free_cluster_count; // should be range check, if 0xffffffff then compute
    uint32_t available_clusters_start; // should be range check, if 0xffffffff then start at 2
    uint8_t reserved2[12];
    uint32_t trail_signature; // should be 0xaa550000
} __attribute__((packed)) fat32_fsinfo_t; // 512 bytes

typedef struct {
    fat_bpb_t bpb;
    fat_ebpb_t ebpb;
} __attribute__((packed)) fat_bootrecord_t; // 512 bytes

typedef struct {
    fat_bpb_t bpb;
    fat32_ebpb_t ebpb;
} __attribute__((packed)) fat32_bootrecord_t; // 512 bytes

typedef struct {
    uint8_t name[11];
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
} __attribute__((packed)) fat_directory_entry_t; // 32 bytes

typedef struct {
    uint8_t order;
    uint16_t chars_1[5];
    uint8_t attr; // should be 0x0f
    uint8_t long_entry_type;
    uint8_t checksum;
    uint16_t chars_2[6];
    uint16_t zero;
    uint16_t chars_3[2];
} __attribute__((packed)) fat_lfn_entry_t; // 32 bytes

