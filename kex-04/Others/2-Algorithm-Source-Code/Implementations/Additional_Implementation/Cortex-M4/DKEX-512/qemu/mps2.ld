/* Linker script for QEMU -M mps2-an386 (Cortex-M4). Code in ZBT SSRAM1 @0x0,
 * data/bss/stack in ZBT SSRAM2/3 @0x20000000 (4MB) — large enough for the
 * reference ML-DSA-87 sign stack, which does not fit STM32F405's 128KB. */
MEMORY {
    CODE (rx)  : ORIGIN = 0x00000000, LENGTH = 4M
    SRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 4M
}
ENTRY(_reset)
SECTIONS {
    .text : { KEEP(*(.isr_vector)) *(.text*) *(.rodata*) } > CODE
    .data : { __data_start__ = .; *(.data*) __data_end__ = .; } > SRAM AT > CODE
    .bss : { __bss_start__ = .; *(.bss*) *(COMMON) __bss_end__ = .; } > SRAM
    _end = .; end = .; __end__ = .;
    __heap_end = ORIGIN(SRAM) + LENGTH(SRAM) - 0x200000;
}
