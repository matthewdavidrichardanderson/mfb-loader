/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#ifndef MFB_DI_H
#define MFB_DI_H
#include "mfb_types.h"
s32 mfb_di_open(void);
s32 mfb_di_close(void);
s32 mfb_di_reset(void);
s32 mfb_di_identify(void);
s32 mfb_di_read_id(void *output);
s32 mfb_di_open_partition(u32 offset);
s32 mfb_di_read(void *output, u32 length, u32 offset);
#endif
