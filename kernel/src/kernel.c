#include "system.h"
#include "mem.h"
#include "tty.h"
#include "kbd.h"
#include "ata.h"
#include "filesystem.h"

#include "multiboot.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "debug.h"

const unsigned int TIMER_PHASE = 100;

char freebuff[512];

int FS_ID = 0;
fs_node_t current_node;

void print_typed_char(key_t k) {
    if(k.released) return;

    if(k.mapped == '\b') {
        tty_set_cursor(tty_get_cursor() - 1); // move back
        tty_print_char(' ', -1, 0, false); // delete printed char
    }
    else {
        putchar(k.mapped);
    }
}

extern uint32_t startkernel;
extern uint32_t endkernel;
void mem_init(multiboot_info_t* mbd) {
    // TODO: sort the mmmt

    // print mem info
    puts("--[MEMORY]-+--------------------+-------------------");
    puts("base addr  | length             | type");
    puts("-----------+--------------------+-------------------");

    for(unsigned int i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(mbd->mmap_addr + i);

        printf("0x%s", itoa(mmmt->addr, freebuff, 16));
        for(int i = 0; i < 8 - (int)strlen(freebuff); i++) putchar(' ');
        printf(" | ");

        printf("%s KiB", itoa(mmmt->len/1024, freebuff, 10));
        for(int i = 0; i < 14 - (int)strlen(freebuff); i++) putchar(' ');
        printf(" | ");

        switch(mmmt->type) {
            case MULTIBOOT_MEMORY_AVAILABLE:
                printf("usable");
                break;
            case MULTIBOOT_MEMORY_RESERVED:
                printf("reserved");
                break;
            case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                printf("ACPI reclamable");
                break;
            case MULTIBOOT_MEMORY_NVS:
                printf("ACPI non-volatile");
                break;
            case MULTIBOOT_MEMORY_BADRAM:
                printf("bad mem");
                break;
        }

        putchar('\n');
    }
    puts("-----------+--------------------+-------------------");

    // get memsize
    size_t memsize = 0;
    for(unsigned int i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(mbd->mmap_addr + i);
        // ignore memory higher than 4GiB
        // the map is sorted so we can break to ignore all data next to it
        if((mmmt->addr >> 32) > 0 || (mmmt->len  >> 32) > 0) break;
        // probaly bugs
        if(i > 0 && mmmt->addr == 0) continue;

        memsize += mmmt->len;
    }
    pmmngr_init(endkernel + 1, memsize);

    // init regions
    for(unsigned int i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(mbd->mmap_addr + i);

        if((mmmt->addr >> 32) > 0 || (mmmt->len >> 32) > 0) break;
        if(i > 0 && mmmt->addr == 0) continue;

        // if not usable or ACPI reclaimable then skip
        if(mmmt->type != MULTIBOOT_MEMORY_AVAILABLE && mmmt->type != MULTIBOOT_MEMORY_ACPI_RECLAIMABLE)
            continue;

        pmmngr_init_region(mmmt->addr, mmmt->len);
    }

    // deinit regions
    pmmngr_deinit_region(startkernel, endkernel - startkernel + 1); // kernel
    pmmngr_deinit_region(endkernel, pmmngr_get_size()/MMNGR_BLOCK_SIZE); // pmmngr

    pmmngr_update_usage(); // always run this after init and deinit regions
    print_debug(LT_OK, "initialized pmmngr with %d MiB\n", pmmngr_get_free_size()/1024/1024);

    // vmmngr init
    MEM_ERR merr = vmmngr_init();
    if(merr != ERR_MEM_SUCCESS) {
        print_debug(LT_CR, "error while enabling paging. system halted. error code %x", merr);
        SYS_HALT();
    }
    print_debug(LT_OK, "paging enabled\n");
}
void disk_init() {
    uint16_t IDENTIFY_returned[256];
    if(!ata_pio_init(IDENTIFY_returned)) {
        print_debug(LT_WN, "failed to initialize ATA PIO mode\n");
        return;
    }

    print_debug(LT_OK, "ATA PIO mode initialized\n");

    print_debug(LT_IF, "    LBA48 mode: %s\n", (IDENTIFY_returned[83] & 0x400) ? "yes" : "no");

    if(IDENTIFY_returned[88]) {
        uint8_t supported_UDMA = IDENTIFY_returned[88] & 0xff;
        uint8_t active_UDMA = IDENTIFY_returned[88] >> 8;
        print_debug(LT_IF, "    supported UDMA mode:");
        for(int i = 0; i < 8; i++)
            if(supported_UDMA & (1 << i)) printf(" %d", i+1);
        putchar('\n');
        print_debug(LT_IF, "    active UDMA mode:");
        for(int i = 0; i < 8; i++)
            if(active_UDMA & (1 << i)) printf(" %d", i+1);
        putchar('\n');
    }

    bool mbr_ok = mbr_load();
    if(!mbr_ok) {
        print_debug(LT_ER, "cannot load MBR\n");
        return;
    }
    print_debug(LT_OK, "MBR loaded\n");

    for(int i = 0; i < 4; i++) {
        partition_entry_t part = mbr_get_partition_entry(i);
        if(part.sector_count == 0) continue;

        print_debug(LT_IF, "partition table %d info:\n", i+1);
        print_debug(LT_IF, "    partition type: 0x%x\n", part.partition_type);
        print_debug(LT_IF, "    drive attribute: %s\n",
                        part.drive_attribute == 0x80 ? "active/bootable" :
                        part.drive_attribute == 0x00 ? "inactive" : "invalid");
        print_debug(LT_IF, "    LBA start: 0x%x\n", part.LBA_start);
        print_debug(LT_IF, "    sector count: %d\n", part.sector_count);

        print_debug(LT_IF, "    fs type: ");
        switch(fs_detect(part)) {
            case FS_EMPTY:
                puts("unknown");
                break;
            case FS_FAT32:
                puts("FAT 32");
                current_node = fat32_init(part, FS_ID);
                print_debug(LT_OK, "initialized FAT 32 filesystem in partition %d\n", i+1);
                break;
            case FS_EXT2:
                puts("ext2");
                print_debug(LT_WN, "EXT2 filesystem in partition %d is not implemented, the partition will be ignored\n", i+1);
                break;
        }
    }
}

int indent_level = 0;
int max_depth = 2;
bool list_dir(fs_node_t node) {
    if(indent_level >= max_depth) return true;
    indent_level++;

    bool is_dir = node.attr == 0x10;

    for(int i = 0; i < indent_level-1; i++)
        printf("|   ");
    printf("|---");
    printf("%s (%s)\n", node.name, is_dir ? "directory" : "file");

    if(is_dir && !strcmp(node.name, ".") && !strcmp(node.name, ".."))
        fs_list_dir(&node, list_dir);

    indent_level--;

    return true;
}

void kmain(multiboot_info_t* mbd, unsigned int magic) {
    // greeting msg to let us know we are in the kernel
    tty_set_attr(LIGHT_CYAN);  puts("hello");
    tty_set_attr(LIGHT_GREEN); printf("this is ");
    tty_set_attr(LIGHT_RED);   puts("the kernel");
    tty_set_attr(LIGHT_GREY);

    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        print_debug(LT_CR, "invalid magic number. system halted\n");
        SYS_HALT();
    }
    if(!(mbd->flags >> 6 & 0x1)) {
        print_debug(LT_CR, "invalid memory map given by GRUB. system halted\n");
        SYS_HALT();
    }
    mem_init(mbd);

    gdt_init();
    idt_init();
    irq_init();

    // start receiving interrupts
    // after initialize all interrupt handlers
    asm volatile("sti");

    disk_init();

    kbd_init();

    // make cursor slimmer
    tty_enable_cursor(13, 14);

    set_key_listener(print_typed_char);

    print_debug(LT_IF, "done initializing\n");

    if(current_node.valid) {
        fs_node_t kern_test_node = fs_find(&current_node, "touch-grass-dude");
        bool just_created_file = false;
        if(!kern_test_node.valid) {
            kern_test_node = fs_touch(&current_node, "touch-grass-dude");
            just_created_file = true;
            puts("created touch-grass-dude");
        }

        if(kern_test_node.valid) {
            if(just_created_file) {
                FILE kern_test_file_write = file_open(&kern_test_node, FILE_WRITE);

                char msg1[] = "hi guys im the kernel. please touch grass.\n";
                char msg2[] = "ty! have a great day.\n";
                file_write(&kern_test_file_write, (uint8_t*)msg1, strlen(msg1));
                file_write(&kern_test_file_write, (uint8_t*)msg2, strlen(msg2));
                file_close(&kern_test_file_write);
            }
            else {
                FILE kern_test_file_write = file_open(&kern_test_node, FILE_APPEND);

                char msg3[] = "did you touch them?\n";
                file_write(&kern_test_file_write, (uint8_t*)msg3, strlen(msg3));
                file_close(&kern_test_file_write);
                puts("touch-grass-dude edited");
            }
            
            FILE kern_test_file_read = file_open(&kern_test_node, FILE_READ);

            puts("reading touch-grass-dude");
            puts("---------------------------------------");
            uint8_t chr = 'n';
            while(file_read(&kern_test_file_read, &chr, 1) != ERR_FS_EOF)
                putchar(chr);
            puts("---------------------------------------");
        }

        fs_node_t kern_test_move_node = fs_find(&current_node, "moving-grass-dude");
        if(!kern_test_move_node.valid) {
            kern_test_move_node = fs_touch(&current_node, "moving-grass-dude");
            puts("created moving-grass-dude");

            char strrr[] = "im a test file\n";
            FILE temp = file_open(&kern_test_move_node, FILE_WRITE);
            file_write(&temp, (uint8_t*)strrr, strlen(strrr));
            file_close(&temp);
        }

        if(kern_test_move_node.valid) {
            char fname[] = "no----moving-grass-dude";
            fs_node_t copied;
            FS_ERR err = 0;
            int i = 1;
            while(err != ERR_FS_SUCCESS) {
                itoa(i, freebuff, 10);
                memcpy(fname+3, freebuff, strlen(freebuff));
                err = fs_copy(&kern_test_move_node, &current_node, &copied, fname);
                i++;
            }

            FILE copied_file = file_open(&copied, FILE_APPEND);
            file_write(&copied_file, (uint8_t*)fname, strlen(fname));
            file_close(&copied_file);
        }

        puts("root directory");
        fs_list_dir(&current_node, list_dir);
    }

    while(true) {
        SYS_SLEEP();
    }
}
