; code copied from https://wiki.osdev.org/Detecting_Memory_(x86)#Getting_an_E820_Memory_Map
; because my brain is too small to actually know and implement this hell

; use the INT 0x15, eax= 0xE820 BIOS function to get a memory map
; note: initially di is 0, be sure to set it to a value so that the BIOS code will not be overwritten. 
;       The consequence of overwriting the BIOS code will lead to problems like getting stuck in `int 0x15`
; inputs: es:di -> destination buffer for 24 byte entries
; outputs: bp = entry count, trashes all registers except esi
e820_memmap:
	xor ebx, ebx ; EBX must be 0 to start
	xor bp, bp ; Keep an entry count in bp
	mov edx, dword 0x534D4150 ; Place "SMAP" into edx
	mov eax, 0xE820
	mov dword [es:di + 20], 1 ; Force a valid ACPI 3.X entry
	mov ecx, 24 ; Ask for 24 bytes
	int 0x15
	jc short .failed ; Carry set on first call means "unsupported function"
	mov edx, dword 0x0534D4150 ; Some BIOSes apparently trash this register
	cmp eax, edx ; On success, eax must have been reset to "SMAP"
	jne short .failed
	test ebx, ebx ; EBX=0 implies list is only 1 entry long (worthless)
	je short .failed
	jmp short .jmpin
.e820lp:
	mov eax, 0xE820 ; EAX, ECX get trashed on every int 0x15 call
	mov [es:di + 20], dword 1 ; Force a valid ACPI 3.X entry
	mov ecx, 24 ; Ask for 24 bytes again
	int 0x15
	jc short .e820f ; Carry set means "end of list already reached"
	mov edx, dword 0x0534D4150 ; Repair potentially trashed register
.jmpin:
	jcxz .skipent ; Skip any 0 length entries
	cmp cl, 20 ; Got a 24 byte ACPI 3.X response?
	jbe short .notext
	test byte [es:di + 20], 1 ; If so, is the "ignore this data" bit clear?
	je short .skipent
.notext:
	mov ecx, [es:di + 8] ; Get lower dword of memory region length
	or ecx, [es:di + 12] ; "or" it with upper dword to test for zero
	jz .skipent ; If length qword is 0, skip entry
	inc bp ; Got a good entry: ++count, move to next storage spot
	add di, 24
.skipent:
	test ebx, ebx ; If ebx is 0, list is complete
	jne short .e820lp
.e820f:
	clc ; Clear carry flag since there is no error
	ret
.failed:
	stc ; Set carry flag to indicate error
	ret
