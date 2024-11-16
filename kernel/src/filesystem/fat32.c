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
#define PROCESS_LFN_ENTRY(entry_name, namelen, temp_lfn, lfn_ready, found_last_LFN_entry) \
    lfn_ready = true; \
    int order = temp_lfn->order; \
    bool last_LFN_entry = false; \
    if(!found_last_LFN_entry) { \
        found_last_LFN_entry = true; \
        last_LFN_entry = true; \
        order &= 0x3f; \
    } \
    int cnt; \
    int offset = (order - 1) * 13; \
    parse_lfn(temp_lfn, entry_name, offset, &cnt); \
    if(last_LFN_entry) { \
        namelen = offset + cnt; \
        if(namelen >= FILENAME_LIMIT) { \
            namelen = FILENAME_LIMIT; \
            entry_name[FILENAME_LIMIT-1] = '\0'; \
        } \
    }

#define PROCESS_SFN_ENTRY(entry_name, namelen, temp_dir) \
    int name_pos = 0; \
    while(temp_dir->name[name_pos] != ' ' && name_pos < 8) { \
        entry_name[name_pos] = temp_dir->name[name_pos]; \
        name_pos++; \
    } \
    if(temp_dir->name[8] != ' ') { \
        entry_name[name_pos++] = '.'; \
        for(int i = 8; i < 11 && temp_dir->name[i] != ' '; i++) { \
            entry_name[name_pos++] = temp_dir->name[i]; \
        } \
    } \
    entry_name[name_pos++] = '\0'; \
    namelen = name_pos;

// the table will be used very frequently
// so it is a good idea to only declare it once
// FIXME: turns out this is dumb because it would cause conflicts between processes
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

static uint8_t gen_checksum(char* shortname) {
    // code taken from fatgen103.doc
    uint8_t sum = 0;

    for(int namelen = 11; namelen != 0; namelen--)
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *shortname++;

    return sum;
}

static void get_bootrec(partition_entry_t part, uint8_t* bootrec) {
    ata_pio_LBA28_access(true, part.LBA_start, 1, bootrec);
}
// static void update_bootrecord(fs_t* fs) {
//     ata_pio_LBA28_access(false, fs->partition.LBA_start, 1, fs->info_table1);
// }

static void get_fsinfo(partition_entry_t part, fat32_bootrecord_t* bootrec, uint8_t* fsinfo) {
    ata_pio_LBA28_access(true, part.LBA_start + bootrec->ebpb.fsinfo_sector, 1, fsinfo);
}
static void update_fsinfo(fs_t* fs) {
    ata_pio_LBA28_access(
        false,
        fs->partition.LBA_start + fs->fat32_info.bootrec.ebpb.fsinfo_sector,
        1, (uint8_t*)(&(fs->fat32_info.fsinfo))
    );
}

static uint32_t get_total_sectors(fat32_bootrecord_t* bootrec) {
    return bootrec->bpb.total_sectors == 0 ? bootrec->bpb.large_sector_count : bootrec->bpb.total_sectors;
}

static uint32_t get_FAT_size(fat32_bootrecord_t* bootrec) {
    return bootrec->bpb.sectors_per_FAT == 0 ? bootrec->ebpb.sectors_per_FAT : bootrec->bpb.sectors_per_FAT;
}

static uint32_t get_first_data_sector(fat32_bootrecord_t* bootrec) {
    return bootrec->bpb.reserved_sectors + bootrec->bpb.FATs * get_FAT_size(bootrec);
}

static uint32_t get_first_FAT_sector(fat32_bootrecord_t* bootrec) {
    return bootrec->bpb.reserved_sectors;
}

static uint32_t get_total_data_sectors(fat32_bootrecord_t* bootrec) {
    return get_total_sectors(bootrec) - get_first_data_sector(bootrec);
}

static uint32_t get_total_clusters(fat32_bootrecord_t* bootrec) {
    return get_total_data_sectors(bootrec) / bootrec->bpb.sectors_per_cluster;
}

static size_t count_free_cluster(fat32_bootrecord_t* bootrec, fs_t* fs, uint32_t first_FAT_sector) {
    size_t free_count = 0;

    // it's gonna takes along time ...
    for(size_t i = 2; i < get_total_clusters(bootrec) + 2; i++) {
        if(get_FAT_entry(bootrec, fs, first_FAT_sector, i) == 0) free_count++;
    }

    return free_count;
}

FS_ERR fat32_cut_cluster_chain(fs_t* fs, uint32_t start_cluster);
static void fix_empty_entries(fs_t* fs, uint32_t start_cluster, uint32_t end_cluster) {
    fat32_bootrecord_t* bootrec = &(fs->fat32_info.bootrec);
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = get_first_data_sector(bootrec);
    uint32_t first_FAT_sector = get_first_FAT_sector(bootrec);

    uint32_t current_cluster = start_cluster;
    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;
    uint8_t directory[cluster_size];

    // the while block below will find a chain of 0xe5 entries that end with a 0x0 entry
    // and also store the cluster that the chain start and how many clusters that the chain lied in

    bool forming_chain = false;
    uint32_t trash_start_cluster = 0;
    int cluster_count = 0;
    while(true) {
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(true, fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);

        for(int index = 0; (unsigned)index < cluster_size; index += sizeof(fat_directory_entry_t)) {
            if(directory[index] != 0x0 && directory[index] != 0xe5) {
                forming_chain = false;
                continue;
            }
            if(directory[index] == 0xe5) {
                if(!forming_chain) {
                    forming_chain = true;
                    trash_start_cluster = current_cluster;
                    cluster_count = 1;
                    continue;
                }
                cluster_count++;
                continue;
            }
            if(directory[index] == 0x0) {
                if(!forming_chain) return; // no job to be done
                // start
                goto start_deleting;
            }
        }

        if(current_cluster == end_cluster) return; // it should never reached here
        current_cluster = get_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster);
    }

    start_deleting:

    if(cluster_count > 1) {
        // the clusters after trash_start_cluster only contain 0xe5 entries and 0x0 entries
        // so we can trash them
        fat32_cut_cluster_chain(fs, trash_start_cluster);

        // also read back the "first" cluster
        uint32_t first_sector = ((trash_start_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(true, fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);
    }

    for(int i = cluster_size - sizeof(fat_directory_entry_t); i >= 0; i -= sizeof(fat_directory_entry_t)) {
        // now clear until we meet something that not 0x0 nor 0xe5
        if(directory[i] != 0x0 && directory[i] != 0xe5) break;
        directory[i] = 0x0;
    }

    // write changes
    uint32_t first_sector = ((trash_start_cluster - 2) * sectors_per_cluster) + first_data_sector;
    ata_pio_LBA28_access(false, fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);
}

static void parse_datetime(uint16_t date, uint16_t time, time_t* t) {
    struct tm _t;

    _t.tm_hour = (time >> 11);
    _t.tm_min = (time >> 5) & 0b111111;
    _t.tm_sec = (time & 0b11111) * 2;
    _t.tm_mday = date & 0b11111;
    _t.tm_mon = (date >> 5) & 0b1111;
    _t.tm_year = (date >> 9) + 1980;

    *t = mktime(&_t);
}
static void parse_timestamp(uint16_t* date, uint16_t* time, struct tm t) {
    *time = t.tm_sec/2;
    *time |= t.tm_min << 5;
    *time |= t.tm_hour << 11;

    *date = t.tm_mday;
    *date |= t.tm_mon << 5;
    *date |= (t.tm_year >= 1980 ? t.tm_year - 1980 : 0) << 9;
}


// find a free custer / clusters chain
// return start cluster address when success
// return 0 when failed to read fsinfo / cannot find a free cluster / not enough free cluster
uint32_t fat32_allocate_clusters(fs_t* fs, size_t cluster_count) {
    fat32_bootrecord_t* bootrec = &(fs->fat32_info.bootrec);
    uint32_t first_FAT_sector = get_first_FAT_sector(bootrec);

    fat32_fsinfo_t* fsinfo = &(fs->fat32_info.fsinfo);

    uint32_t current_cluster = fsinfo->available_clusters_start;
    size_t free_cluster_count = fsinfo->free_cluster_count;

    uint32_t start_cluster = 0;
    uint32_t prev_cluster = 0;

    while(cluster_count > 0) {
        uint32_t FAT_val = get_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster);

        if(FAT_val == FAT_FREE_CLUSTER) {
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
    set_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster, FAT_EOC);

    // update the fsinfo
    fsinfo->available_clusters_start = current_cluster;
    fsinfo->free_cluster_count = free_cluster_count;
    update_fsinfo(fs);

    return start_cluster;
}

// free a cluster chain
FS_ERR fat32_free_cluster_chain(fs_t* fs, uint32_t start_cluster) {
    fat32_bootrecord_t* bootrec = &(fs->fat32_info.bootrec);
    uint32_t first_FAT_sector = get_first_FAT_sector(bootrec);

    fat32_fsinfo_t* fsinfo = &(fs->fat32_info.fsinfo);

    size_t fsinfo_free_cluster_count = fsinfo->free_cluster_count;
    uint32_t fsinfo_start_cluster = fsinfo->available_clusters_start;

    uint32_t current_cluster = start_cluster;
    while(current_cluster < FAT_EOC && current_cluster != FAT_BAD_CLUSTER) {
        uint32_t FAT_val = get_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster);
        set_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster, FAT_FREE_CLUSTER);
        current_cluster = FAT_val;
        fsinfo_free_cluster_count++;
    }

    if(current_cluster == FAT_BAD_CLUSTER) return ERR_FS_BAD_CLUSTER;

    // update fsinfo
    // only update the start cluster if it is smaller
    if(start_cluster < fsinfo_start_cluster) fsinfo->available_clusters_start = start_cluster;
    fsinfo->free_cluster_count = fsinfo_free_cluster_count;
    update_fsinfo(fs);

    return ERR_FS_SUCCESS;
}

// expand a cluster chain
// return start of expanded cluster address when success
// return 0 when cannot find free cluster
uint32_t fat32_expand_cluster_chain(fs_t* fs, uint32_t end_cluster, size_t cluster_count) {
    fat32_bootrecord_t* bootrec = &(fs->fat32_info.bootrec);
    uint32_t first_FAT_sector = get_first_FAT_sector(bootrec);
    // create new chain
    uint32_t new_chain = fat32_allocate_clusters(fs, cluster_count);
    // no more free space
    if(new_chain == 0) return 0;
    // link them
    set_FAT_entry(bootrec, fs, first_FAT_sector, end_cluster, new_chain);
    return new_chain;
}

// cut a cluster chain at start_cluster
FS_ERR fat32_cut_cluster_chain(fs_t* fs, uint32_t start_cluster) {
    fat32_bootrecord_t* bootrec = &(fs->fat32_info.bootrec);
    // get next cluster
    uint32_t first_FAT_sector = get_first_FAT_sector(bootrec);
    uint32_t FAT_val = get_FAT_entry(bootrec, fs, first_FAT_sector, start_cluster);
    if(FAT_val >= FAT_EOC) return ERR_FS_SUCCESS; // no more cluster to cut
    if(FAT_val == FAT_BAD_CLUSTER) return ERR_FS_BAD_CLUSTER;

    // free them
    FS_ERR err = fat32_free_cluster_chain(fs, FAT_val);
    if(err != ERR_FS_SUCCESS) return err;
    set_FAT_entry(bootrec, fs, first_FAT_sector, start_cluster, FAT_EOC);
    return ERR_FS_SUCCESS;
}

// get the last cluster in a cluster chain
uint32_t fat32_get_last_cluster_of_chain(fs_t* fs, uint32_t start_cluster) {
    fat32_bootrecord_t* bootrec = &(fs->fat32_info.bootrec);
    uint32_t first_FAT_sector = get_first_FAT_sector(bootrec);

    uint32_t current_cluster = start_cluster;
    uint32_t FAT_val = get_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster);
    while(FAT_val < FAT_EOC && FAT_val != FAT_BAD_CLUSTER) {
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

    fat32_bootrecord_t* bootrec = &(fs->fat32_info.bootrec);
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = get_first_data_sector(bootrec);
    uint32_t first_FAT_sector = get_first_FAT_sector(bootrec);

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
        if(current_cluster >= FAT_EOC || current_cluster == FAT_BAD_CLUSTER)
            break;

        copied_current_cluster = fat32_expand_cluster_chain(fs, copied_current_cluster, 1);
    }

    return copied_start_cluster;
}

// loop through all files/entries in a dir
// call the callback when find one
FS_ERR fat32_read_dir(fs_node_t* parent, bool (*callback)(fs_node_t)) {
    if(!FS_NODE_IS_DIR(*parent)) return ERR_FS_NOT_DIR;

    fat32_bootrecord_t* bootrec = &(parent->fs->fat32_info.bootrec);
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = get_first_data_sector(bootrec);
    uint32_t first_FAT_sector = get_first_FAT_sector(bootrec);

    uint32_t current_cluster = parent->fat_cluster.start_cluster;
    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;
    uint8_t directory[cluster_size];

    bool lfn_ready = false;
    char entry_name[FILENAME_LIMIT];
    int namelen = 0;
    bool found_last_LFN_entry = false;

    while(true) {
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(true, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);

        for(unsigned int i = 0; i < cluster_size; i += 32) {
            if(directory[i] == 0x00) // no more file/directory in this dir, free entry
                return ERR_FS_EXIT_NATURALLY;
            if(directory[i] == 0xe5) continue; // free entry
            // check if it is a long file name entry
            if(directory[i+11] == 0x0f) {
                fat_lfn_entry_t* temp_lfn = (fat_lfn_entry_t*)(directory+i);
                PROCESS_LFN_ENTRY(entry_name, namelen, temp_lfn, lfn_ready, found_last_LFN_entry);
                continue;
            }

            fat_directory_entry_t* temp_dir = (fat_directory_entry_t*)(directory+i);

            if(!lfn_ready) {
                PROCESS_SFN_ENTRY(entry_name, namelen, temp_dir)
            }
            // LFN name is already ready
            fs_node_t node;
            memcpy(node.name, entry_name, namelen);

            node.fs = parent->fs;
            node.parent_node = parent;
            node.fat_cluster.start_cluster = (uint32_t)temp_dir->first_cluster_number_high << 16
                                | temp_dir->first_cluster_number_low;
            if(node.fat_cluster.start_cluster == 0 && strcmp(node.name, "..")) {
                // in linux the .. dir of a root's child directory is pointed to cluster 0
                // we need to fix it because root directory is in cluster 2
                // else we would destroy cluster 0 which contain filesystem infomation
                node.fat_cluster.start_cluster = bootrec->ebpb.rootdir_cluster;
            }
            node.creation_milisecond = temp_dir->centisecond * 10;
            parse_datetime(temp_dir->creation_date, temp_dir->creation_time, &(node.creation_timestamp));
            parse_datetime(temp_dir->last_access_date, 0, &(node.accessed_timestamp));
            parse_datetime(temp_dir->last_mod_date, temp_dir->last_mod_time, &(node.modified_timestamp));

            node.flags = FS_FLAG_VALID;
            if(temp_dir->attr & FAT_ATTR_DIRECTORY)
                node.flags |= FS_FLAG_DIRECTORY;
            if(temp_dir->attr & FAT_ATTR_HIDDEN)
                node.flags |= FS_FLAG_HIDDEN;

            node.size = temp_dir->size;
            node.fat_cluster.parent_cluster = current_cluster;
            node.fat_cluster.parent_cluster_index = i;

            // run callback
            if(!callback(node)) return ERR_FS_CALLBACK_STOP;

            // reset control var
            lfn_ready = false;
            found_last_LFN_entry = false;
        }

        uint32_t FAT_val = get_FAT_entry(bootrec, parent->fs, first_FAT_sector, current_cluster);

        if(FAT_val >= FAT_EOC)
            break; // end of cluster
        if(FAT_val == FAT_BAD_CLUSTER)
            return ERR_FS_BAD_CLUSTER;

        current_cluster = FAT_val;
    }

    return ERR_FS_EXIT_NATURALLY;
}

// read some clusters to buffer
FS_ERR fat32_read_file(fs_t* fs, uint32_t* start_cluster, uint8_t* buffer, size_t size, int cluster_offset) {
    fat32_bootrecord_t* bootrec = &(fs->fat32_info.bootrec);
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = get_first_data_sector(bootrec);
    uint32_t first_FAT_sector = get_first_FAT_sector(bootrec);

    uint32_t current_cluster = *start_cluster;
    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;
    uint8_t ext_buffer[cluster_size];

    while((unsigned)cluster_offset >= cluster_size) {
        // get next cluster
        uint32_t FAT_val = get_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster);
        if(FAT_val == FAT_BAD_CLUSTER)
            return ERR_FS_BAD_CLUSTER;

        current_cluster = FAT_val;
        cluster_offset -= cluster_size;

        // if we need to iterate more but there is no cluster then return
        if(FAT_val >= FAT_EOC && (unsigned)current_cluster >= cluster_size)
            return ERR_FS_EOF;
    }

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

        if(FAT_val >= FAT_EOC)
            return ERR_FS_EOF;
        if(FAT_val == FAT_BAD_CLUSTER)
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

        if(FAT_val >= FAT_EOC)
            return ERR_FS_EOF;
        if(FAT_val == FAT_BAD_CLUSTER)
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
    fat32_bootrecord_t* bootrec = &(fs->fat32_info.bootrec);
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = get_first_data_sector(bootrec);
    uint32_t first_FAT_sector = get_first_FAT_sector(bootrec);

    uint32_t current_cluster = *start_cluster;
    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;
    uint8_t ext_buffer[cluster_size];

    while((unsigned)cluster_offset >= cluster_size) {
        // get next cluster
        uint32_t FAT_val = get_FAT_entry(bootrec, fs, first_FAT_sector, current_cluster);

        if(FAT_val >= FAT_EOC)
            return ERR_FS_EOF;
        if(FAT_val == FAT_BAD_CLUSTER)
            return ERR_FS_BAD_CLUSTER;

        current_cluster = FAT_val;
        cluster_offset -= cluster_size;
    }

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

        if(FAT_val >= FAT_EOC)
            break;
        if(FAT_val == FAT_BAD_CLUSTER)
            return ERR_FS_BAD_CLUSTER;

        current_cluster = FAT_val;
        write_time++;
        size -= cluster_size;
    }

    if(cluster_offset > 0) buffer -= cluster_size - cluster_offset;

    *start_cluster = current_cluster;
    return ERR_FS_SUCCESS;
}

// add a new entry to parent
// return valid node when success
// return invalid node when cannot find a free cluster / an entry with the same name has already exists
fs_node_t fat32_add_entry(fs_node_t* parent, char* name, uint32_t start_cluster, uint8_t attr, size_t size) {
    fs_node_t node;
    node.flags = 0;

    fat32_bootrecord_t* bootrec = &(parent->fs->fat32_info.bootrec);
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = get_first_data_sector(bootrec);
    uint32_t first_FAT_sector = get_first_FAT_sector(bootrec);

    uint32_t current_cluster = parent->fat_cluster.start_cluster;
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
    char entry_name[FILENAME_LIMIT];
    int namelen = 0;
    bool found_last_LFN_entry = false;

    // find a empty space to put the new entry on
    // also check for duplication
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
                fat_lfn_entry_t* temp_lfn = (fat_lfn_entry_t*)(directory+start_index);
                PROCESS_LFN_ENTRY(entry_name, namelen, temp_lfn, lfn_ready, found_last_LFN_entry);
                continue;
            }

            fat_directory_entry_t* temp_dir = (fat_directory_entry_t*)(directory+start_index);
            if(!lfn_ready) {
                PROCESS_SFN_ENTRY(entry_name, namelen, temp_dir)
            }

            // found duplication
            if(strcmp_case_insensitive(entry_name, name))
                return node;

            lfn_ready = false;
            found_last_LFN_entry = false;
        }
        if(found) break;

        uint32_t FAT_val = get_FAT_entry(bootrec, parent->fs, first_FAT_sector, current_cluster);

        if(FAT_val >= FAT_EOC) { // end of cluster
            // add a new cluster
            current_cluster = fat32_expand_cluster_chain(parent->fs, current_cluster, 1);
            // no more space
            if(current_cluster == 0) return node;
            new_cluster_created = true;
            continue;
        }

        current_cluster = FAT_val;
    }

    // override namelen
    namelen = strlen(name);

    // calculate the number of LFN entries will be generated
    int lfn_entry_count = (namelen + 13) / 13; // include the null char
    if(dotdot_entry) lfn_entry_count = 0; // no need to create LFN entry for . and ..

    // calculate how many clusters are needed to create
    int estimated_new_cluster_count = (lfn_entry_count - (cluster_size - start_index)/sizeof(fat_lfn_entry_t) + 17) / // TODO: wth is 17
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

    node.parent_node = parent;
    node.fs = parent->fs;
    node.parent_node = parent;
    node.fat_cluster.start_cluster = start_cluster;
    node.size = size;

    node.flags = FS_FLAG_VALID;
    if(attr & FAT_ATTR_DIRECTORY)
        node.flags |= FS_FLAG_DIRECTORY;
    if(attr & FAT_ATTR_HIDDEN)
        node.flags |= FS_FLAG_HIDDEN;

    node.creation_milisecond = (clock() % CLOCKS_PER_SEC) * 1000 / CLOCKS_PER_SEC;
    node.creation_timestamp = time(NULL);
    node.modified_timestamp = 0;
    node.accessed_timestamp = 0;

    // add entries
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
            dir_entry->centisecond = node.creation_milisecond / 10;

            uint16_t date_dump, time_dump;

            parse_timestamp(&date_dump, &time_dump, gmtime(&(node.creation_timestamp)));
            dir_entry->creation_time = time_dump;
            dir_entry->creation_date = date_dump;

            parse_timestamp(&date_dump, &time_dump, gmtime(&(node.accessed_timestamp)));
            dir_entry->last_access_date = date_dump;

            dir_entry->first_cluster_number_high = start_cluster >> 16;

            parse_timestamp(&date_dump, &time_dump, gmtime(&(node.modified_timestamp)));
            dir_entry->last_mod_time = time_dump;
            dir_entry->last_mod_date = date_dump;

            dir_entry->first_cluster_number_low = start_cluster & 0xffff;
            dir_entry->size = size;

            node.fat_cluster.parent_cluster = current_cluster;
            node.fat_cluster.parent_cluster_index = start_index;
        }
        done_parsing_name:

        // make changes
        start_index += sizeof(fat_directory_entry_t);
        if(start_index >= cluster_size) { // current cluster is exceeded
            uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
            ata_pio_LBA28_access(false, parent->fs->partition.LBA_start + first_sector,
                    sectors_per_cluster, directory);
            // the next cluster is always valid because we have created it before
            current_cluster = get_FAT_entry(bootrec, parent->fs, first_FAT_sector, current_cluster);
            start_index = 0;
        }
    }

    if(start_index != 0) {
        // clear the remain entry
        for(int i = start_index; (unsigned)i < cluster_size; i += sizeof(fat_directory_entry_t))
            directory[i] = 0x0;

        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(false, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);
    }

    return node;
}

// remove an entry from parent
FS_ERR fat32_remove_entry(fs_node_t* parent, fs_node_t remove_node, bool remove_content) {
    if(strcmp(remove_node.name, ".") || strcmp(remove_node.name, ".."))
        return ERR_FS_FAILED;

    fat32_bootrecord_t* bootrec = &(parent->fs->fat32_info.bootrec);
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = get_first_data_sector(bootrec);
    uint32_t first_FAT_sector = get_first_FAT_sector(bootrec);

    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;
    uint8_t directory[cluster_size];

    int namelen; (void)(namelen); // avoid unused param
    char entry_name[FILENAME_LIMIT];
    if(FS_NODE_IS_DIR(remove_node) && remove_content) {
        // check if it has any child entry
        uint32_t first_sector = ((remove_node.fat_cluster.start_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(true, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);

        for(unsigned int i = 0; i < cluster_size; i += 32) {
            if(directory[i] == 0x00) break; // no entry, yay
            if(directory[i] == 0xe5) return ERR_FS_DIR_NOT_EMPTY; // because it means the next entry is not clear
            if(directory[i+11] == 0x0f) // if it has any LFN entry that mean there is a entry
                return ERR_FS_DIR_NOT_EMPTY;

            fat_directory_entry_t* temp_dir = (fat_directory_entry_t*)(directory+i);

            // at this point this entry should be a SFN
            PROCESS_SFN_ENTRY(entry_name, namelen, temp_dir)
            // if it is not . or .. entry then there is a child entry
            if(!strcmp(entry_name, ".") && !strcmp(entry_name, ".."))
                return ERR_FS_DIR_NOT_EMPTY;
        }
    }

    // now we are good to remove the entry

    int lfn_entry_count = (strlen(remove_node.name) + 12) / 13;

    int node_index = remove_node.fat_cluster.parent_cluster_index;
    uint32_t current_cluster = remove_node.fat_cluster.parent_cluster;

    // check if the next entry is clear
    int clear_val;
    if((unsigned)node_index + 32 < cluster_size) {
        // read current cluster
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(true, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);
        if(directory[node_index+32] == 0x0)
            clear_val = 0x0;
        else clear_val = 0xe5;
    }
    else {
        // check next cluster to see if it is clear or not
        uint32_t FAT_val = get_FAT_entry(bootrec, parent->fs, first_FAT_sector, current_cluster);

        if(FAT_val >= FAT_EOC || FAT_val == FAT_BAD_CLUSTER) {
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
    if(remove_content) fat32_free_cluster_chain(parent->fs, remove_node.fat_cluster.start_cluster);

    // calculate the start of the chain
    int start_index = node_index - lfn_entry_count * sizeof(fat_directory_entry_t);
    int go_back = 0;
    if(start_index < 0) {
        go_back = -start_index;
        start_index = 0;
    }
    int entry_cnt = (node_index + sizeof(fat_directory_entry_t) - start_index) / sizeof(fat_directory_entry_t);

    if(clear_val == 0xe5 || (clear_val == 0x0 && go_back == 0)) {
        // "delete" directory entries in the current cluster, just set the first byte of that entry to 0x0 or 0xe5
        for(int i = 0; i < entry_cnt; i++)
            directory[start_index + i*sizeof(fat_directory_entry_t)] = clear_val;

        // write
        int first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(false, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);
    }
    if(go_back > 0) {
        start_index = cluster_size - go_back;
        entry_cnt = go_back / sizeof(fat_directory_entry_t);

        // find previous cluster of current cluster
        uint32_t last_cluster = parent->fat_cluster.start_cluster;
        uint32_t FAT_val = parent->fat_cluster.start_cluster;
        while(FAT_val != current_cluster) {
            last_cluster = FAT_val;
            FAT_val = get_FAT_entry(bootrec, parent->fs, first_FAT_sector, FAT_val);
        }

        // read the previous cluster
        int first_sector = ((last_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(true, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);

        // "delete"
        for(int i = 0; i < entry_cnt; i++)
            directory[start_index + i*sizeof(fat_directory_entry_t)] = clear_val;

        // write
        ata_pio_LBA28_access(false, parent->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);

        if(clear_val == 0x0) {
            fat32_free_cluster_chain(parent->fs, current_cluster);
            set_FAT_entry(bootrec, parent->fs, first_FAT_sector, last_cluster, FAT_EOC);
        }
    }

    if(clear_val == 0x0)
        fix_empty_entries(parent->fs, parent->fat_cluster.start_cluster, remove_node.fat_cluster.parent_cluster);

    return ERR_FS_SUCCESS;
}

// update node entry infomation on disk
FS_ERR fat32_update_entry(fs_node_t* node) {
    fat32_bootrecord_t* bootrec = &(node->fs->fat32_info.bootrec);
    uint32_t sectors_per_cluster = bootrec->bpb.sectors_per_cluster;
    uint32_t first_data_sector = get_first_data_sector(bootrec);

    uint32_t cluster_size = sectors_per_cluster * bootrec->bpb.bytes_per_sector;
    uint8_t directory[cluster_size];

    uint32_t first_sector = ((node->fat_cluster.parent_cluster - 2) * sectors_per_cluster) + first_data_sector;
    ata_pio_LBA28_access(true, node->fs->partition.LBA_start + first_sector, sectors_per_cluster, directory);

    fat_directory_entry_t* dir = (fat_directory_entry_t*)(directory+node->fat_cluster.parent_cluster_index);
    // TODO: check if the entry is really exists
    
    uint16_t date_dump, time_dump;

    parse_timestamp(&date_dump, &time_dump, gmtime(&(node->accessed_timestamp)));
    dir->last_access_date = date_dump;
    parse_timestamp(&date_dump, &time_dump, gmtime(&(node->modified_timestamp)));
    dir->last_mod_time = time_dump;
    dir->last_mod_date = date_dump;

    dir->attr = 0;
    if(node->flags & FS_FLAG_DIRECTORY)
        dir->attr |= FAT_ATTR_DIRECTORY;
    if(node->flags & FS_FLAG_HIDDEN)
        dir->attr |= FAT_ATTR_HIDDEN;
    dir->size = node->size;
    // name updating is very problematic
    // so if you want to update the name
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
    created_dir.flags = 0;

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
    if(!FS_NODE_IS_VALID(created_dir)) return created_dir;

    // create the '..' and the '.' dir
    // in linux you need to create them
    // so that `ls` will not show input/output error
    fs_node_t dot_dir = fat32_add_entry(&created_dir, ".", start_cluster, FAT_ATTR_DIRECTORY, 0);
    if(!FS_NODE_IS_VALID(dot_dir)) return dot_dir;

    fs_node_t dotdot_dir = fat32_add_entry(&created_dir, "..", parent->fat_cluster.start_cluster, FAT_ATTR_DIRECTORY, 0);
    if(!FS_NODE_IS_VALID(dotdot_dir)) return dotdot_dir;

    return created_dir;
}

// initialize FAT 32
// return the root node
FS_ERR fat32_init(fs_t* fs, partition_entry_t part) {
    fs->partition = part;
    fs->type = FS_FAT32;

    // parsing info tables
    get_bootrec(part, (uint8_t*)(&(fs->fat32_info.bootrec)));
    fat32_bootrecord_t* bootrec = &(fs->fat32_info.bootrec);
    get_fsinfo(part, bootrec, (uint8_t*)(&(fs->fat32_info.fsinfo)));
    fat32_fsinfo_t* fsinfo = &(fs->fat32_info.fsinfo);

    // check fsinfo
    if(fsinfo->lead_signature != 0x41615252
            || fsinfo->mid_signature != 0x61417272
            || fsinfo->trail_signature != 0xaa550000)
        return ERR_FS_INVALID_FSINFO;

    bool fsinfo_update = false;
    if(fsinfo->available_clusters_start == 0xffffffff) {
        fsinfo->available_clusters_start = 2;
        fsinfo_update = true;
    }
    if(fsinfo->free_cluster_count == 0xffffffff) {
        uint32_t first_FAT_sector = get_first_FAT_sector(bootrec);
        fsinfo->free_cluster_count = count_free_cluster(bootrec, fs, first_FAT_sector);
        fsinfo_update = true;
    }
    if(fsinfo_update) update_fsinfo(fs);

    fs->root_node.fs = fs;
    fs->root_node.fat_cluster.start_cluster = bootrec->ebpb.rootdir_cluster;
    fs->root_node.parent_node = NULL;
    fs->root_node.flags = FS_FLAG_VALID | FS_FLAG_DIRECTORY;
    fs->root_node.name[0] = '/';
    fs->root_node.name[1] = '\0';

    return ERR_FS_SUCCESS;
}
