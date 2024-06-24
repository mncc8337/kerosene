BINS = bootsect.bin kernel.bin # order matter
SRCS = src/bootsect.asm src/kernel.asm

all: $(BINS)
	# dd if=bootsect.bin of=disk.img bs=512 count=1 conv=notrunc
	# dd if=kernel.bin of=disk.img bs=512 seek=1 conv=notrunc
	cat $(BINS) > disk.img
%.bin: src/%.asm
	nasm -f bin -o $@ -I./include $<
clean:
	rm $(BINS) disk.img
