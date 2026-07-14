/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#include "mfb_cache.h"
static u32 floor32(u32 value) { return value & ~31u; }
void mfb_dc_flush(const void *address, u32 length)
{
    u32 cursor = floor32((u32)address), end = (u32)address + length;
    for (; cursor < end; cursor += 32) __asm__ volatile("dcbf 0,%0" : : "r"(cursor));
    __asm__ volatile("sync");
}
void mfb_dc_invalidate(void *address, u32 length)
{
    u32 cursor = floor32((u32)address), end = (u32)address + length;
    for (; cursor < end; cursor += 32) __asm__ volatile("dcbi 0,%0" : : "r"(cursor));
    __asm__ volatile("sync");
}
void mfb_ic_invalidate(void *address, u32 length)
{
    u32 cursor = floor32((u32)address), end = (u32)address + length;
    for (; cursor < end; cursor += 32) __asm__ volatile("icbi 0,%0" : : "r"(cursor));
    __asm__ volatile("sync; isync");
}
