[bits 32]

;global enter_usermode
;extern user_function
;enter_usermode:
;    mov ax, (4 * 8) | 3
;    mov ds, ax
;    mov es, ax
;    mov fs, ax
;    mov gs, ax
;
;    push (4 * 8) | 3    ; SS
;    push esp            ; ESP
;    pushf               ; EFLAGS
;    push (3 * 8) | 3    ; CS
;    push user_function  ; function to return to
;
;    ; using IRET to switch to usermode
;    iret
