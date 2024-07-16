[bits 32]

global load_idt
extern idtr
load_idt:
    lidt [idtr]
    ret
