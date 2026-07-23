#include <multiboot.h>

#include <video.h>
#include <ata.h>
#include <kbd.h>
#include <timer.h>
#include <filesystem.h>
#include <locale.h>

#include <system.h>
#include <process.h>
#include <kproc.h>
#include <mem.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>

#include <misc/elf.h>

uint32_t kernel_size;

void kmain();
process_t* kernel_process = 0;

void mem_init(void* mmap_addr, uint32_t mmap_length) {
    // get memsize
    size_t memsize = 0;
    for(unsigned int i = 0; i < mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(mmap_addr + i);

        // ignore memory higher than 4GiB
        if((mmmt->addr >> 32) > 0 || (mmmt->len  >> 32) > 0) continue;
        // probably bugs
        if(i > 0 && mmmt->addr == 0) continue;

        memsize += mmmt->len;
    }
    pmmngr_init(memsize);
    kprint_debug(LT_OK, "pmmngr initialised, detected %d MiB of memory\n", memsize/1024/1024);

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
    pmmngr_deinit_region(0, 4*1024*1024); // first 4 MiB is preserved for kernel

    pmmngr_update_usage(); // always run this after init and deinit regions

    vmmngr_init();
}

void video_init(multiboot_info_t* mbd) {
    virtual_addr_t video_addr = 0;
    unsigned video_width = 0;
    unsigned video_height = 0;
    unsigned video_pitch = 0;
    unsigned video_bpp = 0;

    bool using_framebuffer = false;
    if(mbd->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) {
        video_addr = mbd->framebuffer_addr;
        video_width = mbd->framebuffer_width;
        video_height = mbd->framebuffer_height;
        video_pitch = mbd->framebuffer_pitch;
        video_bpp = mbd->framebuffer_bpp;
       
       switch(mbd->framebuffer_type) {
            case MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED:
                using_framebuffer = false;
                // very rare, not likely to happend
                // so let's just not support it hehe
                break;
            case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
                using_framebuffer = true;
                // FIXME: do sth with framebuffer color_info
                break;
            case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
                using_framebuffer = false;
                break;
       }
    } else {
        using_framebuffer = false;
        video_addr = VIDEO_TEXTMODE_ADDRESS;
        video_width = 80;
        video_height = 25;
        video_pitch = 160;
        video_bpp = 16;
    }

    // map video ptr
    uint32_t total_size = video_height * video_pitch;
    uint32_t total_pages = (total_size + MMNGR_PAGE_SIZE - 1) / MMNGR_PAGE_SIZE;
    for(unsigned p = 0; p < total_pages; p++) {
        unsigned offset = p * MMNGR_PAGE_SIZE;
        vmmngr_map(
            NULL,
            video_addr + offset,
            VIDEO_START + offset,
            PTE_WRITABLE | PTE_WRITETHROUGH
        );
    }
    if(using_framebuffer) {
        video_framebuffer_init(
            video_width, video_height,
            video_pitch, video_bpp
        );
        kprint_debug(
            LT_OK,
            "VESA video initialised with resolution %dx%d, %d bpp\n",
            video_width,
            video_height,
            video_bpp
        );
    } else {
        video_vga_init(video_width, video_height);
        kprint_debug(
            LT_OK,
            "VGA video initialised with resolution %dx%d, %d bpp\n",
            video_width,
            video_height,
            video_bpp
        );
    }
}

void disk_init() {
    uint16_t* dump = kmalloc(256 * sizeof(uint16_t));
    if(!dump) {
        kprint_debug(LT_ER, "not enough memory to initialise disk\n");
        return;
    }

    ATA_PIO_ERR ata_err = ata_pio_init(dump);
    kfree(dump);
    if(ata_err) {
        kprint_debug(LT_WN, "failed to initialise ATA PIO mode. error code %d\n", ata_err);
        return;
    }

    kprint_debug(LT_OK, "ATA PIO mode initialised\n");

    bool mbr_err = mbr_load();
    if(mbr_err) {
        kprint_debug(LT_ER, "cannot load MBR\n");
        return;
    }
    kprint_debug(LT_OK, "MBR loaded\n");

    for(int i = 0; i < 4; i++) {
        partition_entry_t part = mbr_get_partition_entry(i);
        if(part.sector_count == 0) continue;

        unsigned fs_count = 0;

        FS_ERR err;
        switch(vfs_detectfs(&part)) {
            case FS_EMPTY:
            case FS_RAMFS:
                break;

            case FS_FAT32:
                err = fat32_init(vfs_getfs(fs_count), part);
                if(err)
                    kprint_debug(LT_ER, "failed to initialise FAT32 filesystem on partition %d. error code %d\n", i+1, err);
                else {
                    kprint_debug(LT_OK, "initialised FAT32 filesystem on partition %d", i+1);
                    kprintf(" on fs (%d)\n", fs_count);
                    fs_count++;
                }
                break;
            case FS_EXT2:
                kprint_debug(LT_WN, "EXT2 filesystem in partition %d is not implemented, the partition will be ignored\n", i+1);
                break;
        }
    }
}

void kinit(multiboot_info_t* mbd) {
    // disable interrupts to set up things
    asm volatile("cli");

    // from kernel_entry.asm
    extern uint32_t kernel_start;
    extern uint32_t kernel_end;
    kernel_size = &kernel_end - &kernel_start;

    // greeting msg to let us know we are in the kernel
    // note that this will print into preinit video buffer
    // and will not drawn to screen until video is initialised
    kputs("hello");
    kprintf("this is kerosense!\n");
    kprintf("build datetime: %s, %s\n", __TIME__, __DATE__);
    kprintf("kernel size: %d bytes\n", kernel_size);

    // since we have mapped 4MiB from 0x0 to 0xc0000000
    // any physical address under 4MiB can be converted to virtual address
    // by adding 0xc0000000 to it
    // i think that GRUB will not give any address that are larger than 4 MiB
    // except the framebuffer
    // note that the ELF section also included in the kernel (which lies in the first 4MiB)
    // so those physical address of ELF section headers need to offset to KERNEL_START too
    mbd = (void*)mbd + KERNEL_START;

    if(mbd->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME)
        kprint_debug(LT_IF, "using %s bootloader\n", mbd->boot_loader_name + KERNEL_START);

    if(!(mbd->flags & MULTIBOOT_INFO_MEM_MAP)) kernel_panic(NULL);
    mem_init((void*)mbd->mmap_addr + KERNEL_START, mbd->mmap_length);

    // correctly set kernel data region permissions
    extern uint32_t kernel_text_start;
    extern uint32_t kernel_text_end;
    extern uint32_t kernel_rodata_start;
    extern uint32_t kernel_rodata_end;
    uint32_t kernel_text_start_addr = (uint32_t)&kernel_text_start;
    uint32_t kernel_text_end_addr = (uint32_t)&kernel_text_end;
    uint32_t kernel_rodata_start_addr = (uint32_t)&kernel_rodata_start;
    uint32_t kernel_rodata_end_addr = (uint32_t)&kernel_rodata_end;
    for(uint32_t addr = kernel_text_start_addr; addr < kernel_text_end_addr; addr += MMNGR_PAGE_SIZE) {
        pte_t* pte = (pte_t*)(0xffc00000 + ((addr >> 12) * 4));
        *pte &= ~PTE_WRITABLE; // unset writable
    }
    for(uint32_t addr = kernel_rodata_start_addr; addr < kernel_rodata_end_addr; addr += MMNGR_PAGE_SIZE) {
        pte_t* pte = (pte_t*)(0xffc00000 + ((addr >> 12) * 4));
        *pte &= ~PTE_WRITABLE; // unset writable
    }
    vmmngr_flush_tlb();

    video_init(mbd);

    if(kheap_init()) {
        kprint_debug(LT_CR, "failed to initialise kernel heap. not enough memory\n");
        kernel_panic(NULL);
    }
    kprint_debug(LT_OK, "kernel heap initialised\n");

    // if(mbd->flags & MULTIBOOT_INFO_AOUT_SYMS) {
    //     aout_sym = mbd->u.aout_sym;
    // }

    // we gave GRUB an ELF binary so GRUB will not give us the a.out symbol table option (commented above)
    // also only one of the two (a.out option or ELF option) must be existed
    // so we dont need to check the flag
    multiboot_elf_section_header_table_t* elf_sec = &(mbd->u.elf_sec);
    elf_section_header_t* shstrtab_sh = (elf_section_header_t*)(elf_sec->addr + KERNEL_START) + elf_sec->shndx;
    char* shstrtab = (char*)(shstrtab_sh->addr + KERNEL_START);

    // find symtab and strtab
    for(unsigned i = 0; i < elf_sec->num; i++) {
        elf_section_header_t* sh = (elf_section_header_t*)(elf_sec->addr + KERNEL_START) + i;
        char* sec_name = shstrtab + sh->name;

        if(!strcmp(sec_name, ".symtab")) {
            kprint_debug(LT_IF, "found .symtab section\n");
            kernel_set_symtab_sh_ptr((uint32_t)sh);
        } else if(!strcmp(sec_name, ".strtab")) {
            kprint_debug(LT_IF, "found .strtab section\n");
            kernel_set_strtab_ptr(sh->addr + KERNEL_START);
        }
    }

    gdt_init();
    kprint_debug(LT_OK, "GDT initialised\n");

    idt_init();
    kprint_debug(LT_OK, "IDT initialised\n");

    isr_init();
    kprint_debug(LT_OK, "ISR initialised\n");

    syscall_init();
    kprint_debug(LT_OK, "syscall initialised\n");

    if(!vfs_init()) {
        kprint_debug(LT_OK, "initialised ramFS on fs (%d)\n", RAMFS_DISK);
        disk_init();
    } else {
        kprint_debug(LT_CR, "failed to initialise FS. not enough memory\n");
        kernel_panic(NULL);
    }

    kprint_debug(LT_IF, "detected disks: ");
    for(int i = 0; i < MAX_FS; i++)
        if(vfs_is_fs_available(i)) kprintf("%d ", i);
    kputchar('\n');

    locale_set_keyboard_layout(KBD_LAYOUT_US);
    kprint_debug(LT_IF, "set keyboard layout to US\n");

    kbd_init();
    kprint_debug(LT_OK, "keyboard initialised\n");

    timer_init();
    kprint_debug(LT_OK, "timer initialised\n");

    // idle process takes its creator eip to continue
    // which is this function
    process_t* idle_proc = process_make_idle();
    if(!idle_proc) {
        kprint_debug(LT_CR, "failed to create idle process. not enough memory\n");
        kernel_panic(NULL);
    }
    kprint_debug(LT_OK, "created idle process\n");

    scheduler_init(idle_proc);
    kprint_debug(LT_OK, "scheduler initialised\n");

    kernel_process = process_new((uint32_t)kmain, false, NULL);
    if(kernel_process) {
        scheduler_add_process(kernel_process);
        kprint_debug(LT_OK, "created kernel main process\n");
    } else {
        kprint_debug(LT_CR, "failed to create main process. not enough memory\n");
        kernel_panic(NULL);
    }

    kprint_debug(LT_IF, "done initialising\n");

    // start interrupts again after setting up everything
    // this will also enable the timer callback (scheduler switch, see kernel/driver/timer.c)
    asm volatile("sti");

    // going idle
    while(true) asm volatile("sti; hlt;");
}

void kernel_proc1() {
    while(true) {
        syscall_sleep(100);
        video_framebuffer_fill_rectangle(20, 20, 40, 40, video_framebuffer_rgb(VIDEO_GREEN));
    }
}
void kernel_proc2() {
    while(true) {
        syscall_sleep(100);
        video_framebuffer_fill_rectangle(20, 20, 40, 40, video_framebuffer_rgb(VIDEO_RED));
    }
}

void kmain() {
    // should the kernel main process the init process?
    // currently the purpose of the kernel main process is to start other processes
    // that should be the job of the init process right?

    kprint_debug(LT_OK, "jumped into main kernel process\n");

    process_t* proc_stdout = process_new((uint32_t)kproc_stdout, false, NULL);
    if(proc_stdout) scheduler_add_process(proc_stdout);
    else kprint_debug(LT_CR, "cannot start standard out process\n");

    process_t* proc_stdin = process_new((uint32_t)kproc_stdin, false, NULL);
    if(proc_stdin) scheduler_add_process(proc_stdin);
    else kprint_debug(LT_CR, "cannot start standard in process\n");

    kprint_debug(LT_IF, "done initialising\n");
    // free now

    syscall_sleep(1000);

    // start the init process
    process_t* init_proc = process_new(0, true, NULL);
    if(init_proc) {
        ELF_ERR load_err = elf_load_to_proc("(0)/bin/keroshell.elf", init_proc);
        if(load_err) {
            FS_ERR ferr = elf_get_err();
            kprintf("file err while loading %s: %d\n", "keroshell.elf", ferr);
        } else {
            syscall_sleep(100);
            scheduler_add_process(init_proc);
            kputs("shell started");
        }
    } else {
        kprintf("failed to start the shell\n");
    }

    syscall_sleep(7000);

    process_t* proc1 = process_new((uint32_t)kernel_proc1, false, NULL);
    if(proc1) scheduler_add_process(proc1);

    process_t* proc2 = process_new((uint32_t)kernel_proc2, false, NULL);
    if(proc2) scheduler_add_process(proc2);

    while(true) {
        asm volatile("sti; hlt;");
    }
}
