#include "system.h"
#include "mem.h"
#include "tty.h"
#include "kbd.h"
#include "ata.h"

#include "multiboot.h"
#include "errorcode.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

const unsigned int TIMER_PHASE = 100;

char freebuff[512];

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
    ///////////////////////////////////////////////////////////
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
    ///////////////////////////////////////////////////////////

    // get memsize
    ///////////////////////////////////////////////////////////
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
    ///////////////////////////////////////////////////////////

    // init regions
    ///////////////////////////////////////////////////////////
    for(unsigned int i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(mbd->mmap_addr + i);

        if((mmmt->addr >> 32) > 0 || (mmmt->len >> 32) > 0) break;
        if(i > 0 && mmmt->addr == 0) continue;

        // if not usable or ACPI reclaimable then skip
        if(mmmt->type != MULTIBOOT_MEMORY_AVAILABLE && mmmt->type != MULTIBOOT_MEMORY_ACPI_RECLAIMABLE)
            continue;

        pmmngr_init_region(mmmt->addr, mmmt->len);
    }
    ///////////////////////////////////////////////////////////

    // deinit regions
    ///////////////////////////////////////////////////////////
    // kernel
    pmmngr_deinit_region(startkernel, endkernel - startkernel + 1);
    // physical mem manager
    pmmngr_deinit_region(endkernel, pmmngr_get_size()/MMNGR_BLOCK_SIZE);
    ///////////////////////////////////////////////////////////

    pmmngr_update_usage(); // always run this after init and deinit regions
    printf("initialized %d MiB\n", pmmngr_get_free_size()/1024/1024);

    // vmmngr init
    ///////////////////////////////////////////////////////////
    if(vmmngr_init() == ERR_OUT_OF_MEM) {
        puts("cannot enable paging because there is not enough memory, system halted");
        SYS_HALT();
    }
    puts("paging enabled");
    ///////////////////////////////////////////////////////////
}
void disk_init() {
    uint16_t IDENTIFY_returned[256];
    if(ata_pio_init(IDENTIFY_returned)) {
        puts("ATA PIO initialized");

        printf(
            "    LBA48 mode: %s\n",
            (IDENTIFY_returned[83] & 0x400) ? "yes" : "no"
        );

        if(IDENTIFY_returned[88]) {
            uint8_t supported_UDMA = IDENTIFY_returned[88] & 0xff;
            uint8_t active_UDMA = IDENTIFY_returned[88] >> 8;
            printf("    supported UDMA mode:");
            for(int i = 0; i < 8; i++)
                if(supported_UDMA & (1 << i)) printf(" %d", i+1);
            putchar('\n');
            printf("    active UDMA mode:");
            for(int i = 0; i < 8; i++)
                if(active_UDMA & (1 << i)) printf(" %d", i+1);
            putchar('\n');
        }
    }
    else {
        puts("failed to initialize ATA PIO");
    }
}

void kmain(multiboot_info_t* mbd, unsigned int magic) {
    // greeting msg to let us know we are in the kernel
    tty_set_attr(LIGHT_CYAN);  puts("hello");
    tty_set_attr(LIGHT_GREEN); printf("this is ");
    tty_set_attr(LIGHT_RED);   puts("the kernel");
    tty_set_attr(LIGHT_GREY);

    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        puts("invalid magic number. system halted");
        SYS_HALT();
    }
    if(!(mbd->flags >> 6 & 0x1)) {
        puts("invalid memory map given by GRUB. system halted");
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

    puts("done initializing");

    while(true);
}
