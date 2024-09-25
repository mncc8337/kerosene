#include "system.h"
#include "mem.h"
#include "misc/elf.h"
#include "stdio.h"
#include "video.h"

#define MAX_FRAMES 30

static char* string_table = 0;
static elf_section_header_t* symtab_sh = 0;
static elf_section_header_t* debug_line_sh = 0;

void stack_trace(stackframe_t* stk) {
    if(stk == NULL)
        asm("movl %%ebp, %0" : "=r"(stk));

    for(unsigned frame = 0; stk && frame < MAX_FRAMES; frame++) {
        if(!vmmngr_to_physical_addr(NULL, (virtual_addr_t)stk)) {
            printf("unmapped EBP address: 0x%x\n", stk);
            return;
        }

        uint32_t addr = stk->eip;
        printf("[0x%x] ", addr);

        unsigned function_addr;
        char* function_name = kernel_find_symbol(addr, ELF_STT_FUNC, &function_addr);

        unsigned object_addr;
        char* object_name = kernel_find_symbol(addr, ELF_STT_OBJECT, &object_addr);

        char* name;
        if(function_addr > object_addr) name = function_name;
        else name = object_name;

        printf("%s\n", name ? name : "unknown function");

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
char* kernel_find_symbol(unsigned addr, int type, unsigned* ret_addr) {
    if(!symtab_sh || !string_table) return NULL;

    char* function_name = 0;
    *ret_addr = 0;

    for(unsigned i = 0; i < symtab_sh->size; i += symtab_sh->entry_size) {
        elf_symbol_table_t* s = (elf_symbol_table_t*)(symtab_sh->addr + KERNEL_START + i);
        if(s->value > addr || s->value < (*ret_addr)) continue;
        if(ELF_ST_TYPE(s->info) != type) continue;

        function_name = string_table + s->name;
        *ret_addr = s->value;
    }

    return function_name;
}

void kernel_panic(stackframe_t* stk) {
    asm("cli");
    video_set_attr(video_rgb(VIDEO_LIGHT_RED), video_rgb(VIDEO_BLACK));
    puts("kernel panicked!");
    video_set_attr(video_rgb(VIDEO_WHITE), video_rgb(VIDEO_BLACK));
    puts("stack trace:");
    stack_trace(stk);
    puts("system halted!");
    while(1) asm("hlt");
}
