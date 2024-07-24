#include "filesystem.h"
#include "ata.h"

#include "string.h"
#include "debug.h"

// the table will be used very frequently
// so it is a good idea to only declare it once
static uint8_t FAT[512];
// store the last sector that is read to FAT
static int last_read_FAT_sector = -1;

static void parse_lfn(fat_lfn_t* lfn, char* buff, int offset, int* cnt) {
    *cnt = 0;

    for(int i = 0; i < 5 && offset < FILENAME_LIMIT; i++) {
        buff[offset++] = (char)lfn->chars_1[i];
        (*cnt)++;
        if(lfn->chars_1[i] == 0) return;
    }
    for(int i = 0; i < 6 && offset < FILENAME_LIMIT; i++) {
        buff[offset++] = (char)lfn->chars_2[i];
        (*cnt)++;
        if(lfn->chars_2[i] == 0) return;
    }
    for(int i = 0; i < 2 && offset < FILENAME_LIMIT; i++) {
        buff[offset++] = (char)lfn->chars_3[i];
        (*cnt)++;
        if(lfn->chars_3[i] == 0) return;
    }
}

static void set_FAT_entry(fat32_bootrecord_t* bootrec, fs_t* fs,
    uint32_t first_FAT_sector, uint32_t cluster, uint32_t val) {
    int FAT_offset = cluster * 4;
    int FAT_sector = first_FAT_sector + FAT_offset / bootrec->bpb.bytes_per_sector;
    int entry_offset = FAT_offset % bootrec->bpb.bytes_per_sector;
    if(FAT_sector != last_read_FAT_sector) {
        ata_pio_LBA28_access(true, fs->partition.LBA_start + FAT_sector, 1, FAT);
        last_read_FAT_sector = FAT_sector;
    }

    *((uint32_t*)&(FAT[entry_offset])) = val;
    ata_pio_LBA28_access(false, fs->partition.LBA_start + FAT_sector, 1, FAT);
}
static uint32_t get_FAT_entry(fat32_bootrecord_t* bootrec, fs_t* fs,
        uint32_t first_FAT_sector, uint32_t cluster) {
    int FAT_offset = cluster * 4;
    int FAT_sector = first_FAT_sector + FAT_offset / bootrec->bpb.bytes_per_sector;
    int entry_offset = FAT_offset % bootrec->bpb.bytes_per_sector;
    if(FAT_sector != last_read_FAT_sector) {
        ata_pio_LBA28_access(true, fs->partition.LBA_start + FAT_sector, 1, FAT);
        last_read_FAT_sector = FAT_sector;
    }

    return *((uint32_t*)&(FAT[entry_offset])) & 0x0fffffff;
}

fat32_bootrecord_t fat32_get_bootrec(partition_entry_t part) {
    fat32_bootrecord_t bootrec;
    ata_pio_LBA28_access(true, part.LBA_start, 1, (uint8_t*)(&bootrec));
    return bootrec;
}

uint32_t fat32_total_sectors(fat32_bootrecord_t* bootrec) {
    return bootrec->bpb.total_sectors == 0 ? bootrec->bpb.large_sector_count : bootrec->bpb.total_sectors;
}

uint32_t fat32_FAT_size(fat32_bootrecord_t* bootrec) {
    return bootrec->bpb.sectors_per_FAT == 0 ? bootrec->ebpb.sectors_per_FAT : bootrec->bpb.sectors_per_FAT;
}

uint32_t fat32_first_data_sector(fat32_bootrecord_t* bootrec) {
    return bootrec->bpb.reserved_sectors + bootrec->bpb.FATs * fat32_FAT_size(bootrec);
}

uint32_t fat32_first_FAT_sector(fat32_bootrecord_t* bootrec) {
    return bootrec->bpb.reserved_sectors;
}

uint32_t fat32_total_data_sectors(fat32_bootrecord_t* bootrec) {
    return fat32_total_sectors(bootrec) - fat32_first_data_sector(bootrec);
}

uint32_t fat32_total_clusters(fat32_bootrecord_t* bootrec) {
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
    *year = (date >> 9) + 1980;
}

bool fat32_read_dir(fs_node_t* parent, bool (*callback)(fs_node_t)) {
    fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)parent->fs->info_table;
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = fat32_first_data_sector(bootrec);
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);

    uint32_t current_cluster = parent->start_cluster;
    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;
    uint8_t directory[cluster_size];

    while(true) {
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(true, parent->fs->partition.LBA_start + first_sector,
                sectors_per_cluster, directory);

        bool lfn_ready = false;
        fat_lfn_t temp_lfn;
        char lfn_name[FILENAME_LIMIT];
        int namelen = 0;
        fat_directory_entry_t temp_dir;

        for(unsigned int i = 0; i < cluster_size; i += 32) {
            if(directory[i] == 0x00) // no more file/directory in this dir, free entry
                break;
            if(directory[i] == 0xe5) continue; // free entry
            // check if it is a long file name entry
            if(directory[i+11] == 0x0f) {
                lfn_ready = true;

                memcpy(&temp_lfn, directory + i, sizeof(fat_lfn_t));

                int cnt;
                int offset = ((temp_lfn.order & 0xf) - 1) * 13;
                parse_lfn(&temp_lfn, lfn_name, offset, &cnt);
                if((temp_lfn.order >> 4) == 4) {
                    namelen = offset + cnt;
                    if(namelen > FILENAME_LIMIT) {
                        namelen = FILENAME_LIMIT;
                        // the name is exceeding the limit
                        // so we need to set the null char manually
                        lfn_name[FILENAME_LIMIT-1] = '\0';
                    }
                }

                // skip to the next entry
                continue;
            }
            memcpy(&temp_dir, directory + i, sizeof(fat_directory_entry_t));
            if(!lfn_ready) continue;

            // long file name is ready
            fs_node_t node;
            node.fs = parent->fs;
            node.parent_node = parent;

            memcpy(node.name, lfn_name, namelen);

            node.start_cluster = (uint32_t)temp_dir.first_cluster_number_high << 16
                                | temp_dir.first_cluster_number_low;
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
        }

        uint32_t FAT_val = get_FAT_entry(bootrec, parent->fs, first_FAT_sector, current_cluster);

        if(FAT_val >= 0x0ffffff8)
            break; // end of cluster
        if(FAT_val == 0x0ffffff7)
            break; // bad cluster

        current_cluster = FAT_val;
    }

    return true;
}

void fat32_read_file(fs_node_t* node, uint8_t* buffer) {
    fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)node->fs->info_table;
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = fat32_first_data_sector(bootrec);
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);

    uint32_t current_cluster = node->start_cluster;
    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;

    unsigned int cluster_count = 0;
    while(true) {
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        // read into buffer
        ata_pio_LBA28_access(true,
                node->fs->partition.LBA_start + first_sector,
                sectors_per_cluster, buffer + cluster_size * cluster_count);

        // get next cluster
        uint32_t FAT_val = get_FAT_entry(bootrec, node->fs, first_FAT_sector, current_cluster);

        if(FAT_val >= 0x0ffffff8)
            break; // end of cluster
        if(FAT_val == 0x0ffffff7)
            break; // bad cluster

        current_cluster = FAT_val;
        cluster_count++;
    }
}

uint32_t fat32_find_free_clusters(fs_t* fs, size_t cluster_count) {
    fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)fs->info_table;
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);

    // read the FS info
    fat32_fsinfo_t fsinfo;
    ata_pio_LBA28_access(true, fs->partition.LBA_start + bootrec->ebpb.fsinfo_sector, 1, (uint8_t*)&fsinfo);

    // invalid signature
    if(fsinfo.lead_signature != 0x41615252
            || fsinfo.mid_signature != 0x61417272
            || fsinfo.trail_signature != 0xaa550000)
        return 0;

    // note that available clusters start and free cluster count
    // infomation are verified and fixed when initialize the fs

    uint32_t current_cluster = fsinfo.available_clusters_start;
    size_t free_cluster_count = fsinfo.free_cluster_count;

    uint32_t start_cluster = 0;
    uint32_t prev_cluster = 0;

    while(cluster_count > 0) {
        uint32_t FAT_val = get_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster);

        if(FAT_val == 0) {
            // we found a free cluster
            if(start_cluster == 0)
                start_cluster = current_cluster;
            else
                set_FAT_entry(bootrec, fs, first_FAT_sector, prev_cluster, current_cluster);
            prev_cluster = current_cluster;
            cluster_count--;
            free_cluster_count--;
        }
        current_cluster++;
    }
    current_cluster--;
    // set end-of-cluster
    set_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster, 0x0ffffff8);

    // update the fsinfo
    fsinfo.available_clusters_start = current_cluster;
    fsinfo.free_cluster_count = free_cluster_count;
    ata_pio_LBA28_access(false, fs->partition.LBA_start + bootrec->ebpb.fsinfo_sector, 1, (uint8_t*)&fsinfo);

    return start_cluster;
}

void fat32_free_clusters_chain(fs_t* fs, uint32_t start_cluster) {
    fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)fs->info_table;
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);

    // read the FS info structure
    fat32_fsinfo_t fsinfo;
    ata_pio_LBA28_access(true, fs->partition.LBA_start + bootrec->ebpb.fsinfo_sector, 1, (uint8_t*)&fsinfo);

    // invalid signature
    if(fsinfo.lead_signature != 0x41615252
            || fsinfo.mid_signature != 0x61417272
            || fsinfo.trail_signature != 0xaa550000)
        return;

    size_t fsinfo_free_cluster_count = fsinfo.free_cluster_count;
    uint32_t fsinfo_start_cluster = fsinfo.available_clusters_start;

    uint32_t current_cluster = start_cluster;
    while(current_cluster != 0x0ffffff8) {
        uint32_t FAT_val = get_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster);
        // set it to 0 (free)
        set_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster, 0);
        current_cluster = FAT_val;
        fsinfo_free_cluster_count++;
    }

    // update fsinfo
    // only update the start cluster if it is smaller
    if(start_cluster < fsinfo_start_cluster) fsinfo.available_clusters_start = start_cluster;
    fsinfo.free_cluster_count = fsinfo_free_cluster_count;
    ata_pio_LBA28_access(false, fs->partition.LBA_start + bootrec->ebpb.fsinfo_sector, 1, (uint8_t*)&fsinfo);
}

uint32_t fat32_expand_clusters_chain(fs_t* fs, uint32_t end_cluster, size_t cluster_count) {
    fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)fs->info_table;
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);
    // create new chain
    uint32_t new_chain = fat32_find_free_clusters(fs, cluster_count);
    // link them
    set_FAT_entry(bootrec, fs, first_FAT_sector, end_cluster, new_chain);
    return new_chain;
}

// void fat32_add_file_to_dir(fs_t* fs, fs_node_t* parent, fs_node_t* file) {
//     ;
// }

// initialize FAT 32
// return the root node
fs_node_t fat32_init(partition_entry_t part, int id) {
    fs_t fs;
    fs.partition = part;
    fs.type = FS_FAT32;
    fat32_bootrecord_t bootrec = fat32_get_bootrec(part);
    memcpy(fs.info_table, &bootrec, sizeof(fat32_bootrecord_t));

    fs_add(fs, id);

    fs_node_t rootnode;
    rootnode.fs = fs_get(id);
    rootnode.start_cluster = bootrec.ebpb.rootdir_cluster;
    rootnode.parent_node = 0; // no parent
    rootnode.attr = NODE_DIRECTORY;
    rootnode.valid = true;

    // TODO: verify information in fsinfo
    
    fs.root_node = rootnode;
    return rootnode;
}
