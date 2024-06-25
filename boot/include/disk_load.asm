DISK_ERROR_MSG: db "disk read error, system hang", 0
disk_load:
    push dx          ; store dx on stack so later we can recall how many sectors were request to be read
    mov ah, 0x2      ; BIOS read sector function
    mov al, dh       ; read dh sectors
    mov ch, 0x0      ; select cylinder 0
    mov dh, 0x0      ; select head 0
    mov cl, 0x2      ; start reading from second sector (after the boot sector)
    int 0x13         ; BIOS interrupt
    jc .disk_error   ; jump if error (carry flag set)

    pop dx           ; restore dx from the stack
    cmp dh, al       ; if al (sectors read) != dh (sectors expected)
    jne .disk_error  ; display error message
    ret
.disk_error:
    mov si, DISK_ERROR_MSG
    call print_string
    jmp $
