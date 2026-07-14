/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#ifndef MFB_PATCH_H
#define MFB_PATCH_H
#include "mfb_types.h"

typedef struct {
    u32 sections;
    u32 modes_seen;
    u32 video_modes_changed;
    u32 widths_changed;
    u32 runtime_width_changed;
    u32 filters_changed;
    u32 gx_filter_functions_changed;
    u32 aspect_functions_changed;
    u32 pixel_480p_changed;
} mfb_patch_report;

void mfb_patch_init(mfb_patch_report *report);
void mfb_patch_section(mfb_patch_report *report, const u32 config[6],
                       void *address, u32 length);
void mfb_patch_finalize(mfb_patch_report *report, const u32 config[6]);
#endif
