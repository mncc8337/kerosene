[bits 32]

global tss_flush
tss_flush:
    mov ax, (5 * 8) | 3
    ltr ax
    ret
