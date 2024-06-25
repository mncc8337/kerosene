C_SRC = $(wildcard kernel/*.c kernel/driver/*.c)
C_OBJ = $(addsuffix .o, $(basename $(notdir $(C_SRC))))

KERNEL_OFFSET = 0x1000

C_FLAGS = -ffreestanding -m32 -fno-pie
LD_FLAGS = -m elf_i386
NASM_FLAGS = -f elf

all: disk.img

bootsect.bin: boot/bootsect.asm
	nasm -f bin -o $@ -I./boot/include $<

kernel_entry.o: kernel/kernel_entry.asm
	nasm $(NASM_FLAGS) -o $@ $<
%.o: kernel/%.c
	gcc $(C_FLAGS) -o $@ -I./kernel/include -c $<
%.o: kernel/driver/%.c
	gcc $(C_FLAGS) -o $@ -I./kernel/include -c $<
kernel.bin: kernel_entry.o $(C_OBJ)
	ld $(LD_FLAGS) -o kernel.bin -Ttext $(KERNEL_OFFSET) $^ --oformat binary

disk.img: bootsect.bin kernel.bin
	cat $^ > $@
	# dd if=bootsect.bin of=$@ bs=512 count=1 conv=notrunc
	# dd if=kernel.bin of=$@ bs=512 seek=1 conv=notrunc

run: all
	qemu-system-i386 disk.img
debug: all
	qemu-system-i386 disk.img -s -S
clean:
	rm *.bin *.o disk.img
