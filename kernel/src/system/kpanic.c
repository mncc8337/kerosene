#include "system.h"
#include "misc/elf.h"
#include "stdio.h"
#include "video.h"

#define MAX_FRAMES 30
#define NULL 0

static char* string_table;
static elf_section_header_t* symbol_table_sh;

static void stack_trace(stackframe_t* stk) {
    if(stk == NULL)
        asm ("movl %%ebp, %0" : "=r"(stk) ::);

    puts("stack trace:");
    for(unsigned frame = 0; stk && frame < MAX_FRAMES; frame++) {
        uint32_t addr = stk->eip;
        printf("0x%x: ", addr);
        char* function_name = kernel_find_symbol(addr, ELF_STT_FUNC);
        printf("%s\n", function_name ? function_name : "unknown!");
        stk = stk->ebp;
    }
    if(stk != 0) puts("..."); // reached max frames limit
}

void kernel_set_strtab_ptr(uint32_t ptr) {
    string_table = (char*)ptr;
}

void kernel_set_symtabsh_ptr(uint32_t ptr) {
    symbol_table_sh = (elf_section_header_t*)ptr;
}

// find a symbol in the symbol table, given the address
// it will return the lower bound of the address
// set type to -1 to find for all symbol type
char* kernel_find_symbol(unsigned addr, int type) {
    char* function_name = 0;
    uint32_t closest_addr = 0;

    for(unsigned i = 0; i < symbol_table_sh->size; i += symbol_table_sh->entry_size) {
        elf_symbol_table_t* s = (elf_symbol_table_t*)(symbol_table_sh->addr + VMBASE_KERNEL + i);
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
    stack_trace(stk);
    puts("system halted!");
    asm volatile("cli");
    while(1) asm volatile("hlt");
}
