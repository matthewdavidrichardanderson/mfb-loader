/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#ifndef MFB_VIDEO_H
#define MFB_VIDEO_H

#include <gccore.h>

#include "mfb/config.h"

void mfb_video_init(void);
void mfb_video_apply_preview(const mfb_launch_config *config);
u16 mfb_video_framebuffer_width(void);
void *mfb_video_framebuffer(void);

#endif
