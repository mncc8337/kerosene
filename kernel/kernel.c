#include "system.h"
#include "mem.h"

#include "driver/video.h"
#include "driver/kbd.h"
#include "driver/pit.h"

#include "utils.h"

typedef struct {
    uint32_t memmap_ptr;
    size_t memmap_entry_count;
} __attribute__((packed)) bootinfo_t;

const unsigned int TIMER_PHASE = 100;

// free mem everyone
unsigned char freebuff[512];

volatile unsigned int timer_ticks = 0;
char* sec_msg = "seconds elapsed";
void timer_handler() {
    timer_ticks++;

    itoa(timer_ticks/TIMER_PHASE, freebuff, 10);

    print_string(freebuff, MAX_COLS - strlen(freebuff) - 17, 0x04, false);
    print_string(sec_msg, MAX_COLS - 15, 0x04, false);
}
void timer_wait(unsigned int ticks) {
    unsigned long eticks;

    eticks = timer_ticks + ticks;
    while(timer_ticks < eticks) SYS_SLEEP;
}

char* row_str;
char* col_str;
void print_typed_char(key k) {
    if(k.released) return;

    int _row = k.keycode >> 4;
    int _col = k.keycode & 0xf;

    print_string("     ", MAX_COLS * 2 - 5, 0, false);
    print_string(itoa(_row, freebuff, 10), MAX_COLS * 2 - 5, 0, false);
    print_string(itoa(_col, freebuff, 10), MAX_COLS * 2 - 2, 0, false);

    if(k.mapped == '\b') {
        set_cursor(get_cursor() - 1); // move back
        print_char(' ', -1, 0, false); // delete printed char
    }
    else {
        print_char(k.mapped, -1, 0, true);
    }
}

void mem_init(bootinfo_t bootinfo) {
    size_t memmap_cnt = bootinfo.memmap_entry_count;
    memmap_entry_t* entry = (memmap_entry_t*)bootinfo.memmap_ptr;

    // spent way too much time on this table
    print_string("--[MEMORY]-+------------+-------------------\n", -1, 0, true);
    print_string("base addr  | length     | type\n", -1, 0, true);
    print_string("-----------+------------+-------------------\n", -1, 0, true);

    uint64_t base = 0;
    uint64_t length = 0;
    for(uint32_t i = 0; i < memmap_cnt; i++) {
        length = (entry[i].length_high << 31) + entry[i].length_low;
        base = (entry[i].base_high << 31) + entry[i].base_low;

        print_string("0x", -1, 0, true);
        itoa(base, freebuff, 16);
        print_string(freebuff, -1, 0, true);
        for(int i = 0; i < 8 - strlen(freebuff); i++) print_char(' ', -1, 0, true);
        print_string(" | ", -1, 0, true);

        print_string("0x", -1, 0, true);
        itoa(length, freebuff, 16);
        print_string(freebuff, -1, 0, true);
        for(int i = 0; i < 8 - strlen(freebuff); i++) print_char(' ', -1, 0, true);
        print_string(" | ", -1, 0, true);

        switch(entry[i].type) {
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
    print_string("-----------+------------+-------------------\n", -1, 0, true);

    size_t memsize = 0;
    for(uint32_t i = 0; i < memmap_cnt; i++) {
        // reuse variable above
        length = entry[i].length_low + (entry[i].length_high << 31);
        // ignore empty entry
        if(length == 0) continue;

        // any mem that is above 4GiB is ignored
        if(entry[i].base_high == 0)
            memsize += entry[i].length_low;
    }
    print_string("detected ", -1, 0, true);
    print_string(itoa(memsize/1024/1024, freebuff, 10), -1, 0, true);
    print_string(" MiB of mem\n", -1, 0, true);

    // place pmmngr after kernel
    uint32_t* bitmap = (uint32_t*)(KERNEL_ADDR + KERNEL_SECTOR_COUNT * 512);
    pmmngr_init(memsize, bitmap);
    print_string("physical memory manager initialized\n", -1, 0, true);

    for(uint32_t i = 0; i < memmap_cnt; i++) {        
        // init free for use or reclaimable ACPI mem
        if(entry[i].type == MMER_TYPE_USABLE || entry[i].type == MMER_TYPE_ACPI)
            pmmngr_init_region(entry[i].base_low, entry[i].length_low);
    }

    size_t usable_mem = pmmngr_get_free_block_count() * PMMNGR_BLOCK_SIZE /1024/1024;
    print_string("initialized ", -1, 0, true);
    print_string(itoa(usable_mem, freebuff, 10), -1, 0, true);
    print_string(" MiB of free mem\n", -1, 0, true);

    // disable all mem below pmmngr and it self
    pmmngr_deinit_region(0x0, KERNEL_ADDR + KERNEL_SECTOR_COUNT * 512);
    pmmngr_deinit_region(KERNEL_ADDR + KERNEL_SECTOR_COUNT * 512, 0x40000);
}

void main(bootinfo_t bootinfo) {
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

    pit_timer_phase(TIMER_PHASE);
    irq_install_handler(0, timer_handler);

    kbd_init();

    // some io stuff to demo

    // make cursor slimmer
    enable_cursor(13, 14);

    char inp[512];

    while(true) {
        get_string(inp, '\n', print_typed_char);
        print_char('\n', -1, 0, true);
    }
}
