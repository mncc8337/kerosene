#pragma once

#include "stdint.h"

 // 0x7f ELF
#define ELF_MAGIC 0x7f454c46

// INSSET = instruction set
#define ELF_INSSET_UNKNOWN 0x00
#define ELF_INSSET_SPARC   0x02
#define ELF_INSSET_X86     0x03
#define ELF_INSSET_MIPS    0x08
#define ELF_INSSET_POWERPC 0x14
#define ELF_INSSET_ARM     0x28
#define ELF_INSSET_SUPERH  0x2a
#define ELF_INSSET_IA64    0x32
#define ELF_INSSET_X86_64  0x3e
#define ELF_INSSET_AARCH64 0xb7
#define ELF_INSSET_RISC_V  0xf3

// SHT = section header type
#define ELF_SHT_NULL         0
#define ELF_SHT_PROGBITS     1
#define ELF_SHT_SYMTAB       2
#define ELF_SHT_STRTAB       3
#define ELF_SHT_RELOC_ADDEND 4
#define ELF_SHT_HASH         5
#define ELF_SHT_DYNAMIC      6
#define ELF_SHT_NOTE         7
#define ELF_SHT_NOBITS       8
#define ELF_SHT_RELOC        9
#define ELF_SHT_SHLIB        10
#define ELF_SHT_DYNSYM       11
#define ELF_SHT_LOPROC       0x70000000
#define ELF_SHT_HIPROC       0x7fffffff
#define ELF_SHT_LOUSER       0x80000000
#define ELF_SHT_HIUSER       0xffffffff

#define ELF_ST_BIND(i) ((i) >> 4)
#define ELF_ST_TYPE(i) ((i) & 0xf)

// STB = symbol table binding
#define ELF_STB_LOCAL  0
#define ELF_STB_GLOBAL 1
#define ELF_STB_WEAK   2
#define ELF_STB_LOPROC 13
#define ELF_STB_HIPROC 15

// STT = symbol table type
#define ELF_STT_NOTYPE  0
#define ELF_STT_OBJECT  1
#define ELF_STT_FUNC    2
#define ELF_STT_SECTION 3
#define ELF_STT_FILE    4
#define ELF_STT_LOPROC  13
#define ELF_STT_HIPROC  15

typedef struct {
    uint32_t magic;
    uint8_t bits;
    uint8_t endianess;
    uint8_t header_version;
    uint8_t os_abi;
    uint8_t padding[8];
    uint16_t type;
    uint16_t instruction_set;
    uint32_t elf_version;
    uint32_t program_entry_offset;
    uint32_t pht_offset;
    uint32_t sht_offset;
    uint32_t flags;
    uint16_t elf_header_size;
    uint16_t pht_entry_size;
    uint16_t pht_entry_count;
    uint16_t sht_entry_size;
    uint16_t sht_entry_count;
    uint16_t shstrndx; // Section Header STRing iNDeX (the index of the section name string table)
} elf_header_t;

// addr: uint32
// half uint16
// off uint32
// sword signed int32
// word uint32

typedef struct {
    uint32_t name;
    uint32_t type;
    uint32_t flags;
    uint32_t addr;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t addr_align;
    uint32_t entry_size;
} elf_section_header_t;

typedef struct {
    uint32_t name;
    uint32_t value;
    uint32_t size;
    uint8_t info;
    uint8_t other;
    uint16_t shndx; // Section Header iNDeX (the index in the string table)
} elf_symbol_table_t;

typedef struct {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t file_segment_size;
    uint32_t mem_segment_size;
    uint32_t flags;
    uint32_t align;
} elf_program_header_t;

// TODO: fully implement ELF32
