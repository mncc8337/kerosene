TARGET = i686-elf
CROSS_COMPILER_LOC = ./cross/bin/

CC = $(CROSS_COMPILER_LOC)$(TARGET)-gcc
LD = $(CROSS_COMPILER_LOC)$(TARGET)-ld
AR = $(CROSS_COMPILER_LOC)$(TARGET)-ar
ASM = nasm

BIN = bin/

LIBC_SRC = libc/stdio/*.c \
		   libc/stdlib/*.c \
		   libc/string/*.c \
		   libc/time/*.c \

C_SRC = kernel/src/*.c \
		kernel/src/misc/*.c \
		kernel/src/data_structure/*.c \
		kernel/src/system/*.c \
		kernel/src/driver/*.c \
		kernel/src/driver/video/*.c \
		kernel/src/driver/ata/*.c \
		kernel/src/filesystem/*.c \
		kernel/src/mem/*.c \

ASM_SRC = kernel/src/system/*.asm \
		  kernel/src/mem/*.asm \

_LIBC_OBJ = $(addsuffix .o, $(basename $(notdir $(wildcard $(LIBC_SRC)))))
_OBJ  = $(addsuffix .o, $(basename $(notdir $(wildcard $(C_SRC)))))
_OBJ += $(addsuffix .o, $(notdir $(wildcard $(ASM_SRC))))

LIBC_OBJ = $(addprefix $(BIN), $(_LIBC_OBJ))
OBJ = $(addprefix $(BIN), $(_OBJ))

DEFINES = -DTIMER_FREQUENCY=1000 \
		  -DMMNGR_KHEAP_START=0xc0000000 \
		  -DMMNGR_KHEAP_INITAL_SIZE=0x100000 \
		  -D__is_libk \

C_INCLUDES = -I./kernel/src -I./kernel/include -I./libc/include

CFLAGS = $(DEFINES) -ffreestanding -O2 -Wall -Wextra -std=gnu99 -g -MMD -MP
LDFLAGS = -T linker.ld -nostdlib -lgcc
NASMFLAGS = $(DEFINES) -f elf32 -F dwarf

ifdef NO_CROSS_COMPILER
	CC = gcc
	LD = ld
	AR = ar

	CFLAGS = $(DEFINES) -Wall -Wextra \
			 -ffreestanding -m32 -mtune=i386 -fno-pie -nostdlib -nostartfiles \
			 -fno-stack-protector \
			 -g -MMD -MP
	LDFLAGS = -T linker.ld -nostdlib -m32 -fno-pie -lgcc
endif


.PHONY: all libc libk kernel disk copyfs run run-debug clean clean-all

all: $(BIN) $(BIN)kerosene.bin disk copyfs

$(BIN):
	mkdir $(BIN)

$(BIN)%.o: libc/stdio/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
$(BIN)%.o: libc/stdlib/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
$(BIN)%.o: libc/string/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
$(BIN)%.o: libc/time/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
$(BIN)libk.a: $(LIBC_OBJ)
	$(AR) rcs $@ $^
	@echo done building libk
$(BIN)libc.a: $(LIBC_OBJ)
	@echo libc is not ready for build, yet

$(BIN)kernel_entry.o: kernel/src/kernel_entry.asm
	$(ASM) $(NASMFLAGS) -o $@ $<

$(BIN)%.o: kernel/src/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
$(BIN)%.o: kernel/src/misc/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
$(BIN)%.o: kernel/src/data_structure/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
$(BIN)%.o: kernel/src/system/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
$(BIN)%.o: kernel/src/driver/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
$(BIN)%.o: kernel/src/driver/video/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
$(BIN)%.o: kernel/src/driver/ata/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
$(BIN)%.o: kernel/src/filesystem/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
$(BIN)%.o: kernel/src/mem/%.c
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<

$(BIN)%.asm.o: kernel/src/system/%.asm
	$(ASM) $(NASMFLAGS) -o $@ $<
$(BIN)%.asm.o: kernel/src/mem/%.asm
	$(ASM) $(NASMFLAGS) -o $@ $<

$(BIN)kerosene.bin: $(BIN)kernel_entry.o $(OBJ) $(BIN)libk.a
	# use GCC to link instead of LD because LD cannot find the libgcc
	$(CC) $(LDFLAGS) -o $@ $^ -L./bin -lk

libc: $(BIN)libc.a

libk: $(BIN)libk.a

kernel: $(BIN)kerosene.bin

disk: kernel
	./script/gendiskimage.sh 65536

copyfs: disk
	./script/cpyfile.sh grub.cfg ./mnt/boot/grub/ # update grub config
	./script/cpyfile.sh bin/kerosene.bin ./mnt/boot/ # update kernel
	for file in fsfiles/*; do ./script/cpyfile.sh $$file ./mnt/; done

run:
	./script/run.sh

run-debug:
	./script/run.sh debug

clean:
	rm -r $(BIN)

clean-all: clean
	rm disk.img disk-backup.img

DEPS  = $(patsubst %.o,%.d,$(OBJ))
DEPS += $(patsubst %.o,%.d,$(LIBC_OBJ))
-include $(DEPS)
