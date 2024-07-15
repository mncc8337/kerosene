# note
this whole directory is abandoned and it is here for legacy purpose  
you should not use it because it **is broken**  
i quit making this because i cannot get the FAT12 load file to work so i just gave up and use GRUB
# bootloader
a not working 2-stage bootloader for SOS.
## build
this is a simplified makefile for the bootloader.  
```
ASM = nasm
# number of sectors that the kernel will fit in
KERNEL_SECTOR_COUNT = 50
# memory address that second stage bootloader will be loaded to
SECOND_STAGE_ADDR = 0x7e00
# memory address that memmap entry count will be loaded to
MMAP_ENTRY_CNT_ADDR = 0x8000
# memory address that memmap will be loaded to
MMAP_ADDR = 0xB00
# memory address that the kernel will be loaded to
KERNEL_ADDR = 0x1000

DEFINES = -D__is_libk \
			-DKERNEL_SECTOR_COUNT=$(KERNEL_SECTOR_COUNT) \
			-DKERNEL_ADDR=$(KERNEL_ADDR) \
			-DAFTER_KERNEL_ADDR="$(KERNEL_ADDR) + $(KERNEL_SECTOR_COUNT)*512" \
			-DSECOND_STAGE_ADDR=$(SECOND_STAGE_ADDR) \
			-DMMAP_ENTRY_CNT_ADDR=$(MMAP_ENTRY_CNT_ADDR) \
			-DMMAP_ADDR=$(MMAP_ADDR) \

stage1.bin: boot/stage1.asm
	$(ASM) -f bin -o $@ -I./boot/include $< $(DEFINES)
stage2.bin: boot/stage2.asm
	$(ASM) -f bin -o $@ -I./boot/include $< $(DEFINES)
```
