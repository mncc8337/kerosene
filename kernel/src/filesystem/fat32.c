#include "filesystem.h"
#include "ata.h"

#include "string.h"

// these processes are used very frequently
// so i define some macros for them

// TODO: check checksum (optional)
// to be sure that the long name and the short name matchs
// they will not match if the short entries are edited in
// a computer which does not support LFN
// which is very unlikely to occure
#define PROCESS_LFN_ENTRY(index) \
    lfn_ready = true; \
    memcpy(&temp_lfn, directory + index, sizeof(fat_lfn_entry_t)); \
    int order = temp_lfn.order; \
    bool last_LFN_entry = false; \
    if(!found_last_LFN_entry) { \
        found_last_LFN_entry = true; \
        last_LFN_entry = true; \
        order &= 0x3f; \
    } \
    int cnt; \
    int offset = (order - 1) * 13; \
    parse_lfn(&temp_lfn, entry_name, offset, &cnt); \
    if(last_LFN_entry) { \
        namelen = offset + cnt; \
        if(namelen >= FILENAME_LIMIT) { \
            namelen = FILENAME_LIMIT; \
            entry_name[FILENAME_LIMIT-1] = '\0'; \
        } \
    }

#define PROCESS_SFN_ENTRY(temp_dir) \
    int name_pos = 0; \
    while(temp_dir.name[name_pos] != ' ' && name_pos < 8) { \
        entry_name[name_pos] = temp_dir.name[name_pos]; \
        name_pos++; \
    } \
    if(temp_dir.name[8] != ' ') { \
        entry_name[name_pos++] = '.'; \
        for(int i = 8; i < 11 && temp_dir.name[i] != ' '; i++) { \
            entry_name[name_pos++] = temp_dir.name[i]; \
        } \
    } \
    entry_name[name_pos++] = '\0'; \
    namelen = name_pos;

// the table will be used very frequently
// so it is a good idea to only declare it once
static uint8_t FAT[512];
// store the last sector that is read to FAT
static int last_read_FAT_sector = -1;

static void parse_lfn(fat_lfn_entry_t* lfn, char* buff, int offset, int* cnt) {
    *cnt = 0;

    // TODO: unicode support (optional)
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

static size_t count_free_cluster(fat32_bootrecord_t* bootrec, fs_t* fs, uint32_t first_FAT_sector) {
    size_t free_count = 0;

    // it's gonna takes along time ...
    for(size_t i = 2; i < fat32_total_clusters(bootrec) + 2; i++) {
        if(get_FAT_entry(bootrec, fs, first_FAT_sector, i) == 0) free_count++;
    }

    return free_count;
}

static uint8_t gen_checksum(char* shortname) {
    // code taken from fatgen103.doc

    uint8_t sum = 0;

    for(int namelen = 11; namelen != 0; namelen--)
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *shortname++;

    return sum;
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

// find a free custer / clusters chain
// return start cluster address when success
// return 0 when failed to read fsinfo / cannot find a free cluster / not enough free cluster
uint32_t fat32_allocate_clusters(fs_t* fs, size_t cluster_count) {
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

    uint32_t current_cluster = fsinfo.available_clusters_start;
    if(current_cluster == 0xffffffff)
        current_cluster = 2;

    size_t free_cluster_count = fsinfo.free_cluster_count;
    if(free_cluster_count == 0xffffffff)
        free_cluster_count = count_free_cluster(bootrec, fs, first_FAT_sector);

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

    // no available cluster
    if(start_cluster == 0) return 0;

    // set end-of-cluster
    set_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster, 0x0ffffff8);

    // update the fsinfo
    fsinfo.available_clusters_start = current_cluster;
    fsinfo.free_cluster_count = free_cluster_count;
    ata_pio_LBA28_access(false, fs->partition.LBA_start + bootrec->ebpb.fsinfo_sector, 1, (uint8_t*)&fsinfo);

    return start_cluster;
}

// free a cluster chain
FS_ERR fat32_free_cluster_chain(fs_t* fs, uint32_t start_cluster) {
    fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)fs->info_table;
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);

    // read the FS info structure
    fat32_fsinfo_t fsinfo;
    ata_pio_LBA28_access(true, fs->partition.LBA_start + bootrec->ebpb.fsinfo_sector, 1, (uint8_t*)&fsinfo);

    // invalid signature
    if(fsinfo.lead_signature != 0x41615252
            || fsinfo.mid_signature != 0x61417272
            || fsinfo.trail_signature != 0xaa550000)
        return ERR_FS_INVALID_FSINFO;

    size_t fsinfo_free_cluster_count = fsinfo.free_cluster_count;
    if(fsinfo_free_cluster_count == 0xffffffff)
        fsinfo_free_cluster_count = count_free_cluster(bootrec, fs, first_FAT_sector);

    uint32_t fsinfo_start_cluster = fsinfo.available_clusters_start;
    if(fsinfo_start_cluster == 0xffffffff)
        fsinfo_start_cluster = 2;

    uint32_t current_cluster = start_cluster;
    while(current_cluster != 0x0ffffff8 && current_cluster != 0x0ffffff7) {
        uint32_t FAT_val = get_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster);
        // set it to 0 (free)
        set_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster, 0);
        current_cluster = FAT_val;
        fsinfo_free_cluster_count++;
    }

    if(current_cluster == 0x0ffffff7) return ERR_FS_BAD_CLUSTER;

    // update fsinfo
    // only update the start cluster if it is smaller
    if(start_cluster < fsinfo_start_cluster) fsinfo.available_clusters_start = start_cluster;
    fsinfo.free_cluster_count = fsinfo_free_cluster_count;
    ata_pio_LBA28_access(false, fs->partition.LBA_start + bootrec->ebpb.fsinfo_sector, 1, (uint8_t*)&fsinfo);

    return ERR_FS_SUCCESS;
}

// expand a cluster chain
// return start of expanded cluster address when success
// return 0 when cannot find free cluster
uint32_t fat32_expand_cluster_chain(fs_t* fs, uint32_t end_cluster, size_t cluster_count) {
    fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)fs->info_table;
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);
    // create new chain
    uint32_t new_chain = fat32_allocate_clusters(fs, cluster_count);
    // no more free space
    if(new_chain == 0) return 0;
    // link them
    set_FAT_entry(bootrec, fs, first_FAT_sector, end_cluster, new_chain);
    return new_chain;
}

// resize a cluster chain to only have 1 cluster
FS_ERR fat32_cut_cluster_chain(fs_t* fs, uint32_t start_cluster) {
    fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)fs->info_table;
    // get next cluster
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);
    uint32_t FAT_val = get_FAT_entry(bootrec, fs, first_FAT_sector, start_cluster);
    // free them
    FS_ERR err = fat32_free_cluster_chain(fs, FAT_val);
    if(err != ERR_FS_SUCCESS) return err;
    set_FAT_entry(bootrec, fs, first_FAT_sector, start_cluster, 0x0ffffff8);
    return ERR_FS_SUCCESS;
}

// get the last cluster in a cluster chain
uint32_t fat32_get_last_cluster_of_chain(fs_t* fs, uint32_t start_cluster) {
    fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)fs->info_table;
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);

    uint32_t current_cluster = start_cluster;
    uint32_t FAT_val = get_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster);
    while(FAT_val != 0x0ffffff8 && FAT_val != 0x0ffffff7) {
        current_cluster = FAT_val;
        FAT_val = get_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster);
    }

    return current_cluster;
}

// make a copy of a cluster chain
uint32_t fat32_copy_cluster_chain(fs_t* fs, uint32_t start_cluster) {
    // allocate the first cluster
    uint32_t copied_start_cluster = fat32_allocate_clusters(fs, 1);
    if(copied_start_cluster == 0) return 0;

    fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)fs->info_table;
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = fat32_first_data_sector(bootrec);
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);

    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;
    uint8_t data[cluster_size];

    uint32_t current_cluster = start_cluster;
    uint32_t copied_current_cluster = copied_start_cluster;
    while(true) {
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(true, fs->partition.LBA_start + first_sector, sectors_per_cluster, data);

        first_sector = ((copied_current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(false, fs->partition.LBA_start + first_sector, sectors_per_cluster, data);

        current_cluster = get_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster);
        if(current_cluster == 0x0ffffff8 || current_cluster == 0x0ffffff7)
            break;

        copied_current_cluster = fat32_expand_cluster_chain(fs, copied_current_cluster, 1);
    }

    return copied_start_cluster;
}

// loop through all files/entries in a dir
// call the callback when find one
FS_ERR fat32_read_dir(fs_node_t* parent, bool (*callback)(fs_node_t)) {
    if(!parent->isdir) return ERR_FS_NOT_DIR;

    fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)parent->fs->info_table;
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = fat32_first_data_sector(bootrec);
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);

    uint32_t current_cluster = parent->start_cluster;
    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;
    uint8_t directory[cluster_size];

    bool lfn_ready = false;
    fat_lfn_entry_t temp_lfn;
    char entry_name[FILENAME_LIMIT];
    int namelen = 0;
    bool found_last_LFN_entry = false;
    fat_directory_entry_t temp_dir;

    while(true) {
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(true, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);

        for(unsigned int i = 0; i < cluster_size; i += 32) {
            if(directory[i] == 0x00) // no more file/directory in this dir, free entry
                return ERR_FS_EXIT_NATURALLY;
            if(directory[i] == 0xe5) continue; // free entry
            // check if it is a long file name entry
            if(directory[i+11] == 0x0f) {
                PROCESS_LFN_ENTRY(i);
                continue;
            }

            memcpy(&temp_dir, directory + i, sizeof(fat_directory_entry_t));

            if(!lfn_ready) {
                PROCESS_SFN_ENTRY(temp_dir);
            }
            // LFN name is already ready
            fs_node_t node;

            node.dotdir = false;
            node.dotdotdir = false;
            if(strcmp(entry_name, ".")) {
                node.dotdir = true;
                memcpy(node.name, parent->name, strlen(parent->name)+1);
            }
            else if(strcmp(entry_name, "..")) {
                node.dotdotdir = true;
                memcpy(node.name, parent->parent_node->name, strlen(parent->parent_node->name)+1);
            }
            else memcpy(node.name, entry_name, namelen);

            node.fs = parent->fs;
            node.parent_node = parent;
            node.start_cluster = (uint32_t)temp_dir.first_cluster_number_high << 16
                                | temp_dir.first_cluster_number_low;
            if(node.start_cluster == 0 && node.dotdotdir) {
                // in linux the .. dir of a root's child directory is pointed to cluster 0
                // we need to fix it because root directory is in cluster 2
                // else we would destroy cluster 0 which contain filesystem infomation
                node.start_cluster = 2;
            }
            node.centisecond = temp_dir.centisecond;
            node.creation_time = temp_dir.creation_time;
            node.creation_date = temp_dir.creation_date;
            node.last_access_date = temp_dir.last_access_date;
            node.last_mod_time = temp_dir.last_mod_time;
            node.last_mod_date = temp_dir.last_mod_date;
            node.isdir = temp_dir.attr & FAT_ATTR_DIRECTORY;
            node.hidden = temp_dir.attr & FAT_ATTR_HIDDEN;

            node.size = temp_dir.size;
            node.parent_cluster = current_cluster;
            node.parent_cluster_index = i;

            // run callback
            if(!callback(node)) return ERR_FS_CALLBACK_STOP;

            // reset control var
            lfn_ready = false;
            found_last_LFN_entry = false;
        }

        uint32_t FAT_val = get_FAT_entry(bootrec, parent->fs, first_FAT_sector, current_cluster);

        if(FAT_val >= 0x0ffffff8)
            break; // end of cluster
        if(FAT_val == 0x0ffffff7)
            return ERR_FS_BAD_CLUSTER;

        current_cluster = FAT_val;
    }

    return ERR_FS_EXIT_NATURALLY;
}

// read some clusters to buffer
FS_ERR fat32_read_file(fs_t* fs, uint32_t* start_cluster, uint8_t* buffer, size_t size, int cluster_offset) {
    fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)fs->info_table;
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = fat32_first_data_sector(bootrec);
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);

    uint32_t current_cluster = *start_cluster;
    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;
    uint8_t ext_buffer[cluster_size];

    if(cluster_offset > 0) {
        // align buffer to cluster size
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(true, fs->partition.LBA_start + first_sector, sectors_per_cluster, ext_buffer);

        memcpy(buffer, ext_buffer + cluster_offset, (cluster_size - cluster_offset > size ? size : cluster_size - cluster_offset));
        if(size <= cluster_size - cluster_offset) return ERR_FS_SUCCESS;

        size -= cluster_size - cluster_offset;
        buffer += cluster_size - cluster_offset;

        // get next cluster
        uint32_t FAT_val = get_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster);

        if(FAT_val >= 0x0ffffff8)
            return ERR_FS_EOF;
        if(FAT_val == 0x0ffffff7)
            return ERR_FS_BAD_CLUSTER;

        current_cluster = FAT_val;
    }

    unsigned int read_time = 0;
    while(size > 0) {
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;

        if((unsigned)size >= cluster_size)
            ata_pio_LBA28_access(true, fs->partition.LBA_start + first_sector, sectors_per_cluster, buffer + read_time * cluster_size);
        else {
            ata_pio_LBA28_access(true, fs->partition.LBA_start + first_sector, sectors_per_cluster, ext_buffer);
            memcpy(buffer + read_time * cluster_size, ext_buffer, size);
            break;
        }

        // get next cluster
        uint32_t FAT_val = get_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster);

        if(FAT_val >= 0x0ffffff8)
            return ERR_FS_EOF;
        if(FAT_val == 0x0ffffff7)
            return ERR_FS_BAD_CLUSTER;

        current_cluster = FAT_val;
        read_time++;
        size -= cluster_size;
    }
    if(cluster_offset > 0) buffer -= cluster_size - cluster_offset;

    *start_cluster = current_cluster;

    return ERR_FS_SUCCESS;
}

// write infomation into cluster
FS_ERR fat32_write_file(fs_t* fs, uint32_t* start_cluster, uint8_t* buffer, size_t size, int cluster_offset) {
    fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)fs->info_table;
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = fat32_first_data_sector(bootrec);
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);

    uint32_t current_cluster = *start_cluster;
    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;
    uint8_t ext_buffer[cluster_size];

    if(cluster_offset > 0) {
        // align buffer to cluster size
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(true, fs->partition.LBA_start + first_sector, sectors_per_cluster, ext_buffer);

        memcpy(ext_buffer + cluster_offset, buffer, (cluster_size - cluster_offset > size ? size : cluster_size - cluster_offset));
        ata_pio_LBA28_access(false, fs->partition.LBA_start + first_sector, sectors_per_cluster, ext_buffer);
        if(size <= cluster_size - cluster_offset) return ERR_FS_SUCCESS;

        size -= cluster_size - cluster_offset;
        buffer += cluster_size - cluster_offset;
        current_cluster = fat32_expand_cluster_chain(fs, current_cluster, 1);
        if(current_cluster == 0) return ERR_FS_FAILED;
    }

    int estimated_clusters = size / cluster_size;
    if(estimated_clusters > 0)
        fat32_expand_cluster_chain(fs, current_cluster, estimated_clusters);

    int write_time = 0;
    while(size > 0) {
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        // write
        if(size >= cluster_size)
            ata_pio_LBA28_access(false, fs->partition.LBA_start + first_sector, sectors_per_cluster, buffer + write_time * cluster_size);
        else {
            memcpy(ext_buffer, buffer, size);
            memset(ext_buffer + size, 0, cluster_size - size);
            ata_pio_LBA28_access(false, fs->partition.LBA_start + first_sector, sectors_per_cluster, ext_buffer);
            break;
        }

        // get next cluster
        uint32_t FAT_val = get_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster);

        if(FAT_val >= 0x0ffffff8)
            break; // end of cluster
        if(FAT_val == 0x0ffffff7)
            return ERR_FS_BAD_CLUSTER; // bad cluster

        current_cluster = FAT_val;
        write_time++;
        size -= cluster_size;
    }

    if(cluster_offset > 0) buffer -= cluster_size - cluster_offset;

    *start_cluster = current_cluster;
    return ERR_FS_SUCCESS;
}

// WARNING
// the functions below are really messy
// they are clones of the read_dir function
// so they span over hundreds line of code
// also variable naming are very confusing
// you have been warned!

// add a new entry to parent
// return valid node when success
// return invalid node when cannot find a free cluster / an entry with the same name has already exists
fs_node_t fat32_add_entry(fs_node_t* parent, char* name, uint32_t start_cluster, uint8_t attr, size_t size) {
    fs_node_t node;
    node.valid = false;

    fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)parent->fs->info_table;
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = fat32_first_data_sector(bootrec);
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);

    uint32_t current_cluster = parent->start_cluster;
    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;
    uint8_t directory[cluster_size];

    // is this entry is a . or .. entry
    bool dotdot_entry = strcmp(name, ".") || strcmp(name, "..");

    // if is creating a directory and not a dotdot entry
    if((attr & FAT_ATTR_DIRECTORY) && !dotdot_entry) {
        // clear target cluster
        memset(directory, 0, cluster_size);
        uint32_t first_sector = ((start_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(false, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);
    }

    bool lfn_ready = false;
    fat_lfn_entry_t temp_lfn;
    char entry_name[FILENAME_LIMIT];
    int namelen = 0;
    bool found_last_LFN_entry = false;
    fat_directory_entry_t temp_dir;

    unsigned int start_index;
    bool new_cluster_created = false;
    while(true) {
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        if(new_cluster_created) {
            // clear the new cluster
            // this will guarantee us to find a free entry
            memset(directory, 0, cluster_size);
            ata_pio_LBA28_access(false, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);
        }
        else
            ata_pio_LBA28_access(true, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);

        bool found = false;
        for(start_index = 0; start_index < cluster_size; start_index += 32) {
            if(directory[start_index] == 0x00) {
                found = true;
                break;
            }
            if(directory[start_index] == 0xe5) continue;
            if(directory[start_index+11] == 0x0f) {
                PROCESS_LFN_ENTRY(start_index);
                continue;
            }

            memcpy(&temp_dir, directory + start_index, sizeof(fat_directory_entry_t));
            if(!lfn_ready) {
                PROCESS_SFN_ENTRY(temp_dir);
            }

            // name of LFN entries have been parsed at this point
            if(strcmp_case_insensitive(entry_name, name)) {
                // found duplication
                return node;
            }
            lfn_ready = false;
            found_last_LFN_entry = false;
        }
        if(found) break;

        uint32_t FAT_val = get_FAT_entry(bootrec, parent->fs, first_FAT_sector, current_cluster);

        if(FAT_val >= 0x0ffffff8) { // end of cluster
            // add a new cluster
            current_cluster = fat32_expand_cluster_chain(parent->fs, current_cluster, 1);
            // no more space
            if(current_cluster == 0) return node;
            new_cluster_created = true;
            continue;
        }

        current_cluster = FAT_val;
    }

    // we found a start index in current cluster

    // override namelen
    namelen = strlen(name);

    // calculate the number of LFN entries will be generated
    int lfn_entry_count = (namelen + 13) / 13; // include the null char
    if(dotdot_entry) lfn_entry_count = 0; // no need to create LFN entry for . and ..

    // calculate how many clusters are needed to create
    int estimated_new_cluster_count = (lfn_entry_count - (cluster_size - start_index)/sizeof(fat_lfn_entry_t) + 17) /
                                      (cluster_size / sizeof(fat_lfn_entry_t)); // ididthemath
    if(estimated_new_cluster_count > 0) {
        // try to expand
        uint32_t valid = fat32_expand_cluster_chain(parent->fs, current_cluster, estimated_new_cluster_count);
        if(!valid) return node;
    }

    memcpy(node.name, name, namelen+1);

    char shortname[11]; memset(shortname, ' ', 11);

    // generating short name
    // . and .. directory need to be handle differently
    if(dotdot_entry) {
        memcpy(shortname, name, namelen);
    }
    else {
        // it's only needed to be different from the others short name entries
        // so we can actually using a random number generator to do this
        bool isdir = attr & FAT_ATTR_DIRECTORY;
        // find the last embedded period
        int dot_pos;
        for(dot_pos = namelen-1; dot_pos > 0; dot_pos--)
            if(name[dot_pos] == '.') break;
        // extension
        if(dot_pos > 0 && !isdir) {
            int name_pos = dot_pos+1; int filled_chars = 0;
            while(name[name_pos] != '\0' && filled_chars < 3) {
                if(name[name_pos] == ' '
                        || name[name_pos] == '+'
                        || name[name_pos] == ','
                        || name[name_pos] == ';'
                        || name[name_pos] == '='
                        || name[name_pos] == '['
                        || name[name_pos] == ']') name_pos++;
                else shortname[8 + filled_chars++] = name[name_pos++];
            }
        }
        // base name
        int name_pos = 0; int filled_chars = 0;
        while(name[name_pos] != '\0' && filled_chars < 8) {
            if(name_pos == dot_pos && dot_pos > 0 && !isdir) break;
            if(name[name_pos] == ' '
                    || name[name_pos] == '.'
                    || name[name_pos] == '+'
                    || name[name_pos] == ','
                    || name[name_pos] == ';'
                    || name[name_pos] == '='
                    || name[name_pos] == '['
                    || name[name_pos] == ']') name_pos++;
            else shortname[filled_chars++] = name[name_pos++];
        }
        // add numeric tail
        if(dot_pos > 8 || (isdir && namelen > 8)) {
            // TODO: generate some random chars here
            // this implementation did not guarantee
            // us to create a unique shortname
            // that's why we need a random number generator
            shortname[6] = '~';
            shortname[7] = '1';
        }
        // convert to uppercase
        for(int i = 0; i < 11; i++)
            if(shortname[i] >= 0x61 && shortname[i] <= 0x7a) shortname[i] -= 0x20;
    }

    // generate checksum
    uint8_t checksum = gen_checksum(shortname);

    node.valid = true;
    node.parent_node = parent;
    node.fs = parent->fs;
    node.parent_node = parent;
    node.start_cluster = start_cluster;
    node.isdir = attr & FAT_ATTR_DIRECTORY;
    node.hidden = attr & FAT_ATTR_HIDDEN;
    node.size = size;
    // TODO: set correct time attr
    // these are dummy value
    node.centisecond = 102;
    node.creation_time = 0b0010110011101111;
    node.creation_date = 0b0101100110001101;
    node.last_access_date = 0b0101100110001101;
    node.last_mod_time = 0b0010110011101111;
    node.last_mod_date = 0b0101100110001101;

    // add LFN entries
    for(int i = lfn_entry_count; i >= 0; i--) {
        if(i > 0) { // long file name entry
            fat_lfn_entry_t* temp_lfn = (fat_lfn_entry_t*)(directory + start_index);
            memset((char*)temp_lfn, 0, sizeof(fat_lfn_entry_t));

            temp_lfn->order = i;
            if(i == lfn_entry_count) // "first" LFN entry
                temp_lfn->order |= 0x40;
            temp_lfn->attr = 0x0f;
            temp_lfn->checksum = checksum;

            // TODO: unicode support (optional)
            for(int j = 0; j < 5; j++) {
                temp_lfn->chars_1[j] = name[(i-1) * 13 + j];
                if(temp_lfn->chars_1[j] == '\0') goto done_parsing_name;
            }
            for(int j = 0; j < 6; j++) {
                temp_lfn->chars_2[j] = name[(i-1) * 13 + j + 5];
                if(temp_lfn->chars_2[j] == '\0') goto done_parsing_name;
            }
            for(int j = 0; j < 2; j++) {
                temp_lfn->chars_3[j] = name[(i-1) * 13 + j + 5 + 6];
                if(temp_lfn->chars_3[j] == '\0') goto done_parsing_name;
            }

        }
        else if(i == 0) { // directory entry
            fat_directory_entry_t* dir_entry = (fat_directory_entry_t*)(directory + start_index);
            memcpy(dir_entry->name, shortname, 11);
            dir_entry->attr = attr;
            dir_entry->centisecond = node.centisecond;
            dir_entry->creation_time = node.creation_time;
            dir_entry->creation_date = node.creation_date;
            dir_entry->last_access_date = node.last_access_date;
            dir_entry->first_cluster_number_high = start_cluster >> 16;
            dir_entry->last_mod_time = node.last_mod_time;
            dir_entry->last_mod_date = node.last_mod_date;
            dir_entry->first_cluster_number_low = start_cluster & 0xffff;
            dir_entry->size = size;
        }
        done_parsing_name:

        node.parent_cluster = current_cluster;
        node.parent_cluster_index = start_index;

        // make changes
        start_index += 32;
        if(start_index >= cluster_size) { // current cluster is exceeded
            uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
            // only write changes after filling a sector worth of byte
            ata_pio_LBA28_access(false, parent->fs->partition.LBA_start + first_sector,
                    sectors_per_cluster, directory);
            // get next cluster
            // the next cluster is always valid because we have created it before
            current_cluster = get_FAT_entry(bootrec, parent->fs, first_FAT_sector, current_cluster);
            start_index = 0;
        }
    }

    if(start_index != 0) {
        // clear the remain bytes
        memset(directory + start_index, 0, cluster_size - start_index);
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(false, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);
    }

    return node;
}

// remove an entry from parent
FS_ERR fat32_remove_entry(fs_node_t* parent, fs_node_t remove_node, bool remove_content) {
    if(remove_node.dotdir || remove_node.dotdotdir)
        return ERR_FS_FAILED;

    fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)parent->fs->info_table;
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = fat32_first_data_sector(bootrec);
    uint32_t first_FAT_sector = fat32_first_FAT_sector(bootrec);

    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;
    uint8_t directory[cluster_size];

    int lfn_entry_count = (strlen(remove_node.name) + 12) / 13;
    if(remove_node.dotdir || remove_node.dotdotdir) lfn_entry_count = 0; // just for safety

    int node_index = remove_node.parent_cluster_index;
    uint32_t current_cluster = remove_node.parent_cluster;

    int namelen; (void)(namelen); // avoid unused param
    char entry_name[FILENAME_LIMIT];
    if(remove_node.isdir && remove_content) {
        // check if it has any child entry
        uint32_t first_sector = ((remove_node.start_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(true, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);

        fat_directory_entry_t temp_temp_dir; // im good at naming things
        for(unsigned int i = 0; i < cluster_size; i += 32) {
            if(directory[i] == 0x00) break; // no entry, yay
            if(directory[i] == 0xe5) continue; // this should not happen
            if(directory[i+11] == 0x0f) // if it has any LFN entry that mean there is a child entry
                return ERR_FS_DIR_NOT_EMPTY;

            memcpy(&temp_temp_dir, directory + i, sizeof(fat_directory_entry_t));

            // at this point this entry should be a SFN
            PROCESS_SFN_ENTRY(temp_temp_dir);
            // if it is not . or .. entry then there is a child entry
            if(!strcmp(entry_name, ".") && !strcmp(entry_name, ".."))
                return ERR_FS_DIR_NOT_EMPTY;
        }
        // so we only found . and .. entry
    }
    // now we are good to remove the entry

    // check if the next entry is clear
    int clear_val;
    if((unsigned)node_index + 32 < cluster_size) {
        if(directory[node_index+32] == 0x0)
            clear_val = 0x0;
        else clear_val = 0xe5;
    }
    else {
        // check next cluster
        // to see if it is clear or not
        uint32_t FAT_val = get_FAT_entry(bootrec, parent->fs, first_FAT_sector, current_cluster);

        if(FAT_val >= 0x0ffffff8 || FAT_val == 0x0ffffff7) { // it should never happend
            clear_val = 0x0;
        }
        else {
            int first_sector = ((FAT_val - 2) * sectors_per_cluster) + first_data_sector;
            // read the very next sector
            ata_pio_LBA28_access(true, parent->fs->partition.LBA_start + first_sector, 1, directory);
            // now check
            if(directory[0] == 0x0)
                clear_val = 0x0;
            else clear_val = 0xe5;
        }
    }

    // read the cluster that contain the node
    uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
    ata_pio_LBA28_access(true, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);

    // delete the cluster
    if(remove_content) fat32_free_cluster_chain(parent->fs, remove_node.start_cluster);

    // calculate the start of the chain
    int lfn_entry_len = 32 * lfn_entry_count;
    node_index -= lfn_entry_len;

    if(clear_val == 0xe5 || (clear_val == 0x0 && node_index >= 0)) {
        // delete LFN entries in the current cluster
        memset(directory+(node_index < 0 ? 0 : node_index), clear_val, 32 * (lfn_entry_count + 1));
        // write
        int first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(false, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);
    }
    if(node_index < 0) {
        // go back to the last cluster to delete the LFN entry
        node_index += cluster_size;

        // find previous cluster of current cluster
        uint32_t last_cluster = parent->start_cluster;
        uint32_t FAT_val = parent->start_cluster;
        while(FAT_val != current_cluster) {
            last_cluster = FAT_val;
            FAT_val = get_FAT_entry(bootrec, parent->fs, first_FAT_sector, FAT_val);
        }

        int first_sector = ((last_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(true, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);

        memset(directory+node_index, clear_val, cluster_size - node_index);

        // write
        ata_pio_LBA28_access(false, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);

        if(clear_val == 0x0) {
            fat32_free_cluster_chain(parent->fs, current_cluster);
            set_FAT_entry(bootrec, parent->fs, first_FAT_sector, last_cluster, 0x0ffffff8);
        }
    }

    // the pain is over

    return ERR_FS_SUCCESS;
}

// update node entry infomation on disk
FS_ERR fat32_update_entry(fs_node_t* node) {
    fat32_bootrecord_t* bootrec = (fat32_bootrecord_t*)node->fs->info_table;
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = fat32_first_data_sector(bootrec);

    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;
    uint8_t directory[cluster_size];

    uint32_t first_sector = ((node->parent_cluster - 2) * sectors_per_cluster) + first_data_sector;
    ata_pio_LBA28_access(true, node->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);

    fat_directory_entry_t* dir = (fat_directory_entry_t*)(directory+node->parent_cluster_index);
    // TODO: check if the entry is really exists
    
    dir->last_access_date = node->last_access_date;
    dir->last_mod_time = node->last_mod_time;
    dir->last_mod_date = node->last_mod_date;
    dir->attr = node->isdir | node->hidden;
    dir->size = node->size;
    // name updating is very problematic
    // so i you want to update the name
    // just add a new entry and delete the old one

    // write changes
    ata_pio_LBA28_access(false, node->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);

    return ERR_FS_SUCCESS;
}

// make a directory in parent node
// also create . and .. dir
// return valid node when success
// return invalid node when node with the same name has already exists / cannot find free cluster
fs_node_t fat32_mkdir(fs_node_t* parent, char* name, uint32_t start_cluster, uint8_t attr) {
    fs_node_t created_dir;
    created_dir.valid = false;

    // remove leading spaces, trailing spaces and trailing periods
    while(name[0] == ' ') name++;
    int namelen = strlen(name);
    if(namelen == 0) return created_dir;
    while(name[namelen-1] == ' ' || name[namelen-1] == '.') {
        name[namelen-1] = '\0';
        namelen--;
        if(namelen == 0) return created_dir;
    }

    created_dir = fat32_add_entry(parent, name, start_cluster, attr | FAT_ATTR_DIRECTORY, 0);
    if(!created_dir.valid) return created_dir;

    // create the '..' and the '.' dir
    // in linux you need to create them
    // so that `ls` will not show input/output error
    fs_node_t dot_dir = fat32_add_entry(&created_dir, ".", start_cluster, FAT_ATTR_DIRECTORY, 0);
    if(!dot_dir.valid) return dot_dir;
    fs_node_t dotdot_dir = fat32_add_entry(&created_dir, "..", parent->start_cluster, FAT_ATTR_DIRECTORY, 0);
    if(!dotdot_dir.valid) return dotdot_dir;

    return created_dir;
}

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
    rootnode.isdir = true;
    rootnode.hidden = false;
    rootnode.valid = true;

    fs.root_node = rootnode;
    return rootnode;
}
