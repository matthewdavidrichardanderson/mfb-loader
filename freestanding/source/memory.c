/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#include "mfb_memory.h"
void *mfb_memset(void *dst, int value, u32 length)
{ u8 *d = dst; while (length--) *d++ = (u8)value; return dst; }
void *mfb_memcpy(void *dst, const void *src, u32 length)
{ u8 *d = dst; const u8 *s = src; while (length--) *d++ = *s++; return dst; }
int mfb_memcmp(const void *a, const void *b, u32 length)
{ const u8 *x=a,*y=b; while(length--){ if(*x!=*y)return *x-*y; ++x;++y;} return 0; }
