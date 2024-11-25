#include "multiboot.h"

#include "video.h"
#include "ata.h"
#include "kbd.h"
#include "timer.h"
#include "filesystem.h"
#include "locale.h"

#include "system.h"
#include "process.h"
#include "syscall.h"
#include "mem.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "debug.h"

#include "kshell.h"

#include "misc/elf.h"

uint32_t kernel_size;

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
    print_debug(LT_OK, "pmmngr initialised, detected %d MiB of memory\n", memsize/1024/1024);

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
                // TODO: do sth with framebuffer color_info
                break;
            case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
                using_framebuffer = false;
                break;
        }
    }
    else {
        using_framebuffer = false;
        video_addr = VIDEO_TEXTMODE_ADDRESS;
        video_width = 80;
        video_height = 25;
        video_pitch = 160;
        video_bpp = 16;
    }

    // map video ptr
    for(unsigned i = 0; i < video_height * video_pitch; i += MMNGR_PAGE_SIZE)
        vmmngr_map(NULL, video_addr + i, VIDEO_START + i, PTE_WRITABLE);
    if(using_framebuffer) {
        video_vesa_init(
            video_width, video_height,
            video_pitch, video_bpp
        );
        print_debug(LT_OK, "VESA video initialised\n");
    }
    else {
        video_vga_init(video_width, video_height);
        print_debug(LT_OK, "VGA video initialised\n");
    }
}

void disk_init() {
    uint16_t* dump = kmalloc(256 * sizeof(uint16_t));
    if(!dump) {
        print_debug(LT_ER, "not enough memory to initialise disk\n");
        return;
    }

    ATA_PIO_ERR ata_err = ata_pio_init(dump);
    kfree(dump);
    if(ata_err) {
        print_debug(LT_WN, "failed to initialise ATA PIO mode. error code %d\n", ata_err);
        return;
    }

    print_debug(LT_OK, "ATA PIO mode initialised\n");

    bool mbr_err = mbr_load();
    if(mbr_err) {
        print_debug(LT_ER, "cannot load MBR\n");
        return;
    }
    print_debug(LT_OK, "MBR loaded\n");

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
                    print_debug(LT_ER, "failed to initialize FAT32 filesystem on partition %d. error code %d\n", i+1, err);
                else {
                    print_debug(LT_OK, "initialised FAT32 filesystem on partition %d", i+1);
                    printf(" on fs (%d)\n", fs_count);
                    fs_count++;
                }
                break;
            case FS_EXT2:
                print_debug(LT_WN, "EXT2 filesystem in partition %d is not implemented, the partition will be ignored\n", i+1);
                break;
        }
    }
}

void kmain();
void kinit(multiboot_info_t* mbd) {
    extern char kernel_start;
    extern char kernel_end;
    kernel_size = &kernel_end - &kernel_start;

    // greeting msg to let us know we are in the kernel
    // note that this will print into preinit video buffer
    // and will not drawn to screen until video is initialised
    puts("hello");
    printf("this is "); puts("kernosene!");
    printf("build datetime: %s, %s\n", __TIME__, __DATE__);
    printf("kernel size: %d bytes\n", kernel_size);

    // since we have mapped 4MiB from 0x0 to 0xc0000000
    // any physical address under 4MiB can be converted to virtual address
    // by adding 0xc0000000 to it
    // i think that GRUB will not give any address that are larger than 4 MiB
    // except the framebuffer
    // note that the ELF section also included in the kernel (which lies in the first 4MiB)
    // so those physical address of ELF section headers need to offset to KERNEL_START too
    mbd = (void*)mbd + KERNEL_START;

    if(mbd->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME)
        print_debug(LT_IF, "using %s bootloader\n", mbd->boot_loader_name + KERNEL_START);

    // disable interrupts to set up things
    asm volatile("cli");

    if(!(mbd->flags & MULTIBOOT_INFO_MEM_MAP)) kernel_panic(NULL);
    mem_init((void*)mbd->mmap_addr + KERNEL_START, mbd->mmap_length);

    video_init(mbd);

    if(kheap_init()) {
        print_debug(LT_ER, "failed to initialise kernel heap. not enough memory\n");
        kernel_panic(NULL);
    }
    print_debug(LT_OK, "kernel heap initialised\n");

    // if(mbd->flags & MULTIBOOT_INFO_AOUT_SYMS) {
    //     aout_sym = mbd->u.aout_sym;
    // }

    // TODO: implement the a.out option, assumption is bad

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

        if(strcmp(sec_name, ".symtab")) {
            print_debug(LT_IF, "found .symtab section\n");
            kernel_set_symtab_sh_ptr((uint32_t)sh);
        }
        else if(strcmp(sec_name, ".strtab")) {
            print_debug(LT_IF, "found .strtab section\n");
            kernel_set_strtab_ptr(sh->addr + KERNEL_START);
        }
    }

    gdt_init();
    print_debug(LT_OK, "GDT initialised\n");

    idt_init();
    print_debug(LT_OK, "IDT initialised\n");

    isr_init();
    print_debug(LT_OK, "ISR initialised\n");

    uint32_t esp = 0; asm volatile("mov %%esp, %%eax" : "=a" (esp));
    tss_set_stack(esp);
    print_debug(LT_OK, "TSS installed\n");

    syscall_init();
    print_debug(LT_OK, "syscall initialised\n");

    if(!vfs_init()) {
        print_debug(LT_OK, "initialised ramFS on fs (%d)\n", RAMFS_DISK);
        disk_init();
    }
    else print_debug(LT_ER, "failed to initialise FS. not enough memory\n");

    print_debug(LT_IF, "all disk detected: ");
    for(int i = 0; i < MAX_FS; i++)
        if(vfs_is_fs_available(i)) printf("%d ", i);
    putchar('\n');

    locale_set_keyboard_layout(KBD_LAYOUT_US);
    print_debug(LT_IF, "set keyboard layout to US\n");

    kbd_init();
    print_debug(LT_OK, "keyboard initialised\n");

    timer_init();
    print_debug(LT_OK, "timer initialised\n");

    // add kernel process
    // there must be at least one process in the scheduler
    kernel_process = process_new((uint32_t)kmain, 0, false);
    if(!kernel_process) {
        print_debug(LT_CR, "failed to initialise kernel process. not enough memory\n");
        kernel_panic(NULL);
    }
    print_debug(LT_IF, "created kernel main process\n");
    scheduler_init(kernel_process);
    print_debug(LT_OK, "scheduler initialised\n");

    // start interrupts again after setting up everything
    // this will also start the scheduler and cause a process switch to kmain
    asm volatile("sti");

    // wait for process switch
    while(true);
}

void load_elf_file(char* path) {
    fs_t* fs = vfs_getfs(0);
    if(!fs) return;

    fs_node_t elf;
    FS_ERR find_err = fs_find(&fs->root_node, path, &elf);
    if(find_err) {
        printf("cannot find '%s', error %d\n", path, find_err);
        return;
    }

    printf("found %s\n", path);

    void* addr = kmalloc(elf.size);
    if(!addr) {
        puts("OOM!");
        return;
    }

    uint32_t entry;
    ELF_ERR err = elf_load(&elf, addr, vmmngr_get_page_directory(), &entry);
    kfree(addr);
    if(err) {
        printf("failed to load elf, err %d\n", err);
        return;
    }
    else puts("elf file loaded");

    printf("jumping to entry (%x)\n", entry);
    int (*prog)(void) = (void*)entry;
    int exit_code = prog();
    printf("program exited with code %d\n", exit_code);
}

void kernel_proc1() {
    int ret;
    while(true) {
        SYSCALL_1P(SYSCALL_SLEEP, ret, 100);
        video_vesa_fill_rectangle(20, 20, 40, 40, video_vesa_rgb(VIDEO_GREEN));
    }
    // SYSCALL_0P(SYSCALL_KILL_PROCESS, ret);
}
void kernel_proc2() {
    int ret;
    while(true) {
        SYSCALL_1P(SYSCALL_SLEEP, ret, 100);
        video_vesa_fill_rectangle(20, 20, 40, 40, video_vesa_rgb(VIDEO_RED));
    }
    // SYSCALL_0P(SYSCALL_KILL_PROCESS, ret);
}

void kmain() {
    print_debug(LT_OK, "done initialising\n");

    // if(shell_init()) puts("not enough memory for kshell.");
    //
    // process_t* shell_proc = process_new((uint32_t)shell_start, 0, false);
    // if(shell_proc) scheduler_add_process(shell_proc);
    //
    // process_t* proc1 = process_new((uint32_t)kernel_proc1, 0, false);
    // process_t* proc2 = process_new((uint32_t)kernel_proc2, 0, false);
    // if(proc1) scheduler_add_process(proc1);
    // if(proc2) scheduler_add_process(proc2);

    // int ret;
    // SYSCALL_1P(SYSCALL_SLEEP, ret, 1000);
    // load_elf_file("hi.elf");

    fs_node_t* root = &vfs_getfs(31)->root_node;

    fs_node_t ngu;
    fs_touch(root, "ngu", &ngu);

    char data[] = "What is Lorem Ipsum?\n\nLorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.\n\nWhy do we use it?\n\nIt is a long established fact that a reader will be distracted by the readable content of a page when looking at its layout. The point of using Lorem Ipsum is that it has a more-or-less normal distribution of letters, as opposed to using 'Content here, content here', making it look like readable English. Many desktop publishing packages and web page editors now use Lorem Ipsum as their default model text, and a search for 'lorem ipsum' will uncover many web sites still in their infancy. Various versions have evolved over the years, sometimes by accident, sometimes on purpose (injected humour and the like).\n\nWhere does it come from?\n\nContrary to popular belief, Lorem Ipsum is not simply random text. It has roots in a piece of classical Latin literature from 45 BC, making it over 2000 years old. Richard McClintock, a Latin professor at Hampden-Sydney College in Virginia, looked up one of the more obscure Latin words, consectetur, from a Lorem Ipsum passage, and going through the cites of the word in classical literature, discovered the undoubtable source. Lorem Ipsum comes from sections 1.10.32 and 1.10.33 of \"de Finibus Bonorum et Malorum\" (The Extremes of Good and Evil) by Cicero, written in 45 BC. This book is a treatise on the theory of ethics, very popular during the Renaissance. The first line of Lorem Ipsum, \"Lorem ipsum dolor sit amet..\", comes from a line in section 1.10.32.\nThe standard chunk of Lorem Ipsum used since the 1500s is reproduced below for those interested. Sections 1.10.32 and 1.10.33 from \"de Finibus Bonorum et Malorum\" by Cicero are also reproduced in their exact original form, accompanied by English versions from the 1914 translation by H. Rackham.\n\nWhere can I get some?\n\nThere are many variations of passages of Lorem Ipsum available, but the majority have suffered alteration in some form, by injected humour, or randomised words which don't look even slightly believable. If you are going to use a passage of Lorem Ipsum, you need to be sure there isn't anything embarrassing hidden in the middle of text. All the Lorem Ipsum generators on the Internet tend to repeat predefined chunks as necessary, making this the first true generator on the Internet. It uses a dictionary of over 200 Latin words, combined with a handful of model sentence structures, to generate Lorem Ipsum which looks reasonable. The generated Lorem Ipsum is therefore always free from repetition, injected humour, or non-characteristic words etc.";

    FILE f;
    file_open(&f, &ngu, FILE_WRITE);
    file_write(&f, (uint8_t*)data, strlen(data)+1);
    file_close(&f);

    fs_find(root, "ngu", &ngu);
    file_open(&f, &ngu, FILE_READ);

    FS_ERR err;
    const int resolution = 1024; // try changing this!!!
    char buff[resolution];
    while((err = file_read(&f, (uint8_t*)buff, resolution)) != ERR_FS_EOF) {
        for(unsigned i = 0; i < resolution; i++)
            putchar(buff[i]);
    }
    for(unsigned i = 0; i < f.node->size % resolution; i++)
        putchar(buff[i]);
    file_close(&f);

    puts("\n\n\ndone");
    printf("pos: %d\n", f.position);

    while(true);
}
