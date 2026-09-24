MEMORY {
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1M
    SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}
ENTRY(_reset)
SECTIONS {
    .text : { KEEP(*(.isr_vector)) *(.text*) *(.rodata*) } > FLASH
    .data : { __data_start__ = .; *(.data*) __data_end__ = .; } > SRAM AT > FLASH
    .bss : { __bss_start__ = .; *(.bss*) *(COMMON) __bss_end__ = .; } > SRAM
    _end = .; end = .; __end__ = .;
    __heap_end = ORIGIN(SRAM) + LENGTH(SRAM) - 0x4000;
}
