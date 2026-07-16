/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#ifndef MFB_LAUNCH_H
#define MFB_LAUNCH_H

#include <gccore.h>
#include "mfb/config.h"

#define MFB_LAUNCH_PARAM_ADDR ((void *)0x90100000)
#define MFB_LAUNCHER_ADDR ((void *)0x80A80000)
#define MFB_LAUNCHER_STAGE_ADDR ((void *)0x90110000)
#define MFB_TRAMPOLINE_ADDR ((void *)0x93300000)
#define MFB_LAUNCH_MAGIC 0x4d46424c

typedef struct {
    u32 magic;
    u32 partition_offset;
    u8 requested_ios;
    u8 reserved[3];
    mfb_launch_config config;
    u32 diagnostic_xfb;
    u32 vi_vtrdcr;
    u32 vi_vto;
    u32 vi_vte;
} mfb_launch_params;

_Static_assert(sizeof(mfb_launch_config) == 24, "launch config ABI changed");
_Static_assert(__builtin_offsetof(mfb_launch_config, patch_480p) == 12,
               "480p flag ABI changed");
_Static_assert(__builtin_offsetof(mfb_launch_config, disable_dithering) == 13,
               "dithering flag ABI changed");

#endif
