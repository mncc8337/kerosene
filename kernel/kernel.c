#include "system.h"
#include "mem.h"

#include "driver/video.h"
#include "driver/kbd.h"

#include "utils.h"

typedef struct {
    uint32_t memmap_ptr;
    size_t memmap_entry_count;
} __attribute__((packed)) bootinfo_t;

const unsigned int TIMER_PHASE = 100;

char freebuff[512];

void print_typed_char(key_t k) {
    if(k.released) return;

    if(k.mapped == '\b') {
        set_cursor(get_cursor() - 1); // move back
        print_char(' ', -1, 0, false); // delete printed char
    }
    else {
        print_char(k.mapped, -1, 0, true);
    }
}

void mem_init(bootinfo_t bootinfo) {
    size_t entry_cnt = bootinfo.memmap_entry_count;
    memmap_entry_t* mmptr = (memmap_entry_t*)bootinfo.memmap_ptr;

    // sort the map
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

    // spent way too much time on this table
    print_string("--[MEMORY]-+--------------------+-------------------\n", -1, 0, true);
    print_string("base addr  | length             | type\n", -1, 0, true);
    print_string("-----------+--------------------+-------------------\n", -1, 0, true);

    uint64_t base = 0;
    uint64_t length = 0;
    for(uint32_t i = 0; i < entry_cnt; i++) {
        length = ((uint64_t)mmptr[i].length_high << 32) | mmptr[i].length_low;
        base = ((uint64_t)mmptr[i].base_high << 32) | mmptr[i].base_low;

        print_string("0x", -1, 0, true);
        itoa(base, freebuff, 16);
        print_string(freebuff, -1, 0, true);
        for(int i = 0; i < 8 - (int)strlen(freebuff); i++) print_char(' ', -1, 0, true);
        print_string(" | ", -1, 0, true);

        itoa(length/1024, freebuff, 10);
        print_string(freebuff, -1, 0, true);
        print_string(" KiB", -1, 0, true);
        for(int i = 0; i < 14 - (int)strlen(freebuff); i++) print_char(' ', -1, 0, true);
        print_string(" | ", -1, 0, true);

        switch(mmptr[i].type) {
            case MMER_TYPE_USABLE:
                print_string("usable", -1, 0, true);
                break;
            case MMER_TYPE_RESERVED:
                print_string("reserved", -1, 0, true);
                break;
            case MMER_TYPE_ACPI:
                print_string("ACPI reclamable", -1, 0, true);
                break;
            case MMER_TYPE_ACPI_NVS:
                print_string("ACPI non-volatile", -1, 0, true);
                break;
            case MMER_TYPE_BADMEM:
                print_string("bad mem", -1, 0, true);
                break;
        }

        print_char('\n', -1, 0, true);
    }
    print_string("-----------+--------------------+-------------------\n", -1, 0, true);

    // get memsize
    size_t memsize = 0;
    for(unsigned int i = 0; i < entry_cnt; i++) {
        // ignore memory higher than 4GiB
        // the map is sorted so we can break to ignore all data next to it
        if(mmptr[i].base_high > 0 || mmptr[i].length_high > 0) break;
        // probaly bugs
        if(i > 0 && mmptr[i].base_low == 0) continue;

        // if not usable or not apci reclaimable then skip
        if(mmptr[i].type != MMER_TYPE_USABLE && mmptr[i].type != MMER_TYPE_ACPI)
            continue;

        memsize += mmptr[i].length_low;
    }
    pmmngr_init(0xc0000000 + KERNEL_SECTOR_COUNT*512, memsize);

    // init regions
    for(unsigned int i = 0; i < entry_cnt; i++) {
        if(mmptr[i].base_high > 0 || mmptr[i].length_high > 0) break;
        if(i > 0 && mmptr[i].base_low == 0) continue;
        if(mmptr[i].type != MMER_TYPE_USABLE && mmptr[i].type != MMER_TYPE_ACPI)
            continue;

        pmmngr_init_region(mmptr[i].base_low, mmptr[i].length_low);
    }
    // deinit regions
    pmmngr_deinit_region(KERNEL_ADDR, KERNEL_SECTOR_COUNT*512);

    print_string("initialized ", -1, 0, true);
    print_string(itoa(memsize/1024/1024, freebuff, 10), -1, 0, true);
    print_string(" MiB memory\n", -1, 0, true);

    vmmngr_init();
    print_string("paging enabled\n", -1, 0, true);
}

void kmain(bootinfo_t bootinfo) {
    // greeting msg to let us know we are in the kernel
    print_string("hello\n", -1, LIGHT_CYAN, true);
    print_string("this is ", -1, LIGHT_GREEN, true);
    print_string("the kernel\n", -1, LIGHT_RED, true);

    mem_init(bootinfo);

    gdt_init();
    idt_init();
    irq_init();

    // start receiving interrupts
    // after initialize all interrupt handlers
    asm volatile("sti");

    kbd_init();

    // make cursor slimmer
    enable_cursor(13, 14);

    set_key_listener(print_typed_char);

    while(true);
}
