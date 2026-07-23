include .env

export BIN_DIR
export OBJ_DIR
export DISK_IMAGE_SIZE

$(shell mkdir $(BIN_DIR) $(BIN_DIR)coreutils/ $(OBJ_DIR))

# kernel defines

C_INCLUDES := -I./kernel/src -I./kernel/include -I./libc/include

LIBC_SRC := $(shell cd libc/src && find -L * -type f -name '*.c' | sort)
C_SRC := $(shell cd kernel/src && find -L * -type f -name '*.c' | sort)
A_SRC := $(shell cd kernel/src && find -L * -type f -name '*.asm' | sort)

LIBC_OBJ := $(addprefix $(OBJ_DIR)libc/, $(LIBC_SRC:.c=.o))
OBJ := $(addprefix $(OBJ_DIR)kernel/, $(C_SRC:.c=.o))
OBJ += $(addprefix $(OBJ_DIR)kernel/, $(addsuffix .o, $(A_SRC)))

COREUTILS_SRC := $(shell cd coreutils && find -L * -type f -name '*.c')
COREUTILS_ELF := $(addprefix $(BIN_DIR)coreutils/, $(COREUTILS_SRC:.c=.elf))

SHELL_SRC := $(shell cd shell/src && find -L * -type f -name '*.c')
SHELL_OBJ := $(addprefix $(OBJ_DIR)shell/, $(SHELL_SRC:.c=.o))

USER_SRC := $(shell cd userapp && find -L * -type f -name '*.c')
USER_ELF := $(addprefix fsfiles/, $(USER_SRC:.c=.elf))

# see .env
DEFINES := -DKERNEL_START=$(KERNEL_START) \
		  -DVMMNGR_TEMP_TABLE=$(VMMNGR_TEMP_TABLE) \
		  -DVMMNGR_TEMP_PD=$(VMMNGR_TEMP_PD) \
		  -DVIDEO_START=$(VIDEO_START) \
		  -DKHEAP_START=$(KHEAP_START) \
		  -DKHEAP_INITIAL_SIZE=$(KHEAP_INITIAL_SIZE) \
		  -DKHEAP_MAX_SIZE=$(KHEAP_MAX_SIZE) \
		  -DRHEAP_START=$(RHEAP_START) \
		  -DRHEAP_INITIAL_SIZE=$(RHEAP_INITIAL_SIZE) \
		  -DRHEAP_MAX_SIZE=$(RHEAP_MAX_SIZE) \
		  -DTIMER_FREQUENCY=$(TIMER_FREQUENCY) \
		  -DUHEAP_START=$(UHEAP_START) \
		  -DUHEAP_INITIAL_SIZE=$(UHEAP_INITIAL_SIZE) \
		  -DUHEAP_MAX_SIZE=$(UHEAP_MAX_SIZE) \

CFLAGS = $(DEFINES) -ffreestanding -O0 -Wall -Wextra -g -MMD -MP
LDFLAGS = -T linker.ld -nostdlib
LDLIBS = -lgcc
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

.PHONY: all libc kernel disk copyfs run run-debug coreutils shell userapp clean clean-all

all: libc kernel coreutils shell userapp disk copyfs

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
$(BIN_DIR)kerosene.elf: $(OBJ_DIR)kernel/kernel_entry.asm.o $(OBJ)
	# use GCC to link instead of LD because LD cannot find the libgcc
	$(CC) $(LDFLAGS) -o $@ $^ -L./bin -lc $(LDLIBS)

# coreutils
$(BIN_DIR)coreutils/%.elf: coreutils/%.c $(BIN_DIR)libc.a
	$(CC) $(DEFINES) -I./libc/include -ffreestanding -nostdlib -e _start -o $@ $< -L./bin -lc -lgcc

# shell
$(OBJ_DIR)shell/%.o: shell/src/%.c
	mkdir -p $$(dirname $@)
	$(CC) $(CFLAGS) -o $@ $(C_INCLUDES) -c $<
$(BIN_DIR)keroshell.elf: $(SHELL_OBJ) $(BIN_DIR)libc.a
	$(CC) $(DEFINES) -I./libc/include -I./kernel/include -ffreestanding -nostdlib -e _start -o $@ $(SHELL_OBJ) -L./bin -lc -lgcc

# user app
fsfiles/%.elf: userapp/%.c
	$(CC) $(DEFINES) -I./libc/include -ffreestanding -nostdlib -e _start -o $@ $< -L./bin -lc -lgcc

libc: $(BIN_DIR)libc.a

kernel: $(BIN_DIR)kerosene.elf

coreutils: $(COREUTILS_ELF)

shell: $(BIN_DIR)keroshell.elf

userapp: $(USER_ELF)

disk:
	./script/gendiskimage.sh $(DISK_IMAGE_SIZE)

copyfs: disk
	./script/mount-device.sh
	sudo cp grub.cfg ./mnt/boot/grub/ # update grub config
	sudo cp $(BIN_DIR)kerosene.elf ./mnt/boot/ # update kernel
	sudo mkdir -p ./mnt/bin/
	sudo cp $(COREUTILS_ELF) ./mnt/bin/ # update coreutils
	sudo cp $(BIN_DIR)keroshell.elf ./mnt/bin/ # update shell
	for file in fsfiles/*; do sudo cp -r $$file ./mnt/; done
	./script/umount-device.sh

run:
	./script/run.sh

run-debug:
	./script/run.sh debug

clean:
	rm -r $(OBJ_DIR) $(BIN_DIR)kerosene.elf $(BIN_DIR)libc.a $(BIN_DIR)coreutils/ $(BIN_DIR)keroshell.elf

clean-all:
	rm -r $(BIN_DIR) $(OBJ_DIR)

DEPS  = $(patsubst %.o,%.d,$(OBJ))
DEPS += $(patsubst %.o,%.d,$(LIBC_OBJ))
DEPS += $(patsubst %.o,%.d,$(SHELL_OBJ))
-include $(DEPS)
