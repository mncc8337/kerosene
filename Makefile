BINS = bootsect.bin kernel.bin # order matter
SRCS = $(addsuffix .asm, $(basename $(BINS)))

all: $(BINS)
	# dd if=bootsect.bin of=disk.img bs=512 count=1 conv=notrunc
	# dd if=kernel.bin of=disk.img bs=512 seek=1 conv=notrunc
	cat $(BINS) > disk.img
$(BINS): $(SRCS)
	nasm -f bin -o $@ $<
clean:
	rm $(BINS) disk.img
