/* Above the firmware's load window limit, and clear of its stack at run time */
MEMORY
{
    RAM (rwx) : ORIGIN = 0x20016000, LENGTH = 4K
}

ENTRY(Reset_Handler)

SECTIONS
{
    .text : {
        KEEP(*(.vectors))
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > RAM

    __stack_top = ORIGIN(RAM) + LENGTH(RAM);

    /DISCARD/ : { *(.ARM.*) *(.comment) *(.note*) *(.data*) *(.bss*) }
}
