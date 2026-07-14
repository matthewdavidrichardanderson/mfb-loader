/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

OUTPUT_FORMAT("elf32-powerpc")
OUTPUT_ARCH(powerpc:common)
ENTRY(_start)
SECTIONS {
    . = 0x80F00000;
    .text : { *(.start) *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }
    . = ALIGN(32);
    __bss_start = .;
    .bss : { *(.bss .bss.* COMMON) }
    __bss_end = .;
    . = ALIGN(32);
    __stack_bottom = .;
    . += 0x10000;
    __stack_top = .;
}
