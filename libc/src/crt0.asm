extern main
extern syscall_kill ; sys/syscall/syscall_kill.c
                    ; void syscall_kill(int exit_code);

argument_count: dw 0
global argument_count

argument_vector: dd 0
global argument_vector

environment_count: dw 0
global environment_count

environment_pointer: dd 0
global environment_pointer

global _start
_start:
    ; upon entering _start, eax holds argc, ebx holds argv, ecx holds envc and edx holds envp
    mov [argument_count], eax
    mov [argument_vector], ebx
    mov [environment_count], ecx
    mov [environment_pointer], edx

    push edx ; envp
    push ebx ; argv
    push eax ; argc
    call main
    add esp, 12
    ; now eax contains the exit code

    push eax;
    call syscall_kill
    add esp, 4

.loop:
    pause
    jmp .loop
