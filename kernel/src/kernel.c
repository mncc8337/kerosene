#include "system.h"
#include "mem.h"
#include "tty.h"
#include "kbd.h"

#include "errorcode.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

typedef struct {
    uint32_t memmap_ptr;
    size_t memmap_entry_count;
} __attribute__((packed)) bootinfo_t;

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

void mem_init(bootinfo_t bootinfo) {
    size_t entry_cnt = bootinfo.memmap_entry_count;
    memmap_entry_t* mmptr = (memmap_entry_t*)bootinfo.memmap_ptr;

    // sort the map
    ///////////////////////////////////////////////////////////
    bool sorted = false;
    memmap_entry_t temp;
    while(!sorted) {
        sorted = true;
        for(unsigned int i = 1; i < entry_cnt; i++) {
            uint64_t prev_base = ((uint64_t)mmptr[i-1].base_high << 32) | mmptr[i-1].base_low;
            uint64_t curr_base = ((uint64_t)mmptr[i].base_high << 32) | mmptr[i].base_low;

            if(prev_base > curr_base) {
                sorted = false;
                temp = mmptr[i];
                mmptr[i] = mmptr[i-1];
                mmptr[i-1] = temp;
            }
        }
    }
    ///////////////////////////////////////////////////////////

    // print mem info
    ///////////////////////////////////////////////////////////
    // spent way too much time on this table
    printf("--[MEMORY]-+--------------------+-------------------\n");
    printf("base addr  | length             | type\n");
    printf("-----------+--------------------+-------------------\n");

    uint64_t base = 0;
    uint64_t length = 0;
    for(uint32_t i = 0; i < entry_cnt; i++) {
        length = ((uint64_t)mmptr[i].length_high << 32) | mmptr[i].length_low;
        base = ((uint64_t)mmptr[i].base_high << 32) | mmptr[i].base_low;

        printf("0x%s", itoa(base, freebuff, 16));
        for(int i = 0; i < 8 - (int)strlen(freebuff); i++) putchar(' ');
        printf(" | ");

        printf("%s KiB", itoa(length/1024, freebuff, 10));
        for(int i = 0; i < 14 - (int)strlen(freebuff); i++) putchar(' ');
        printf(" | ");

        switch(mmptr[i].type) {
            case MMER_TYPE_USABLE:
                printf("usable");
                break;
            case MMER_TYPE_RESERVED:
                printf("reserved");
                break;
            case MMER_TYPE_ACPI:
                printf("ACPI reclamable");
                break;
            case MMER_TYPE_ACPI_NVS:
                printf("ACPI non-volatile");
                break;
            case MMER_TYPE_BADMEM:
                printf("bad mem");
                break;
        }

        putchar('\n');
    }
    printf("-----------+--------------------+-------------------\n");
    ///////////////////////////////////////////////////////////

    // get memsize
    ///////////////////////////////////////////////////////////
    size_t memsize = 0;
    for(unsigned int i = 0; i < entry_cnt; i++) {
        // ignore memory higher than 4GiB
        // the map is sorted so we can break to ignore all data next to it
        if(mmptr[i].base_high > 0 || mmptr[i].length_high > 0) break;
        // probaly bugs
        if(i > 0 && mmptr[i].base_low == 0) continue;

        memsize += mmptr[i].length_low;
    }
    pmmngr_init(AFTER_KERNEL_ADDR + 1, memsize);
    ///////////////////////////////////////////////////////////

    // init regions
    ///////////////////////////////////////////////////////////
    for(unsigned int i = 0; i < entry_cnt; i++) {
        if(mmptr[i].base_high > 0 || mmptr[i].length_high > 0) break;
        if(i > 0 && mmptr[i].base_low == 0) continue;
        // if not usable or ACPI reclaimable then skip
        if(mmptr[i].type != MMER_TYPE_USABLE && mmptr[i].type != MMER_TYPE_ACPI)
            continue;

        pmmngr_init_region(mmptr[i].base_low, mmptr[i].length_low);
    }
    ///////////////////////////////////////////////////////////

    // deinit regions
    ///////////////////////////////////////////////////////////
    // kernel
    pmmngr_deinit_region(KERNEL_ADDR, KERNEL_SECTOR_COUNT*512);
    // physical mem manager
    pmmngr_deinit_region(AFTER_KERNEL_ADDR, pmmngr_get_size()/MMNGR_BLOCK_SIZE);
    ///////////////////////////////////////////////////////////

    pmmngr_update_usage();
    printf("initialized ");
    printf(itoa(pmmngr_get_free_size()/1024/1024, freebuff, 10));
    printf(" MiB\n");

    // vmmngr init
    ///////////////////////////////////////////////////////////
    if(vmmngr_init() == ERR_OUT_OF_MEM) {
        printf("cannot enable paging because there is not enough memory, system halted\n");
        SYS_HALT();
    }
    printf("paging enabled\n");
    ///////////////////////////////////////////////////////////
}

void kmain(bootinfo_t bootinfo) {
    // greeting msg to let us know we are in the kernel
    tty_set_attr(LIGHT_CYAN);  printf("hello\n");
    tty_set_attr(LIGHT_GREEN); printf("this is ");
    tty_set_attr(LIGHT_RED);   printf("the kernel\n");
    tty_set_attr(LIGHT_GREY);

    mem_init(bootinfo);

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

    printf("done initializing\n");

    while(true);
}
