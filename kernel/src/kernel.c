#include "system.h"
#include "mem.h"
#include "tty.h"
#include "kbd.h"

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

    pmmngr_update_usage();
    printf("initialized ");
    printf(itoa(pmmngr_get_free_size()/1024/1024, freebuff, 10));
    puts(" MiB");

    // vmmngr init
    ///////////////////////////////////////////////////////////
    if(vmmngr_init() == ERR_OUT_OF_MEM) {
        puts("cannot enable paging because there is not enough memory, system halted");
        SYS_HALT();
    }
    puts("paging enabled");
    ///////////////////////////////////////////////////////////
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

    kbd_init();

    // make cursor slimmer
    tty_enable_cursor(13, 14);

    set_key_listener(print_typed_char);

    puts("done initializing");

    char printing_txt[] = "printing";
    printf("im %s formated stri%c%c\n", printing_txt, 'n', 'g');
    printf("this is some numbers: %d %d %d %d\n", 15, 0, 12, -8);
    printf("%% symbol\n");
    printf("some hex num: %x %x %x %x\n", 0x13, 0xbeef, 0xdead, 0xbee);
    printf("some oct num: %o %o %o %o\n", 07, 0100, 014, 8);

    while(true);
}
