OUTPUT_FORMAT("elf32-m68k")
ENTRY(_start)
SECTIONS
{
    . = 0;
    .text : { 
        *(.text.start) 
        *(.text .text.*) 
        *(.rodata .rodata.*) 
        . = ALIGN(4); 
    }
    .data : { 
        *(.data .data.*) 
        . = ALIGN(4); 
    }
    .bss  : { 
        . = ALIGN(4); 
        __bss_start = .; 
        *(.bss .bss.* COMMON) 
        . = ALIGN(4); 
        __bss_end = .; 
    }
    
    _end = .;
    
    /DISCARD/ : { 
        *(.comment) 
        *(.note*) 
        *(.eh_frame*) 
    }
}