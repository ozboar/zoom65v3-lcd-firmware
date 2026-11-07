/* Loaded by the ROM bootloader, which keeps the low part of SRAM for itself */
MEMORY
{
    RAM (rwx) : ORIGIN = 0x20008000, LENGTH = 64K
    MEDIA (rw) : ORIGIN = 0x20018000, LENGTH = 112K
}

ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .media_ram (NOLOAD) : {
        *(.media_ram*)
    } > MEDIA

    .isr_vector : {
        KEEP(*(.isr_vector))
    } > RAM

    .text : {
        *(.text*)
        *(.rodata*)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
    } > RAM

    .ARM.exidx : {
        *(.ARM.exidx*)
    } > RAM

    .init_array : {
        KEEP(*(.preinit_array*))
        KEEP(*(.init_array*))
        KEEP(*(.fini_array*))
    } > RAM

    /* Already in place once loaded, so the startup copy is a no-op */
    .data : {
        . = ALIGN(4);
        _sidata = .;
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    .bss (NOLOAD) : {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
        end = .;
    } > RAM

    /DISCARD/ : { *(.ARM.attributes) *(.comment) *(.note*) }
}
