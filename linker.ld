SECTIONS {
    .text ALIGN (0x00) : {
        *(.text)
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame_hdr)
        *(.eh_frame)
        *(.note.gnu.property)
    }
}
