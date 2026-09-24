EXTERN(vector_table)
ENTRY(reset_handler)
MEMORY
{
 ram (rwx) : ORIGIN = 0x20000000, LENGTH = 112K
 ram2 (rwx) : ORIGIN = 0x2001c000, LENGTH = 16K
 rom (rx) : ORIGIN = 0x08000000, LENGTH = 1024K
 ccm (rwx) : ORIGIN = 0x10000000, LENGTH = 64K
}
SECTIONS
{
 .text : {
  *(.vectors)
  *(.text*)
  . = ALIGN(4);
  *(.rodata*)
  . = ALIGN(4);
 } >rom
 .preinit_array : {
  . = ALIGN(4);
  __preinit_array_start = .;
  KEEP (*(.preinit_array))
  __preinit_array_end = .;
 } >rom
 .init_array : {
  . = ALIGN(4);
  __init_array_start = .;
  KEEP (*(SORT(.init_array.*)))
  KEEP (*(.init_array))
  __init_array_end = .;
 } >rom
 .fini_array : {
  . = ALIGN(4);
  __fini_array_start = .;
  KEEP (*(.fini_array))
  KEEP (*(SORT(.fini_array.*)))
  __fini_array_end = .;
 } >rom
 .ARM.extab : {
  *(.ARM.extab*)
 } >rom
 .ARM.exidx : {
  __exidx_start = .;
  *(.ARM.exidx*)
  __exidx_end = .;
 } >rom
 . = ALIGN(4);
 _etext = .;
 .data : {
  _data = .;
  *(.data*)
  . = ALIGN(4);
  _edata = .;
 } >ram AT >rom
 _data_loadaddr = LOADADDR(.data);
 /* afs-kex-512's reference call tree needs ~80K of contiguous stack, which
    only the 128K main SRAM (SRAM1+SRAM2, physically contiguous) can provide.
    To make room there, the largest static crypto_kex test buffers (the two
    secret keys and the four pass messages, ~49K total) are relocated to the
    64K CCM. This MUST come before the generic `.bss` below so the named
    `.bss.<buf>` input sections (emitted thanks to -fdata-sections) land here
    instead of being swallowed by `*(.bss*)`. The `_b` suffix matches test.c's
    DEF() macro; the plain names match speed/stack/profiling/testvectors.c.
    These buffers are always fully written before being read, so the fact that
    the reset handler only zero-inits the main-SRAM .bss (not CCM) is safe. */
 .ccm (NOLOAD) : {
  *(.ccmram*)
  *(.bss.ska_b .bss.skb_b .bss.m1_b .bss.m2_b .bss.m3_b .bss.m4_b)
  *(.bss.ska .bss.skb .bss.m1 .bss.m2 .bss.m3 .bss.m4)
  . = ALIGN(8);
 } >ccm
 .bss : {
  *(.bss*)
  *(COMMON)
  . = ALIGN(4);
  _ebss = .;
  end = .;
 } >ram
 .ram2 : {
  *(.ram2*)
  . = ALIGN(4);
 } >ram2
 /DISCARD/ : { *(.eh_frame) }
 . = ALIGN(4);
}
/* Stack top = top of the contiguous SRAM1+SRAM2 block (0x20020000); it grows
   down through the ram2/ram boundary (physically continuous) toward the heap. */
PROVIDE(_stack = ORIGIN(ram2) + LENGTH(ram2));
