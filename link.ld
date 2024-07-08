ENTRY (loader)

SECTIONS{
    .text :{
    text_start = .;
        *(.text)
    }

    .rodata ALIGN (0x1000) : {
        *(.rodata)
    }

.data ALIGN (0x1000) : {

   *(.data)
   end_data = .;
}


    .bss : {
        sbss = .;
        *(COMMON)
        *(.bss)
        ebss = .;
        endkernel = .;
    }


}
