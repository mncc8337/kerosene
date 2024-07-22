#include "filesystem.h"
#include "ata.h"

#include "string.h"
#include "debug.h"

static void parse_lfn(FAT_LFN* lfn, char* buff, int* cnt) {
    for(int i = 0; i < 5 && *cnt < 32; i++) {
        if(lfn->chars_1[i] == 0)
            return;
        buff[(*cnt)++] = (char)lfn->chars_1[i];
    }
    for(int i = 0; i < 6 && *cnt < 32; i++) {
        if(lfn->chars_2[i] == 0)
            return;
        buff[(*cnt)++] = (char)lfn->chars_2[i];
    }
    for(int i = 0; i < 2 && *cnt < 32; i++) {
        if(lfn->chars_3[i] == 0)
            return;
        buff[(*cnt)++] = (char)lfn->chars_3[i];
    }
}

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
    *year = (date >> 9) + 1980; // i read the docs lol
}

bool fat32_read_dir(fs_node_t* parent, bool (*callback)(fs_node_t)) {
    FAT32_BOOT_RECORD_t* bootrec = (FAT32_BOOT_RECORD_t*)parent->fs->info_table;
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = fat32_first_data_sector(bootrec);
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);

    uint32_t current_cluster = parent->start_cluster;
    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;
    uint8_t directory[cluster_size];

    uint8_t FAT[bootrec->bpb.bytes_per_sector];

    while(true) {
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(true, parent->fs->partition.LBA_start + first_sector,
                sectors_per_cluster, directory);

        bool lfn_ready = false;
        FAT_LFN temp_lfn;
        char lfn_name[32];
        int namelen = 0;
        FAT_DIRECTORY_ENTRY temp_dir;

        for(unsigned int i = 0; i < cluster_size; i += 32) {
            if(directory[i] == 0x00) // no more file/directory in this dir, free entry
                break;
            if(directory[i] == 0xe5) continue; // free entry
            // check if it is long file name entry
            if(directory[i+11] == 0x0f) {
                lfn_ready = true;

                if(namelen < 32) {
                    memcpy(&temp_lfn, directory + i, 32);
                    int startlen = namelen; // store for later use
                    parse_lfn(&temp_lfn, lfn_name, &namelen);
                    // reverse the parsed string because LFN entries are in reverse order
                    // set end point for strrev. note that it will be replaced in the next iteration
                    lfn_name[namelen] = '\0';
                    strrev(lfn_name + startlen);
                }
                // skip to the next entry
                continue;
            }
            memcpy(&temp_dir, directory + i, 32);
            if(!lfn_ready) continue;

            // long file name is ready
            fs_node_t node;
            node.fs = parent->fs;
            node.parent_node = parent;

            // the parser did not add the null character automatically
            // so we need to manually do it
            lfn_name[namelen++] = '\0';
            // reverse again to get the correct string
            strrev(lfn_name);
            // copy all the thing, including the null character added earlier
            memcpy(node.name, lfn_name, namelen);

            node.start_cluster = (uint32_t)temp_dir.first_cluster_number_high << 16 | temp_dir.first_cluster_number_low;
            node.centisecond = temp_dir.centisecond;
            node.creation_time = temp_dir.creation_time;
            node.creation_date = temp_dir.creation_date;
            node.last_access_date = temp_dir.last_access_date;
            node.last_mod_time = temp_dir.last_mod_time;
            node.last_mod_date = temp_dir.last_mod_date;
            node.attr = temp_dir.attr;

            node.size = temp_dir.size;

            // run callback
            if(!callback(node)) return false;

            // reset control var
            lfn_ready = false;
            namelen = 0;
        }

        // get next cluster
        int FAT_offset = current_cluster * 4;
        int FAT_sector = first_FAT_sector + FAT_offset / bootrec->bpb.bytes_per_sector;
        ata_pio_LBA28_access(true, parent->fs->partition.LBA_start + FAT_sector, 1, FAT);
        int entry_offset = FAT_offset % bootrec->bpb.bytes_per_sector;

        uint32_t FAT_val = *((uint32_t*)&(FAT[entry_offset]));
        FAT_val &= 0x0fffffff;

        if(FAT_val >= 0x0ffffff8)
            break; // end of cluster
        if(FAT_val == 0x0ffffff7)
            break; // bad cluster

        current_cluster = FAT_val;
    }

    return true;
}

void fat32_read_file(fs_node_t* node, uint8_t* buffer) {
    FAT32_BOOT_RECORD_t* bootrec = (FAT32_BOOT_RECORD_t*)node->fs->info_table;
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = fat32_first_data_sector(bootrec);
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);

    uint32_t current_cluster = node->start_cluster;
    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;

    uint8_t FAT[bootrec->bpb.bytes_per_sector];

    unsigned int cluster_count = 0;
    while(true) {
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        // read into buffer
        ata_pio_LBA28_access(true,
                node->fs->partition.LBA_start + first_sector,
                sectors_per_cluster, buffer + cluster_size * cluster_count);

        // get next cluster
        int FAT_offset = current_cluster * 4;
        int FAT_sector = first_FAT_sector + FAT_offset / bootrec->bpb.bytes_per_sector;
        ata_pio_LBA28_access(true, node->fs->partition.LBA_start + FAT_sector, 1, FAT);
        int entry_offset = FAT_offset % bootrec->bpb.bytes_per_sector;

        uint32_t FAT_val = *((uint32_t*)&(FAT[entry_offset]));
        FAT_val &= 0x0fffffff;

        if(FAT_val >= 0x0ffffff8)
            break; // end of cluster
        if(FAT_val == 0x0ffffff7)
            break; // bad cluster

        current_cluster = FAT_val;
        cluster_count++;
    }
}

// initialize FAT 32
// return the root node
fs_node_t fat32_init(partition_entry_t part, int id) {
    fs_t fs;
    fs.partition = part;
    fs.type = FS_FAT32;
    FAT32_BOOT_RECORD_t bootrec = fat32_get_bootrec(part);
    memcpy(fs.info_table, &bootrec, 512);

    fs_add(fs, id);

    fs_node_t rootnode;
    rootnode.fs = fs_get(id);
    rootnode.start_cluster = bootrec.ebpb.rootdir_cluster;
    rootnode.parent_node = 0; // no parent
    rootnode.attr = NODE_DIRECTORY;
    rootnode.valid = true;
    
    fs.root_node = rootnode;
    return rootnode;
}
