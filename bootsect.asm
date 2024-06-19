org 0x7c00
bits 16

boot: jmp b_main
nop

bsOemName:            DB "S OS    "    ;OEM identifier (8 BYTES)
bpbBytesPerSector:    DW 0x0200        ;512 Bytes per logical sector (WORD)
bpbSectorsPerCluster: DB 0x01          ;1 Logical sector per cluster (BYTE)
bpbReservedSectors:   DW 0x0001        ;1 Reserved logical sector (WORD)
bpbNumberOfFATs:      DB 0x02          ;2 FATs on the storage media (BYTE)
bpbRootEntries:       DW 0x00E0        ;224 Root directory entries (WORD)
bpbTotalSectors:      DW 0x0B40        ;2880 Total logical sectors in volume
                                       ;(WORD)
bpbMedia:             DB 0xF0          ;Media Descriptor (BYTE): F0 = 1.44kB
                                       ;3.5-inch disk
bpbSectorsPerFAT:     DW 0x0009        ;9 Logical sectors per FAT (WORD)
bpbSectorsPerTrack:   DW 0x0012        ;18 Physical sectors per track (WORD)
bpbHeadsPerCylinder:  DW 0x0002        ;2 Heads (WORD)
bpbHiddenSectors:     DD 0x00000000    ;Hidden sectors (DWORD)
bpbTotalSectorsBig:   DD 0x00000000    ;Large total logical sectors (DWORD)
bsDriveNumber:        DB 0x00          ;Physical drive number (BYTE)
bsUnused:             DB 0x00          ;ExtFlags (BYTE)
bsExtBootSignature:   DB 0x29          ;Extended boot signature (BYTE). 0x29
                                       ;indicates that the EBPB has the
                                       ;following 3 entries:
bsSerialNumber:       DD 0x00010203    ;Volume serial number (DWORD)
bsVolumeLabel:        DB "S_OS       " ;Volume label (11 BYTES)
bsFileSystem:         DB "FAT12   "    ;File-system type (8 BYTES)

%include "print.asm"
bootloader_msg: db "bootloader started", ENDL, ENDS
floppy_reset_msg: db "floppy disk reset...", ENDL, ENDS
kernel_msg: db "kernel loaded", ENDL, ENDS

reset_floppy:
    mov ah,0x00                        ;Reset Disk Drives function code for
                                       ;INT 13
    mov dl,0x00                        ;Set 1st floppy drive as drive to reset
    int 0x13                           ;Call INT 13 to reset the drive
    jc reset_floppy                    ;Try again if carry flag (CF) is set
                                       ;(indicates an error)
    mov si, floppy_reset_msg           ;Move the address of msg into the
                                       ;SI register
    call print                         ;call the output subroutine
    ret                                ;return control to the caller

read_kernel:
    mov ax,0x1000
    mov es,ax                          ;Set the Extra Segment (ES) value to
                                       ;4096
    xor bx,bx                          ;Set the BX register to 0
                                       ;The kernel's address will be 1000:0000
    mov ah,0x02                        ;Read Sectors From Drive function code
                                       ;for INT 13
    mov al,0x01                        ;Sectors to read count
    mov ch,0x00                        ;Cylinder
    mov cl,0x02                        ;Sector to read
    mov dh,0x00                        ;Head number
    mov dl,0x00                        ;Drive number
    int 0x13                           ;BIOS Interrupt Call
    jc read_kernel                     ;Try again if carry flag (CF) is set
                                       ;(indicates an error)
    mov si, kernel_msg
    call print

    jmp 0x1000:0x0000 

b_main:
    mov si, bootloader_msg
    call print
    call reset_floppy
    call read_kernel

times 510-($-$$) db 0
dw 0xaa55
