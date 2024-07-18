TARGET = i686-elf
CROSS_COMPILER_LOC = ./cross/bin/

CC = $(CROSS_COMPILER_LOC)$(TARGET)-gcc
# LD = $(CROSS_COMPILER_LOC)$(TARGET)-ld
ASM = nasm

LIBC_SRC = libc/stdio/*.c \
		   libc/stdlib/*.c \
		   libc/string/*.c \

C_SRC = kernel/src/*.c \
		kernel/src/utils/*.c \
		kernel/src/system/*.c \
		kernel/src/driver/*.c \
		kernel/src/driver/ata/*.c \
		kernel/src/filesystem/*.c \
		kernel/src/mem/*.c \

ASM_SRC = kernel/src/system/*.asm \
		  kernel/src/mem/*.asm \

LIBC_OBJ = $(addsuffix .o, $(basename $(notdir $(wildcard $(LIBC_SRC)))))
OBJ  = $(addsuffix .o, $(basename $(notdir $(wildcard $(C_SRC)))))
OBJ += $(addsuffix .o, $(notdir $(wildcard $(ASM_SRC))))

# memory address that the kernel will be loaded to
KERNEL_ADDR = 0x1000

DEFINES = -D__is_libk

C_INCLUDES = -I./kernel/include -I./libc/include

# CFLAGS = $(DEFINES) -ffreestanding -m32 -mtune=i386 -fno-pie -nostdlib -nostartfiles
# LDFLAGS = -T linker.ld -m elf_i386 -nostdlib --nmagic
CFLAGS = $(DEFINES) -ffreestanding -O2 -Wall -Wextra -std=gnu99
LDFLAGS = -T linker.ld -nostdlib -lgcc
NASMFLAGS = $(DEFINES) -f elf32 -F dwarf

.PHONY: all libc libk kernel disk

all: kernel.bin

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

multiboot-header.o: multiboot-header.asm
	$(ASM) $(NASMFLAGS) -o $@ $<

%.o: kernel/src/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
%.o: kernel/src/utils/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
%.o: kernel/src/system/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
%.o: kernel/src/driver/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
%.o: kernel/src/driver/ata/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
%.o: kernel/src/filesystem/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
%.o: kernel/src/mem/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<

%.asm.o: kernel/src/system/%.asm
	$(ASM) $(NASMFLAGS) -o $@ $<
%.asm.o: kernel/src/mem/%.asm
	$(ASM) $(NASMFLAGS) -o $@ $<

kernel.bin: multiboot-header.o $(OBJ) libk.a
	# use GCC to link instead of LD because LD cannot find the libgcc
	$(CC) $(LDFLAGS) -o $@ $^ -L./ -lk

libc:
	@echo libc is not ready to build, yet

libk: libk.a

kernel: kernel.bin
