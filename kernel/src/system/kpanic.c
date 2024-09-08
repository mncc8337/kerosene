#include "system.h"
#include "misc/elf.h"
#include "stdio.h"
#include "video.h"

#define MAX_FRAMES 30
#define NULL ((void *)0)

static char* string_table = 0;
static elf_section_header_t* symtab_sh = 0;
static elf_section_header_t* debug_line_sh = 0;

static void stack_trace(stackframe_t* stk) {
    if(stk == NULL)
        asm ("movl %%ebp, %0" : "=r"(stk));

    for(unsigned frame = 0; stk && frame < MAX_FRAMES; frame++) {
        uint32_t addr = stk->eip;
        printf("[0x%x] ", addr);

        char* function_name = kernel_find_symbol(addr, ELF_STT_FUNC);
        printf("%s\n", function_name ? function_name : "unknown function");

        stk = stk->ebp;
    }
    if(stk != 0) puts("..."); // reached max frames limit
}

void kernel_set_strtab_ptr(uint32_t ptr) {
    string_table = (char*)ptr;
}

void kernel_set_symtab_sh_ptr(uint32_t ptr) {
    symtab_sh = (elf_section_header_t*)ptr;
}

void kernel_set_debug_line_sh_ptr(uint32_t ptr) {
    debug_line_sh = (elf_section_header_t*)ptr;
}

// find a symbol in the symbol table, given the address
// it will return the lower bound of the address
// set type to -1 to find for all symbol type
char* kernel_find_symbol(unsigned addr, int type) {
    if(!symtab_sh || !string_table) return NULL;

    char* function_name = 0;
    uint32_t closest_addr = 0;

    for(unsigned i = 0; i < symtab_sh->size; i += symtab_sh->entry_size) {
        elf_symbol_table_t* s = (elf_symbol_table_t*)(symtab_sh->addr + KERNEL_START + i);
        if(s->value > addr || s->value < closest_addr) continue;
        if(type > 0 && ELF_ST_TYPE(s->info) != type) continue;

        function_name = string_table + s->name;
        closest_addr = s->value;
    }

    return function_name;
}

void kernel_panic(stackframe_t* stk) {
    video_set_attr(video_rgb(VIDEO_LIGHT_RED), video_rgb(VIDEO_BLACK));
    puts("kernel panicked!");
    video_set_attr(video_rgb(VIDEO_WHITE), video_rgb(VIDEO_BLACK));
    puts("stack trace:");
    stack_trace(stk);
    puts("system halted!");
    asm volatile("cli");
    while(1) asm volatile("hlt");
}
