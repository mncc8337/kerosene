#include "multiboot.h"
#include "system.h"
#include "kpanic.h"
#include "tty.h"
#include "mem.h"
#include "ata.h"
#include "kbd.h"
#include "timer.h"
#include "syscall.h"
#include "filesystem.h"

#include "stdio.h"
#include "debug.h"

#include "kshell.h"

char freebuff[512];

int FS_ID = 0;
fs_t* fs;

extern uint32_t startkernel;
extern uint32_t endkernel;
void mem_init(uint32_t mmap_addr, uint32_t mmap_length) {
    // get memsize
    size_t memsize = 0;
    for(unsigned int i = 0; i < mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(mmap_addr + i);
        // ignore memory higher than 4GiB
        if((mmmt->addr >> 32) > 0 || (mmmt->len  >> 32) > 0) continue;
        // probaly bugs
        if(i > 0 && mmmt->addr == 0) continue;

        memsize += mmmt->len;
    }
    pmmngr_init(endkernel+1, memsize);

    // init regions
    for(unsigned int i = 0; i < mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(mmap_addr + i);

        if((mmmt->addr >> 32) > 0 || (mmmt->len >> 32) > 0) continue;
        if(i > 0 && mmmt->addr == 0) continue;

        // if not usable or ACPI reclaimable then skip
        if(mmmt->type != MULTIBOOT_MEMORY_AVAILABLE && mmmt->type != MULTIBOOT_MEMORY_ACPI_RECLAIMABLE)
            continue;

        pmmngr_init_region(mmmt->addr, mmmt->len);
    }

    // deinit regions
    pmmngr_deinit_region(startkernel, endkernel - startkernel + 1); // kernel
    pmmngr_deinit_region(endkernel+1, pmmngr_get_size()/MMNGR_BLOCK_SIZE); // pmmngr

    pmmngr_update_usage(); // always run this after init and deinit regions
    print_debug(LT_OK, "initialised pmmngr with %d MiB\n", pmmngr_get_free_size()/1024/1024);

    // vmmngr init
    MEM_ERR merr = vmmngr_init();
    if(merr != ERR_MEM_SUCCESS) {
        print_debug(LT_CR, "error while enabling paging. system halted. error code %x", merr);
        kpanic();
    }
    print_debug(LT_OK, "paging enabled\n");
}
void disk_init() {
    if(!ata_pio_init((uint16_t*)freebuff)) {
        print_debug(LT_WN, "failed to initialise ATA PIO mode\n");
        return;
    }

    print_debug(LT_OK, "ATA PIO mode initialised\n");

    bool mbr_ok = mbr_load();
    if(!mbr_ok) {
        print_debug(LT_ER, "cannot load MBR\n");
        return;
    }
    print_debug(LT_OK, "MBR loaded\n");

    for(int i = 0; i < 4; i++) {
        partition_entry_t part = mbr_get_partition_entry(i);
        if(part.sector_count == 0) continue;

        FS_ERR err;
        switch(fs_detect(part)) {
            case FS_EMPTY:
                break;
            case FS_FAT32:
                err = fat32_init(part, FS_ID);
                if(err != ERR_FS_SUCCESS) {
                    print_debug(LT_ER, "failed to initialize FAT32 filesystem in partition %d. error code %d\n", i+1, err);
                    break;
                }
                print_debug(LT_OK, "initialised FAT32 filesystem in partition %d\n", i+1);
                fs = fs_get(FS_ID);
                break;
            case FS_EXT2:
                print_debug(LT_WN, "EXT2 filesystem in partition %d is not implemented, the partition will be ignored\n", i+1);
                break;
        }
    }

    if(fs->root_node.valid) {
        fs->root_node.name[0] = '/';
        fs->root_node.name[1] = '\0';
    }
}

void kmain(multiboot_info_t* mbd, unsigned int magic) {
    // disable interrupts at the start to set up things
    asm volatile("cli");

    // greeting msg to let us know we are in the kernel
    tty_set_attr(LIGHT_CYAN);  puts("hello");
    tty_set_attr(LIGHT_GREEN); printf("this is ");
    tty_set_attr(LIGHT_RED);   puts("sos kernel!");
    tty_set_attr(LIGHT_GREY);
    printf("build datetime: %s, %s\n", __TIME__, __DATE__);

    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        print_debug(LT_CR, "invalid magic number. system halted\n");
        kpanic();
    }

    if(mbd->flags & (1 << 9))
        print_debug(LT_IF, "using %s bootloader\n", mbd->boot_loader_name);

    gdt_init();
    print_debug(LT_OK, "GDT initialised\n");
    uint32_t esp = 0;
    asm volatile("mov %%esp, %%eax" : "=a" (esp));
    tss_install(0x10, esp);
    print_debug(LT_OK, "TSS installed\n");
    idt_init();
    print_debug(LT_OK, "IDT initialised\n");
    isr_init();
    print_debug(LT_OK, "ISR initialised\n");

    disk_init();

    if(!(mbd->flags & (1 << 6))) {
        print_debug(LT_CR, "no memory map given by bootloader. system halted\n");
        kpanic();
    }
    mem_init(mbd->mmap_addr, mbd->mmap_length);

    syscall_init();
    print_debug(LT_OK, "syscall initialised\n");

    kbd_init();

    timer_init();

    // start interrupts again after setting up everything
    asm volatile("sti");

    // make cursor slimmer
    tty_enable_cursor(13, 14);

    print_debug(LT_IF, "done initialising\n");

    shell_set_root_node(fs->root_node);
    shell_start();

    // // i cannot get this to work :(
    // // enter_usermode();

    // // test "syscall"
    // // the privilege is still ring 0, though
    // asm volatile("xor %eax, %eax; int $0x80"); // SYS_SYSCALL_TEST
    // asm volatile("xor %eax, %eax; inc %eax; int $0x80"); // SYS_PUTCHAR

    while(true);
}
