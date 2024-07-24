#include "filesystem.h"
#include "ata.h"

#include "string.h"
#include "debug.h"

// the table will be used very frequently
// so it is a good idea to only declare it once
static uint8_t FAT[512];
// store the last sector that is read to FAT
static int last_read_FAT_sector = -1;

static void parse_lfn(fat_lfn_entry_t* lfn, char* buff, int offset, int* cnt) {
    *cnt = 0;

    // TODO: unicode support
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

    bool lfn_ready = false;
    fat_lfn_entry_t temp_lfn;
    char lfn_name[FILENAME_LIMIT];
    int namelen = 0;
    bool found_last_LFN_entry = false;
    fat_directory_entry_t temp_dir;

    while(true) {
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        ata_pio_LBA28_access(true, parent->fs->partition.LBA_start + first_sector,
                sectors_per_cluster, directory);

        for(unsigned int i = 0; i < cluster_size; i += 32) {
            if(directory[i] == 0x00) // no more file/directory in this dir, free entry
                return true;
            if(directory[i] == 0xe5) continue; // free entry
            // check if it is a long file name entry
            if(directory[i+11] == 0x0f) {
                // TODO: check checksum (optional)
                // to be sure that the long name and the short name matchs
                // they will not match if the short entries are edited in
                // a computer which does not support LFN
                // which is very unlikely to occure

                lfn_ready = true;

                memcpy(&temp_lfn, directory + i, sizeof(fat_lfn_entry_t));

                int order = temp_lfn.order;
                bool last_LFN_entry = false;
                if(!found_last_LFN_entry) {
                    found_last_LFN_entry = true;
                    last_LFN_entry = true;
                    // TODO: check if the 6th bit (0x40) is originally 0 or 1
                    // can just check the next entry but it will be very complex
                    // if the next entry is in another cluster
                    // right now just assume that all entries order are less than 0x40
                    order &= 0x3f;
                }

                int cnt;
                int offset = (order - 1) * 13;
                parse_lfn(&temp_lfn, lfn_name, offset, &cnt);
                if(last_LFN_entry) {
                    namelen = offset + cnt;
                    if(namelen >= FILENAME_LIMIT) {
                        namelen = FILENAME_LIMIT;
                        // the name exceeded the limit
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
            found_last_LFN_entry = false;
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
    uint32_t new_chain = fat32_allocate_clusters(fs, cluster_count);
    // no more free space
    if(new_chain == 0) return 0;
    // link them
    set_FAT_entry(bootrec, fs, first_FAT_sector, end_cluster, new_chain);
    return new_chain;
}

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

    unsigned int start_index;
    bool new_cluster_created = false;
    while(true) {
        uint32_t first_sector = ((current_cluster - 2) * sectors_per_cluster) + first_data_sector;
        if(new_cluster_created) {
            // clear the new cluster
            // this will guarantee us to find a free entry
            memset(directory, 0, cluster_size);
            ata_pio_LBA28_access(false, parent->fs->partition.LBA_start + first_sector,
                    sectors_per_cluster, directory);
        }
        else
            ata_pio_LBA28_access(true, parent->fs->partition.LBA_start + first_sector,
                    sectors_per_cluster, directory);

        bool found = false;
        for(start_index = 0; start_index < cluster_size; start_index += 32) {
            // TODO: check if the `name` entry is already exists

            if(directory[start_index] == 0x00) {
                found = true;
                break;
            }
        }
        if(found) break;

        uint32_t FAT_val = get_FAT_entry(bootrec, parent->fs, first_FAT_sector, current_cluster);

        if(FAT_val >= 0x0ffffff8) { // end of cluster
            // add a new cluster
            current_cluster = fat32_expand_clusters_chain(parent->fs, current_cluster, 1);
            // nomore space
            if(current_cluster == 0) return node;
            new_cluster_created = true;
            continue;
        }

        current_cluster = FAT_val;
    }

    // we found a start index in current cluster

    // remove leading spaces, trailing spaces and trailing periods
    while(name[0] == ' ') name++;
    int namelen = strlen(name);
    while(name[namelen-1] == ' ' || name[namelen-1] == '.') {
        name[namelen-1] = '\0';
        namelen--;
    }

    // calculate the number of LFN entries will be generated
    int lfn_entry_count = (namelen + 13) / 13; // include null char
    // calculate how many clusters are needed to create
    int estimated_new_cluster_count = (lfn_entry_count - (cluster_size - start_index)/sizeof(fat_lfn_entry_t) + 17) /
                                      (cluster_size / sizeof(fat_lfn_entry_t)); // ididthemath
    if(estimated_new_cluster_count > 0) {
        // try to expand
        uint32_t valid = fat32_expand_clusters_chain(parent->fs, current_cluster, estimated_new_cluster_count);
        if(!valid) return node;
    }

    memcpy(node.name, name, namelen+1);

    char shortname[11];
    // generate short name
    // it's only needed to be different from the others short name entries
    // so we can actually using a random number generator to do this
    // TODO: generate properly
    memcpy(shortname, name, 11);
    uint8_t checksum = gen_checksum(shortname);

    node.valid = true;
    node.parent_node = parent;
    node.fs = parent->fs;
    node.parent_node = parent;
    node.start_cluster = start_cluster;
    node.attr = attr;
    node.size = size;
    // TODO: set correct time attr
    // these are dummy value
    node.centisecond = 102;
    node.creation_time = 0b0010110011101111;
    node.creation_date = 0b0101100110001101;
    node.last_access_date = 0b0101100110001101;
    node.last_mod_time = 0b0010110011101111;
    node.last_mod_date = 0b0101100110001101;

    // add FLN entries
    fat_lfn_entry_t temp_lfn;
    for(int i = lfn_entry_count; i >= 0; i--) {
        if(i > 0) { // long file name entry
            temp_lfn.order = i;
            if(i == lfn_entry_count) // "first" LFN entry
                temp_lfn.order |= 0x40;
            temp_lfn.attr = 0x0f;
            temp_lfn.checksum = checksum;

            // TODO: unicode support
            // not very likely to do it
            // because who tf naming using emoji ???
            for(int j = 0; j < 5; j++) {
                temp_lfn.chars_1[j] = name[(i-1) * 13 + j];
                if(temp_lfn.chars_1[j] == '\0') goto done_parsing_name;
            }
            for(int j = 0; j < 6; j++) {
                temp_lfn.chars_2[j] = name[(i-1) * 13 + j + 5];
                if(temp_lfn.chars_2[j] == '\0') goto done_parsing_name;
            }
            for(int j = 0; j < 2; j++) {
                temp_lfn.chars_3[j] = name[(i-1) * 13 + j + 5 + 6];
                if(temp_lfn.chars_3[j] == '\0') goto done_parsing_name;
            }
            // note that we have clear the temp_lfn with 0's
            // so it is automatically null terminated
        
            done_parsing_name:
            memcpy(directory + start_index, (uint8_t*)(&temp_lfn), sizeof(fat_lfn_entry_t));
            // clear
            memset((char*)(&temp_lfn), 0, sizeof(fat_lfn_entry_t));
        }
        else if(i == 0) { // directory entry
            fat_directory_entry_t dir_entry;
            memcpy(dir_entry.name, shortname, 11);
            dir_entry.attr = attr;
            dir_entry.centisecond = node.centisecond;
            dir_entry.creation_time = node.creation_time;
            dir_entry.creation_date = node.creation_date;
            dir_entry.last_access_date = node.last_access_date;
            dir_entry.first_cluster_number_high = start_cluster >> 16;
            dir_entry.last_mod_time = node.last_mod_time;
            dir_entry.last_mod_date = node.last_mod_date;
            dir_entry.first_cluster_number_low = start_cluster & 0xffff;
            dir_entry.size = size;

            memcpy(directory + start_index, (uint8_t*)(&dir_entry), sizeof(fat_directory_entry_t));
        }

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
        ata_pio_LBA28_access(false, parent->fs->partition.LBA_start + first_sector,
                sectors_per_cluster, directory);
    }

    return node;
}

fs_node_t fat32_mkdir(fs_node_t* parent, char* name, uint32_t start_cluster, uint8_t attr) {
    fs_node_t created_dir = fat32_add_entry(parent, name, start_cluster, attr | NODE_DIRECTORY, 0);
    if(!created_dir.valid) return created_dir;

    // create the '..' and the '.' dir
    fs_node_t dot_dir = fat32_add_entry(parent, ".", start_cluster, NODE_DIRECTORY, 0);
    if(!dot_dir.valid) return dot_dir;
    fs_node_t dotdot_dir = fat32_add_entry(parent, "..", parent->start_cluster, NODE_DIRECTORY, 0);
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
    rootnode.attr = NODE_DIRECTORY;
    rootnode.valid = true;

    // TODO: verify information in fsinfo
    
    fs.root_node = rootnode;
    return rootnode;
}
