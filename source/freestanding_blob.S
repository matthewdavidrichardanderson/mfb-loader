/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

    .section .rodata
    .balign 32
    .global mfb_freestanding_bin
mfb_freestanding_bin:
    .incbin "../freestanding/mfb-freestanding.bin"
    .global mfb_freestanding_bin_end
mfb_freestanding_bin_end:
