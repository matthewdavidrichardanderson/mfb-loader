/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

    .section .start,"ax"
    .global _start
    .extern __bss_start
    .extern __bss_end
    .extern __stack_top
    .extern mfb_payload_main
_start:
    mfmsr 3
    rlwinm 3,3,0,17,15
    mtmsr 3
    isync
    li 0,0
    lis 1,__stack_top@h
    ori 1,1,__stack_top@l
    addi 1,1,-16
    lis 2,0
    lis 13,0
    lis 3,__bss_start@h
    ori 3,3,__bss_start@l
    lis 4,__bss_end@h
    ori 4,4,__bss_end@l
0:
    cmplw 3,4
    bge 1f
    stw 0,0(3)
    addi 3,3,4
    b 0b
1:
    sync
    bl mfb_payload_main
2:
    b 2b
