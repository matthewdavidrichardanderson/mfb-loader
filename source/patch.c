/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#include <gccore.h>
#include <string.h>

#include "mfb/patch.h"

typedef struct {
    u32 vi_tv_mode;
    u16 fb_width;
    u16 efb_height;
    u16 xfb_height;
    u16 vi_x_origin;
    u16 vi_y_origin;
    u16 vi_width;
    u16 vi_height;
    u32 xfb_mode;
    u8 field_rendering;
    u8 aa;
    u8 sample_pattern[12][2];
    u8 vfilter[7];
} mfb_rvl_mode;

static const u8 s_sample_pattern[24] = {
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
};
static const u8 s_sample_pattern_aa[24] = {
    3, 2, 9, 6, 3, 10, 3, 2, 9, 6, 3, 10,
    9, 2, 3, 6, 9, 10, 9, 2, 3, 6, 9, 10,
};
static const u8 s_filter_off[7] = {0, 0, 21, 22, 21, 0, 0};
static const u8 s_filter_on[7] = {4, 8, 12, 16, 12, 8, 4};

static u32 instruction_rt(u32 word) { return (word >> 21) & 31u; }
static u32 instruction_ra(u32 word) { return (word >> 16) & 31u; }

static bool is_gx_set_dither(const u32 *code)
{
    const u32 bp = instruction_rt(code[0]);
    const u32 token = instruction_rt(code[1]);
    const u32 state = instruction_rt(code[3]);
    const u32 gx = instruction_ra(code[3]);
    const u16 state_offset = (u16)code[3];
    return (code[0] & 0xfc1fffffu) == 0x3c00cc01u &&
           (code[1] & 0xfc1fffffu) == 0x38000061u &&
           code[2] == 0x38000000u &&
           (code[3] & 0xfc000000u) == 0x80000000u &&
           (code[4] & 0xffe0ffffu) == 0x5060177au &&
           instruction_ra(code[4]) == state &&
           (code[5] & 0xfc00ffffu) == 0x98008000u &&
           instruction_rt(code[5]) == token && instruction_ra(code[5]) == bp &&
           (code[6] & 0xfc00ffffu) == 0x90008000u &&
           instruction_rt(code[6]) == state && instruction_ra(code[6]) == bp &&
           (code[7] & 0xfc000000u) == 0x90000000u &&
           instruction_rt(code[7]) == state && instruction_ra(code[7]) == gx &&
           (u16)code[7] == state_offset &&
           (code[8] & 0xfc00ffffu) == 0xb0000002u &&
           instruction_rt(code[8]) == 0 && instruction_ra(code[8]) == gx &&
           code[9] == 0x4e800020u;
}

static bool looks_like_mode(const mfb_rvl_mode *mode)
{
    return (mode->fb_width == 512 || mode->fb_width == 608 || mode->fb_width == 640) &&
           mode->vi_width >= 512 && mode->vi_width <= 720 &&
           mode->vi_height >= 240 && mode->vi_height <= 1200 &&
           mode->field_rendering <= 1 && mode->aa <= 1 &&
           (memcmp(mode->sample_pattern, s_sample_pattern, sizeof(s_sample_pattern)) == 0 ||
            memcmp(mode->sample_pattern, s_sample_pattern_aa, sizeof(s_sample_pattern_aa)) == 0);
}

static u16 requested_width(const mfb_launch_config *config, u16 framebuffer_width)
{
    if (config->scale_mode == MFB_SCALE_1_TO_1)
        return framebuffer_width;
    return mfb_config_preview_width(config, framebuffer_width);
}

static void force_video_mode(mfb_rvl_mode *mode, mfb_video_mode video_mode)
{
    if (video_mode == MFB_VIDEO_480I60) {
        mode->vi_tv_mode = VI_TVMODE_NTSC_INT;
        mode->efb_height = 480;
        mode->xfb_height = 480;
        mode->vi_y_origin = 0;
        mode->vi_height = 480;
        mode->xfb_mode = VI_XFBMODE_DF;
        mode->field_rendering = GX_FALSE;
    } else if (video_mode == MFB_VIDEO_576I50) {
        mode->vi_tv_mode = VI_TVMODE_PAL_INT;
        mode->efb_height = 528;
        mode->xfb_height = 528;
        mode->vi_y_origin = 23;
        mode->vi_height = 528;
        mode->xfb_mode = VI_XFBMODE_DF;
        mode->field_rendering = GX_FALSE;
    } else if (video_mode == MFB_VIDEO_480P60) {
        /* Keep the game's render and display-copy geometry intact. */
        mode->vi_tv_mode = VI_TVMODE_NTSC_PROG;
        mode->xfb_mode = VI_XFBMODE_SF;
        mode->field_rendering = GX_FALSE;
    }
}

void mfb_patch_begin(mfb_patch_report *report, const mfb_launch_config *config)
{
    memset(report, 0, sizeof(*report));
    report->vi_scale = config->scale_mode == MFB_SCALE_AUTO ?
                       MFB_PATCH_NOT_REQUESTED : MFB_PATCH_FAILED;
    report->video_mode = config->video_mode == MFB_VIDEO_AUTO ?
                         MFB_PATCH_NOT_REQUESTED : MFB_PATCH_FAILED;
    report->deflicker = config->filter_mode == MFB_FILTER_AUTO ?
                       MFB_PATCH_NOT_REQUESTED : MFB_PATCH_FAILED;
    report->dithering = config->disable_dithering ?
                        MFB_PATCH_FAILED : MFB_PATCH_NOT_REQUESTED;
    report->pixel_480p = config->patch_480p ? MFB_PATCH_FAILED : MFB_PATCH_NOT_REQUESTED;
    report->region_free = config->region_policy == MFB_REGION_DISC ?
                          MFB_PATCH_FAILED : MFB_PATCH_NOT_REQUESTED;
}

void mfb_patch_section(mfb_patch_report *report, const mfb_launch_config *config,
                       void *address, u32 length)
{
    ++report->sections_seen;
    const u32 section_length = length;
    u8 *cursor = address;
    while (length >= sizeof(mfb_rvl_mode)) {
        mfb_rvl_mode *mode = (mfb_rvl_mode *)cursor;
        if (looks_like_mode(mode)) {
            if (config->video_mode != MFB_VIDEO_AUTO) {
                force_video_mode(mode, config->video_mode);
                ++report->video_modes_changed;
                report->video_mode = MFB_PATCH_APPLIED;
            }
            if (config->scale_mode != MFB_SCALE_AUTO) {
                const u16 width = requested_width(config, mode->fb_width);
                mode->vi_width = width;
                mode->vi_x_origin = (VI_MAX_WIDTH_NTSC - width) / 2;
                ++report->video_modes_changed;
                report->vi_scale = MFB_PATCH_APPLIED;
            }
            if (config->filter_mode != MFB_FILTER_AUTO) {
                const u8 *filter = config->filter_mode == MFB_FILTER_OFF ?
                                   s_filter_off : s_filter_on;
                memcpy(mode->vfilter, filter, sizeof(mode->vfilter));
                ++report->filters_changed;
                report->deflicker = MFB_PATCH_APPLIED;
            }
            cursor += sizeof(mfb_rvl_mode);
            length -= sizeof(mfb_rvl_mode);
            continue;
        }
        cursor += 4;
        length -= 4;
    }

    /* GXSetDither updates bit 2 of the PE colour-mode register from r3.
     * Use the function's known zero register as the source instead, so every
     * request writes a disabled dither bit while preserving its other state. */
    if (config->disable_dithering && section_length >= 40u) {
        u8 *const data = address;
        for (u32 offset = 0; offset + 40u <= section_length; offset += 4) {
            u32 *const code = (u32 *)(data + offset);
            if (is_gx_set_dither(code)) {
                code[4] &= ~(31u << 21);
                ++report->dither_functions_changed;
                report->dithering = MFB_PATCH_APPLIED;
            }
        }
    }
}

bool mfb_patch_can_launch(const mfb_patch_report *report)
{
    return report->vi_scale != MFB_PATCH_FAILED &&
           report->video_mode != MFB_PATCH_FAILED &&
           report->deflicker != MFB_PATCH_FAILED &&
           report->dithering != MFB_PATCH_FAILED &&
           report->pixel_480p != MFB_PATCH_FAILED &&
           report->region_free != MFB_PATCH_FAILED;
}

const char *mfb_patch_state_string(mfb_patch_state state)
{
    static const char *const names[] = {"not requested", "applied", "not applicable", "failed"};
    return state <= MFB_PATCH_FAILED ? names[state] : "invalid";
}
