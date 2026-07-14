/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#ifndef MFB_PATCH_H
#define MFB_PATCH_H

#include <gccore.h>

#include "mfb/config.h"

typedef enum {
    MFB_PATCH_NOT_REQUESTED = 0,
    MFB_PATCH_APPLIED,
    MFB_PATCH_NOT_APPLICABLE,
    MFB_PATCH_FAILED
} mfb_patch_state;

typedef struct {
    mfb_patch_state vi_scale;
    mfb_patch_state video_mode;
    mfb_patch_state deflicker;
    mfb_patch_state pixel_480p;
    mfb_patch_state region_free;
    u32 sections_seen;
    u32 video_modes_changed;
    u32 filters_changed;
} mfb_patch_report;

void mfb_patch_begin(mfb_patch_report *report, const mfb_launch_config *config);
void mfb_patch_section(mfb_patch_report *report, const mfb_launch_config *config,
                       void *address, u32 length);
bool mfb_patch_can_launch(const mfb_patch_report *report);
const char *mfb_patch_state_string(mfb_patch_state state);

#endif
