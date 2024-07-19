#include "filesystem.h"
#include "ata.h"

#include "string.h"
#include "debug.h"

FAT32_BOOT_RECORD_t fat32_get_bootrec(partition_entry_t part) {
    FAT32_BOOT_RECORD_t bootrec;
    ata_pio_LBA28_access(true, part.LBA_start, 1, (uint8_t*)(&bootrec));
    return bootrec;
}

uint32_t fat32_total_sectors(FAT32_BOOT_RECORD_t* bootrec) {
    return bootrec->bpb.total_sectors == 0 ? bootrec->bpb.large_sector_count : bootrec->bpb.total_sectors;
}

uint32_t fat32_FAT_size(FAT32_BOOT_RECORD_t* bootrec) {
    return bootrec->bpb.sectors_per_FAT == 0 ? bootrec->ebpb.sectors_per_FAT : bootrec->bpb.sectors_per_FAT;
}

uint32_t fat32_first_data_sector(FAT32_BOOT_RECORD_t* bootrec) {
    return bootrec->bpb.reserved_sectors + bootrec->bpb.FATs * fat32_FAT_size(bootrec);
}

uint32_t fat32_first_FAT_sector(FAT32_BOOT_RECORD_t* bootrec) {
    return bootrec->bpb.reserved_sectors;
}

uint32_t fat32_total_data_sectors(FAT32_BOOT_RECORD_t* bootrec) {
    return fat32_total_sectors(bootrec) - fat32_first_data_sector(bootrec);
}

uint32_t fat32_total_clusters(FAT32_BOOT_RECORD_t* bootrec) {
    return fat32_total_data_sectors(bootrec) / bootrec->bpb.sectors_per_cluster;
}

void fat32_parse_time(uint16_t time, int* second, int* minute, int* hour) {
    *hour = (time >> 11);
    *minute = (time >> 5) & 0b111111;
    *second = (time & 0b11111) * 2;
}
void fat32_parse_date(uint16_t date, int* day, int* month, int* year) {
    *day = date & 0b11111;
    *month = (date >> 5) & 0b1111;
    // NOTE: wrong year: 24 yields 44, 12 yeilds 32, 95 yields 15
    // seem like we just need to subtract 20
    // added 100 to prevent it from being negative
    *year = ((date >> 9) - 20 + 100) % 100;
}

void fat32_read_dir(FAT32_BOOT_RECORD_t* bootrec, uint32_t start_cluster, uint32_t sector_offset) {
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = fat32_first_data_sector(bootrec);
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);

    // cluster size in byte
    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;
    uint8_t directory[cluster_size];

    uint8_t FAT[bootrec->bpb.bytes_per_sector];

    bool finish = false;
    while(!finish) {
        uint32_t first_sector = ((start_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(true, sector_offset + first_sector, sectors_per_cluster, directory);

        bool lfn_buffer_ready = false;
        FAT_LFN temp_lfn;
        FAT_DIRECTORY temp_dir;
        for(unsigned int i = 0; i < cluster_size; i += 32) {
            if(directory[i] == 0x00) { // no more file/directory in this dir
                finish = true;
                break;
            }
            if(directory[i] == 0xe5) continue; // unused entry
            // check if it is long file name entry
            if(directory[i+11] == 0x0f) {
                memcpy(&temp_lfn, directory + i, 32);
                lfn_buffer_ready = true;

                // print its name
                for(int j = 1; j < 32; j++)
                    putchar(directory[i+j]);
                putchar('\n');

                continue;
            }
            memcpy(&temp_dir, directory + i, 32);
            if(lfn_buffer_ready) {
                // apply the long file name to the dir just read
                // will figure it out later
                lfn_buffer_ready = false;

                // print attr
                if(temp_dir.attr & 0x10)
                    puts("    directory");
                else {
                    puts("    file");
                    printf("    size: %d bytes\n", temp_dir.size);
                }
                int day, month, year;
                int second, minute, hour;
                fat32_parse_date(temp_dir.creation_date, &day, &month, &year);
                fat32_parse_time(temp_dir.creation_time, &second, &minute, &hour);
                printf("    create at %d/%d/%d, ", day, month, year);
                printf("%d:%d:%d.%d\n", hour, minute, second, temp_dir.centisecond);

                fat32_parse_date(temp_dir.last_access_date, &day, &month, &year);
                printf("    last access %d/%d/%d\n", day, month, year);

                fat32_parse_date(temp_dir.last_mod_date, &day, &month, &year);
                fat32_parse_time(temp_dir.last_mod_time, &second, &minute, &hour);
                printf("    last modification %d/%d/%d, ", day, month, year);
                printf("%d:%d:%d\n", hour, minute, second);
            }
        }

        // get next cluster
        int FAT_offset = start_cluster * 4;
        int FAT_sector = first_FAT_sector + FAT_offset / bootrec->bpb.bytes_per_sector;
        ata_pio_LBA28_access(true, sector_offset + FAT_sector, 1, FAT);
        int entry_offset = FAT_offset % bootrec->bpb.bytes_per_sector;

        uint32_t FAT_val;
        memcpy(&FAT_val, FAT + entry_offset, 32);
        FAT_val &= 0x0fffffff;

        if(FAT_val >= 0x0ffffff8)
            break; // end of cluster
        if(FAT_val == 0x0ffffff7)
            break; // bad cluster

        start_cluster = FAT_val;
    }
}

FILESYSTEM fat32_init(partition_entry_t part) {
    FILESYSTEM fat32;
    fat32.part = part;
    fat32.fs_type = FS_FAT32;

    fs_add(fat32);
    return fat32;
}
