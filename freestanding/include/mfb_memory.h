/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#ifndef MFB_MEMORY_H
#define MFB_MEMORY_H
#include "mfb_types.h"
void *mfb_memset(void *dst, int value, u32 length);
void *mfb_memcpy(void *dst, const void *src, u32 length);
int mfb_memcmp(const void *a, const void *b, u32 length);
#endif
