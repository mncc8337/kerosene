#include "multiboot.h"
#include "system.h"
#include "kpanic.h"
#include "video.h"
#include "mem.h"
#include "ata.h"
#include "kbd.h"
#include "timer.h"
#include "syscall.h"
#include "filesystem.h"

#include "stdio.h"
#include "debug.h"

#include "kshell.h"

#include "access/my_avt.h"

extern uint32_t kernel_start;
extern uint32_t kernel_end;

// page-aligned kernel size
uint32_t kernel_size;

char freebuff[512];

int FS_ID = 0;
fs_t* fs;

int video_addr = 0;
int video_width = 0;
int video_height = 0;
int video_pitch = 0;
int video_bpp = 0;

void mem_init(uint32_t mmap_addr, uint32_t mmap_length) {
    // get memsize
    size_t memsize = 0;
    for(unsigned int i = 0; i < mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*)(mmap_addr + i);
        // ignore memory higher than 4GiB
        if((mmmt->addr >> 32) > 0 || (mmmt->len  >> 32) > 0) continue;
        // probaly bugs
        if(i > 0 && mmmt->addr == 0) continue;

        memsize += mmmt->len;
    }
    pmmngr_init(kernel_end+1, memsize);

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
    pmmngr_deinit_region(kernel_start, kernel_size); // kernel
    pmmngr_deinit_region(kernel_end + 1, pmmngr_get_size()/MMNGR_BLOCK_SIZE); // pmmngr
    pmmngr_deinit_region(video_addr, video_height * video_pitch); // video

    pmmngr_update_usage(); // always run this after init and deinit regions
    print_debug(LT_OK, "initialised pmmngr with %d MiB\n", pmmngr_get_free_size()/1024/1024);

    // vmmngr init
    MEM_ERR merr = vmmngr_init();
    if(merr != ERR_MEM_SUCCESS) {
        print_debug(LT_CR, "error while enabling paging. system halted. error code %x", merr);
        kpanic();
    }
    
    // map video address before printing anything
    virtual_addr_t virt_video_addr = 0xc0000000 + kernel_size;
    for(int i = 0; i < video_height * video_pitch; i += MMNGR_PAGE_SIZE)
        vmmngr_map_page(video_addr + i, virt_video_addr + i);
    if(video_using_framebuffer()) video_framebuffer_set_ptr(virt_video_addr);
    else video_textmode_set_ptr(virt_video_addr);
    video_addr = virt_video_addr;

    print_debug(LT_OK, "paging enabled\n");
}
void disk_init() {
    if(!ata_pio_init((uint16_t*)freebuff)) {
        print_debug(LT_WN, "failed to initialise ATA PIO mode\n");
        return;
    }

    print_debug(LT_OK, "ATA PIO mode initialised\n");

    bool mbr_ok = mbr_load();
    if(!mbr_ok) {
        print_debug(LT_ER, "cannot load MBR\n");
        return;
    }
    print_debug(LT_OK, "MBR loaded\n");

    for(int i = 0; i < 4; i++) {
        partition_entry_t part = mbr_get_partition_entry(i);
        if(part.sector_count == 0) continue;

        FS_ERR err;
        switch(fs_detect(part)) {
            case FS_EMPTY:
                break;
            case FS_FAT32:
                err = fat32_init(part, FS_ID);
                if(err != ERR_FS_SUCCESS) {
                    print_debug(LT_ER, "failed to initialize FAT32 filesystem in partition %d. error code %d\n", i+1, err);
                    break;
                }
                print_debug(LT_OK, "initialised FAT32 filesystem in partition %d\n", i+1);
                fs = fs_get(FS_ID);
                break;
            case FS_EXT2:
                print_debug(LT_WN, "EXT2 filesystem in partition %d is not implemented, the partition will be ignored\n", i+1);
                break;
        }
    }

    if(fs->root_node.valid) {
        fs->root_node.name[0] = '/';
        fs->root_node.name[1] = '\0';
    }
}

void kmain(multiboot_info_t* mbd, unsigned int magic) {
    // calculate kernel_size
    kernel_size = kernel_end - kernel_start;
    if(kernel_size % MMNGR_PAGE_SIZE > 0) {
        kernel_size += MMNGR_PAGE_SIZE - (kernel_size % MMNGR_PAGE_SIZE);
        kernel_end = kernel_start + kernel_size - 1;
    }

    // disable interrupts at the start to set up things
    asm volatile("cli");

    // init video
    if(mbd->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) {
        video_addr = mbd->framebuffer_addr;
        video_width = mbd->framebuffer_width;
        video_height = mbd->framebuffer_height;
        video_pitch = mbd->framebuffer_pitch;
        video_bpp = mbd->framebuffer_bpp;

        switch(mbd->framebuffer_type) {
            case MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED:
                // very rare, not likely to happend
                // so let's just not support it hehe
                break;
            case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
                // TODO: do sth with framebuffer color_info
                video_framebuffer_set_ptr(video_addr);
                video_framebuffer_init(video_pitch,
                                       video_width,
                                       video_height,
                                       video_bpp);
                break;
            case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
                video_textmode_set_ptr(video_addr);
                video_textmode_init(video_width, video_height);
                break;
        }
    }
    else {
        // textmode video memory at VIDEO_TEXTMODE_ADDRESS should be available
        // with or without GRUB
        video_textmode_init(80, 25);
        video_textmode_set_ptr(VIDEO_TEXTMODE_ADDRESS);
        video_addr = VIDEO_TEXTMODE_ADDRESS;
        video_width = 80;
        video_height = 25;
        video_pitch = 160;
        video_bpp = 16;
    }

    // greeting msg to let us know we are in the kernel
    video_set_attr(VIDEO_LIGHT_CYAN, VIDEO_BLACK); puts("hello");
    video_set_attr(VIDEO_LIGHT_GREEN, VIDEO_BLACK); printf("this is ");
    video_set_attr(VIDEO_LIGHT_RED, VIDEO_BLACK);   puts("kernosene!");
    video_set_attr(VIDEO_LIGHT_GREY, VIDEO_BLACK);
    printf("build datetime: %s, %s\n", __TIME__, __DATE__);

    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        print_debug(LT_CR, "invalid magic number. system halted\n");
        kpanic();
    }

    if(mbd->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME)
        print_debug(LT_IF, "using %s bootloader\n", mbd->boot_loader_name);

    gdt_init();
    print_debug(LT_OK, "GDT initialised\n");
    uint32_t esp = 0;
    asm volatile("mov %%esp, %%eax" : "=a" (esp));
    tss_install(0x10, esp);
    print_debug(LT_OK, "TSS installed\n");
    idt_init();
    print_debug(LT_OK, "IDT initialised\n");
    isr_init();
    print_debug(LT_OK, "ISR initialised\n");

    if(!(mbd->flags & MULTIBOOT_INFO_MEM_MAP)) {
        print_debug(LT_CR, "no memory map given by bootloader. system halted\n");
        kpanic();
    }
    mem_init(mbd->mmap_addr, mbd->mmap_length);

    disk_init();

    syscall_init();
    print_debug(LT_OK, "syscall initialised\n");

    kbd_init();

    timer_init();

    // start interrupts again after setting up everything
    asm volatile("sti");

    print_debug(LT_IF, "done initialising\n");

    // if(video_using_framebuffer()) {
    //     char* my_avt_data = my_avt_header_data;
    //     for(int y = 0; y < (signed)my_avt_height; y++) {
    //         for(int x = 0; x < (signed)my_avt_width; x++) {
    //             int pixel[3];
    //             HEADER_PIXEL(my_avt_data, pixel);
    //             uint32_t color = (pixel[2] << 24) | (pixel[0] << 16) | (pixel[1] << 8);
    //             video_framebuffer_plot_pixel(x, y, color);
    //         }
    //     }
    // }

    shell_set_root_node(fs->root_node);
    shell_start();

    // // i cannot get this to work :(
    // // enter_usermode();

    // // test "syscall"
    // // the privilege is still ring 0, though
    // asm volatile("xor %eax, %eax; int $0x80"); // SYS_SYSCALL_TEST
    // asm volatile("xor %eax, %eax; inc %eax; int $0x80"); // SYS_PUTCHAR

    while(true);
}
