extern main
extern syscall_kill ; sys/syscall/syscall_kill.c
                    ; void syscall_kill(int exit_code);

argc: dd 0

global _start
_start:
    ; upon entering _start, ecx holds argc and esi holds argv
    push esi
    push ecx
    call main
    add esp, 8
    ; now eax contains the exit code

    push eax;
    call syscall_kill
    add esp, 4

.loop:
    pause
    jmp .loop
