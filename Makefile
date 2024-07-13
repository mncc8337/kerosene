TARGET = i686-elf
CROSS_COMPILER_LOC = ./cross/bin/

CC = $(CROSS_COMPILER_LOC)$(TARGET)-gcc
# LD = $(CROSS_COMPILER_LOC)$(TARGET)-ld
GDB = gdb
ASM = nasm
QEMU = qemu-system-i386

LIBC_SRC = libc/stdio/*.c \
		   libc/stdlib/*.c \
		   libc/string/*.c \

C_SRC = kernel/src/*.c \
		kernel/src/utils/*.c \
		kernel/src/system/*.c \
		kernel/src/driver/*.c \
		kernel/src/mem/*.c \

LIBC_OBJ = $(addsuffix .o, $(basename $(notdir $(wildcard $(LIBC_SRC)))))
OBJ = $(addsuffix .o, $(basename $(notdir $(wildcard $(C_SRC)))))

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

C_INCLUDES = -I./kernel/include -I./libc/include

# CFLAGS = $(DEFINES) -ffreestanding -m32 -mtune=i386 -fno-pie -nostdlib -nostartfiles
# LDFLAGS = -T linker.ld -m elf_i386 -nostdlib --nmagic
CFLAGS = $(DEFINES) -ffreestanding -O2 -Wall -Wextra -std=gnu99
LDFLAGS = -nostdlib -lgcc
NASMFLAGS = $(DEFINES) -f elf32 -F dwarf

QEMUFLAGS = -m 128M \
			-debugcon stdio \
			-no-reboot \
			-accel tcg,thread=single \
			-usb \

.PHONY: all run debug libc libk bootloader kernel disk

all: disk.img

%.o: libc/stdio/%.c
	$(CC) $(CFLAGS) -MD -o $@ $(C_INCLUDES) -c $<
%.o: libc/stdlib/%.c
	$(CC) $(CFLAGS) -MD -o $@ $(C_INCLUDES) -c $<
%.o: libc/string/%.c
	$(CC) $(CFLAGS) -MD -o $@ $(C_INCLUDES) -c $<
libk.a: $(LIBC_OBJ)
	$(CROSS_COMPILER_LOC)$(TARGET)-ar rcs $@ $^
	@echo done building libk
	# rm $(LIBC_OBJ)
libc.a: $(LIBC_OBJ)
	@echo libc is not ready for building, yet

stage1.bin: boot/stage1.asm
	$(ASM) -f bin -o $@ -I./boot/include $< $(DEFINES)
stage2.bin: boot/stage2.asm
	$(ASM) -f bin -o $@ -I./boot/include $< $(DEFINES)
bootloader.bin: stage1.bin stage2.bin
	cat $^ > $@

kernel_entry.o: kernel/src/kernel_entry.asm
	$(ASM) $(NASMFLAGS) -o $@ $<

%.o: kernel/src/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
%.o: kernel/src/driver/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
%.o: kernel/src/system/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
%.o: kernel/src/utils/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
%.o: kernel/src/mem/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<

kernel.elf: kernel_entry.o $(OBJ) libk.a
	# use GCC to link instead of LD because LD cannot find the libgcc
	$(CC) $(LDFLAGS) -o $@ -Ttext $(KERNEL_ADDR) $^ -L./ -lk
kernel.bin: kernel.elf
	$(CROSS_COMPILER_LOC)$(TARGET)-objcopy -O binary $< $@
	dd if=/dev/zero of=$@ bs=512 count=0 seek=$(KERNEL_SECTOR_COUNT)

disk.img: bootloader.bin kernel.bin
	cat $^ > $@

libc:
	@echo libc is not ready to build, yet

libk: libk.a

bootloader: bootloader.bin

kernel: kernel_entry.o $(OBJ)

disk: disk.img

run: disk.img
	$(QEMU) -drive file=disk.img,format=raw $(QEMUFLAGS)
debug: disk.img
	$(QEMU) -drive file=disk.img,format=raw $(QEMUFLAGS) -s -S &
	$(GDB) kernel.elf \
		-ex 'target remote localhost:1234' \
		-ex 'layout src' \
		-ex 'layout reg' \
		-ex 'break main' \
		-ex 'continue'
clean:
	rm *.bin *.o *.d *.a *.elf *.img
