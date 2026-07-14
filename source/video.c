/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#include <gccore.h>
#include <string.h>

#include "mfb/video.h"

static void *s_xfb;
static GXRModeObj s_mode;

void mfb_video_init(void)
{
    VIDEO_Init();
    const GXRModeObj *preferred = VIDEO_GetPreferredMode(NULL);
    memcpy(&s_mode, preferred, sizeof(s_mode));
    s_xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(&s_mode));
    /* Clear the complete XFB before the console begins drawing. */
    VIDEO_ClearFrameBuffer(&s_mode, s_xfb, COLOR_BLACK);
    console_init(s_xfb, 0, 0, s_mode.fbWidth, s_mode.xfbHeight,
                 s_mode.fbWidth * VI_DISPLAY_PIX_SZ);
    VIDEO_Configure(&s_mode);
    VIDEO_SetNextFramebuffer(s_xfb);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (s_mode.viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();
}

void mfb_video_apply_preview(const mfb_launch_config *config)
{
    const u16 width = mfb_config_preview_width(config, s_mode.fbWidth);
    s_mode.viWidth = width;
    s_mode.viXOrigin = (VI_MAX_WIDTH_NTSC - width) / 2;
    VIDEO_Configure(&s_mode);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

u16 mfb_video_framebuffer_width(void)
{
    return s_mode.fbWidth;
}

void *mfb_video_framebuffer(void)
{
    return s_xfb;
}
