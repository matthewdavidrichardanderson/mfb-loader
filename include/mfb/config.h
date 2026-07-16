/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#ifndef MFB_CONFIG_H
#define MFB_CONFIG_H

#include <gccore.h>

typedef enum {
    MFB_REGION_CONSOLE = 0,
    MFB_REGION_DISC
} mfb_region_policy;

typedef enum {
    MFB_SCALE_AUTO = 0,
    MFB_SCALE_1_TO_1,
    MFB_SCALE_640,
    MFB_SCALE_656,
    MFB_SCALE_672,
    MFB_SCALE_704,
    MFB_SCALE_720,
    MFB_SCALE_COUNT
} mfb_scale_mode;

typedef enum {
    MFB_FILTER_AUTO = 0,
    MFB_FILTER_OFF,
    MFB_FILTER_ON,
    MFB_FILTER_COUNT
} mfb_filter_mode;

typedef enum {
    MFB_VIDEO_AUTO = 0,
    MFB_VIDEO_480I60,
    MFB_VIDEO_576I50,
    MFB_VIDEO_480P60,
    MFB_VIDEO_COUNT
} mfb_video_mode;

typedef enum {
    MFB_ASPECT_AUTO = 0,
    MFB_ASPECT_4_3,
    MFB_ASPECT_16_9,
    MFB_ASPECT_COUNT
} mfb_aspect_mode;

typedef struct {
    mfb_scale_mode scale_mode;
    mfb_filter_mode filter_mode;
    mfb_video_mode video_mode;
    bool patch_480p;
    bool disable_dithering;
    mfb_region_policy region_policy;
    mfb_aspect_mode aspect_mode;
} mfb_launch_config;

mfb_launch_config mfb_config_defaults(void);
bool mfb_config_valid(const mfb_launch_config *config);
u16 mfb_config_preview_width(const mfb_launch_config *config, u16 framebuffer_width);
const char *mfb_scale_mode_string(mfb_scale_mode mode);
const char *mfb_filter_mode_string(mfb_filter_mode mode);
const char *mfb_video_mode_string(mfb_video_mode mode);
const char *mfb_aspect_mode_string(mfb_aspect_mode mode);

#endif
