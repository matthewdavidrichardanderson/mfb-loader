/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#ifndef MFB_CACHE_H
#define MFB_CACHE_H
#include "mfb_types.h"
void mfb_dc_flush(const void *address, u32 length);
void mfb_dc_invalidate(void *address, u32 length);
void mfb_ic_invalidate(void *address, u32 length);
#endif
