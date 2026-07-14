/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#include "mfb/config.h"

mfb_launch_config mfb_config_defaults(void)
{
    const mfb_launch_config config = {
        .scale_mode = MFB_SCALE_AUTO,
        .filter_mode = MFB_FILTER_AUTO,
        .video_mode = MFB_VIDEO_AUTO,
        .patch_480p = false,
        .region_policy = MFB_REGION_CONSOLE,
        .aspect_mode = MFB_ASPECT_AUTO,
    };
    return config;
}

bool mfb_config_valid(const mfb_launch_config *config)
{
    return config != NULL && config->scale_mode < MFB_SCALE_COUNT &&
           config->filter_mode < MFB_FILTER_COUNT &&
           config->video_mode < MFB_VIDEO_COUNT &&
           config->region_policy <= MFB_REGION_DISC &&
           config->aspect_mode < MFB_ASPECT_COUNT;
}

const char *mfb_video_mode_string(mfb_video_mode mode)
{
    static const char *const names[MFB_VIDEO_COUNT] = {
        "Auto (disc/console)", "480i / 60 Hz", "576i / 50 Hz", "480p / 60 Hz"
    };
    return mode < MFB_VIDEO_COUNT ? names[mode] : "Invalid";
}

u16 mfb_config_preview_width(const mfb_launch_config *config, u16 framebuffer_width)
{
    static const u16 widths[MFB_SCALE_COUNT] = {0, 0, 640, 656, 672, 704, 720};
    if (!mfb_config_valid(config))
        return framebuffer_width;
    if (config->scale_mode == MFB_SCALE_AUTO || config->scale_mode == MFB_SCALE_1_TO_1)
        return framebuffer_width;
    return widths[config->scale_mode];
}

const char *mfb_scale_mode_string(mfb_scale_mode mode)
{
    static const char *const names[MFB_SCALE_COUNT] = {
        "Auto", "1:1 (Framebuffer)", "640 px", "656 px", "672 px", "704 px", "720 px"
    };
    return mode < MFB_SCALE_COUNT ? names[mode] : "Invalid";
}

const char *mfb_filter_mode_string(mfb_filter_mode mode)
{
    static const char *const names[MFB_FILTER_COUNT] = {
        "Auto", "Off", "On"
    };
    return mode < MFB_FILTER_COUNT ? names[mode] : "Invalid";
}

const char *mfb_aspect_mode_string(mfb_aspect_mode mode)
{
    static const char *const names[MFB_ASPECT_COUNT] = {
        "Auto (console)", "4:3", "16:9"
    };
    return mode < MFB_ASPECT_COUNT ? names[mode] : "Invalid";
}
