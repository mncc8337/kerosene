include .env

TARGET ?= i686-elf
CROSS_COMPILER_LOC ?= ./cross/bin/

CC ?= $(CROSS_COMPILER_LOC)$(TARGET)-gcc
LD ?= $(CROSS_COMPILER_LOC)$(TARGET)-ld
AR ?= $(CROSS_COMPILER_LOC)$(TARGET)-ar
AS ?= nasm

$(shell mkdir $(BIN_DIR) $(OBJ_DIR))

# kernel defines

C_INCLUDES := -I./kernel/src -I./kernel/include -I./libc/include

LIBC_SRC := $(shell cd libc/src && find -L * -type f -name '*.c' | LC_ALL=C sort)
C_SRC := $(shell cd kernel/src && find -L * -type f -name '*.c' | LC_ALL=C sort)
A_SRC := $(shell cd kernel/src && find -L * -type f -name '*.asm' | LC_ALL=C sort)

LIBC_OBJ := $(addprefix $(OBJ_DIR)libc/, $(LIBC_SRC:.c=.o))
LIBK_OBJ := $(addprefix $(OBJ_DIR)libk/, $(LIBC_SRC:.c=.o))
OBJ := $(addprefix $(OBJ_DIR)kernel/, $(C_SRC:.c=.o))
OBJ += $(addprefix $(OBJ_DIR)kernel/, $(addsuffix .o, $(A_SRC)))

# kernel configs

KERNEL_START      ?= 0xc0000000
# address reserved for editing page directories
VMMNGR_RESERVED   ?= 0xc03fe000
# address for current page directory to be mapped in
VMMNGR_PD         ?= 0xc03ff000
VIDEO_START       ?= 0xc0400000
# kernel heap
KHEAP_START       ?= 0xc0800000
KHEAP_INITAL_SIZE ?= 0x100000
KHEAP_MAX_SIZE    ?= 0x1000000
# timer
TIMER_FREQUENCY   ?= 1000
# userspace
UHEAP_START       ?= 0x100000
UHEAP_INITAL_SIZE ?= 0x100000
UHEAP_MAX_SIZE    ?= 0x1000000

DEFINES := -DKERNEL_START=$(KERNEL_START) \
		  -DVMMNGR_RESERVED=$(VMMNGR_RESERVED) \
		  -DVMMNGR_PD=$(VMMNGR_PD) \
		  -DVIDEO_START=$(VIDEO_START) \
		  -DKHEAP_START=$(KHEAP_START) \
		  -DKHEAP_INITAL_SIZE=$(KHEAP_INITAL_SIZE) \
		  -DKHEAP_MAX_SIZE=$(KHEAP_MAX_SIZE) \
		  -DTIMER_FREQUENCY=$(TIMER_FREQUENCY) \
		  -DUHEAP_START=$(UHEAP_START) \
		  -DUHEAP_INITIAL_SIZE=$(UHEAP_INITAL_SIZE) \
		  -DUHEAP_MAX_SIZE=$(UHEAP_MAX_SIZE) \

CFLAGS = $(DEFINES) -ffreestanding -O2 -Wall -Wextra -g -MMD -MP
LDFLAGS = -T linker.ld -nostdlib -lgcc
ASFLAGS = $(DEFINES) -f elf32 -F dwarf

ifdef NO_CROSS_COMPILER
	CC := gcc
	LD := ld
	AR := ar

	CFLAGS := $(DEFINES) -Wall -Wextra \
			 -ffreestanding -m32 -mtune=i386 -fno-pie -nostdlib -nostartfiles \
			 -fno-stack-protector \
			 -g -MMD -MP
	LDFLAGS := -T linker.ld -nostdlib -m32 -fno-pie -lgcc
endif


.PHONY: all libc libk kernel disk copyfs run run-debug clean clean-all

all: $(BIN_DIR)kerosene.elf libk libc disk copyfs

# libk
$(OBJ_DIR)libk/%.o: libc/src/%.c
	mkdir -p $$(dirname $@)
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $< -D__is_libk

$(BIN_DIR)libk.a: $(LIBK_OBJ)
	$(AR) rcs $@ $^
	@echo done building libk

# libc
$(OBJ_DIR)libc/%.o: libc/src/%.c
	mkdir -p $$(dirname $@)
	echo $@
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
$(BIN_DIR)libc.a: $(LIBC_OBJ)
	$(AR) rcs $@ $^
	@echo done building libc

# kernel

$(OBJ_DIR)kernel/%.o: kernel/src/%.c
	mkdir -p $$(dirname $@)
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<

$(OBJ_DIR)kernel/%.asm.o: kernel/src/%.asm
	mkdir -p $$(dirname $@)
	$(AS) $(ASFLAGS) -o $@ $<

$(BIN_DIR)kerosene.elf: $(OBJ_DIR)kernel/kernel_entry.asm.o $(OBJ) $(BIN_DIR)libk.a
	# use GCC to link instead of LD because LD cannot find the libgcc
	$(CC) $(LDFLAGS) -o $@ $^ -L./bin -lk

libc: $(BIN_DIR)libc.a

libk: $(BIN_DIR)libk.a

kernel: $(BIN_DIR)kerosene.elf

disk: $(BIN_DIR)kerosene.elf
	./script/gendiskimage.sh 65536

copyfs: disk
	./script/cpyfile.sh grub.cfg ./mnt/boot/grub/ # update grub config
	./script/cpyfile.sh bin/kerosene.elf ./mnt/boot/ # update kernel
	for file in fsfiles/*; do ./script/cpyfile.sh $$file ./mnt/; done

run:
	./script/run.sh

run-debug:
	./script/run.sh debug

clean:
	rm -r $(BIN_DIR) $(OBJ_DIR)

clean-all: clean
	rm disk.img disk-backup.img

DEPS  = $(patsubst %.o,%.d,$(OBJ))
DEPS += $(patsubst %.o,%.d,$(LIBC_OBJ))
DEPS += $(patsubst %.o,%.d,$(LIBK_OBJ))
-include $(DEPS)
