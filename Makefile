include .env

$(shell mkdir $(BIN_DIR) $(OBJ_DIR))

# kernel defines

C_INCLUDES := -I./kernel/src -I./kernel/include -I./libc/include

LIBC_SRC := $(shell cd libc/src && find -L * -type f -name '*.c' | sort)
C_SRC := $(shell cd kernel/src && find -L * -type f -name '*.c' | sort)
A_SRC := $(shell cd kernel/src && find -L * -type f -name '*.asm' | sort)

LIBC_OBJ := $(addprefix $(OBJ_DIR)libc/, $(LIBC_SRC:.c=.o))
LIBK_OBJ := $(addprefix $(OBJ_DIR)libk/, $(LIBC_SRC:.c=.o))
OBJ := $(addprefix $(OBJ_DIR)kernel/, $(C_SRC:.c=.o))
OBJ += $(addprefix $(OBJ_DIR)kernel/, $(addsuffix .o, $(A_SRC)))

USER_SRC := $(shell cd userapp && find -L * -type f -name '*.c')
USER_ELF := $(addprefix fsfiles/, $(USER_SRC:.c=.elf))

# see .env
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

.PHONY: all libk libc kernel disk copyfs run run-debug userapp clean clean-all

all: libk libc kernel disk copyfs userapp

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

# user app
fsfiles/%.elf: userapp/%.c
	$(CC) -I./libc/include -ffreestanding -nostdlib -Ttext 0x0 -lgcc -o $@ $< -L./bin -lc

libc: $(BIN_DIR)libc.a

libk: $(BIN_DIR)libk.a

kernel: $(BIN_DIR)kerosene.elf

disk:
	./script/gendiskimage.sh $(DISK_IMAGE_SIZE)

copyfs: disk
	./script/cpyfile.sh grub.cfg ./mnt/boot/grub/ # update grub config
	./script/cpyfile.sh bin/kerosene.elf ./mnt/boot/ # update kernel
	for file in fsfiles/*; do ./script/cpyfile.sh $$file ./mnt/; done

run:
	./script/run.sh

run-debug:
	./script/run.sh debug

userapp: $(USER_ELF)

clean:
	rm -r $(BIN_DIR) $(OBJ_DIR)

clean-all: clean
	rm disk.img disk-backup.img

DEPS  = $(patsubst %.o,%.d,$(OBJ))
DEPS += $(patsubst %.o,%.d,$(LIBC_OBJ))
DEPS += $(patsubst %.o,%.d,$(LIBK_OBJ))
-include $(DEPS)
