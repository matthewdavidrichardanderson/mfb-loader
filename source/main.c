/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>

#include "mfb/config.h"
#include "mfb/disc.h"
#include "mfb/log.h"
#include "mfb/settings.h"
#include "mfb/video.h"

enum { ROW_VIDEO, ROW_ASPECT, ROW_SCALE, ROW_FILTER, ROW_DITHER, ROW_480P, ROW_REGION, ROW_LAUNCH, ROW_COUNT };

static void draw_menu(const mfb_launch_config *config, int row,
                      mfb_disc_status disc_status, const char disc_id[7])
{
    printf("\x1b[2J\x1b[H\n\n");
    puts("  MFB Loader\n");
    printf("  %c Video output     : %s\n", row == ROW_VIDEO ? '>' : ' ',
           mfb_video_mode_string(config->video_mode));
    printf("  %c Aspect ratio     : %s\n", row == ROW_ASPECT ? '>' : ' ',
           mfb_aspect_mode_string(config->aspect_mode));
    printf("  %c Horizontal scale : %s\n", row == ROW_SCALE ? '>' : ' ',
           mfb_scale_mode_string(config->scale_mode));
    printf("  %c Deflicker        : %s\n", row == ROW_FILTER ? '>' : ' ',
           mfb_filter_mode_string(config->filter_mode));
    printf("  %c Dithering        : %s\n", row == ROW_DITHER ? '>' : ' ',
           config->disable_dithering ? "Off" : "Auto");
    printf("  %c 480p pixel patch : %s\n", row == ROW_480P ? '>' : ' ',
           config->patch_480p ? "On" : "Off");
    printf("  %c Region policy    : %s\n\n", row == ROW_REGION ? '>' : ' ',
           config->region_policy == MFB_REGION_DISC ? "Region free" : "Console default");
    printf("  %c Launch disc\n\n", row == ROW_LAUNCH ? '>' : ' ');
    printf("  Drive: %s", mfb_disc_status_string(disc_status));
    if (disc_id[0] != '\0')
        printf(" [%s]", disc_id);
    puts("\n\n  D-PAD select/change  A next  PLUS/Z diagnostics  B back  HOME/START exit");
}

static void draw_diagnostics(mfb_disc_status status)
{
    const mfb_disc_diagnostics *diag = mfb_disc_get_diagnostics();
    printf("\x1b[2J\x1b[H\n\n");
    puts("  mfb-loader diagnostics\n");
    puts("  SDK: pinned libogc2 c4a4f22\n");
    printf("  IOS: %ld revision %ld\n", (long)diag->ios_version, (long)diag->ios_revision);
    printf("  DI_Init: %ld   DI status: %02lx   cover call: %ld   cover register: %08lx\n",
           (long)diag->di_init_result, (unsigned long)diag->di_status,
           (long)diag->cover_result, (unsigned long)diag->cover_register);
    printf("  Disc ID call: %ld   drive error: %08lx\n",
           (long)diag->disc_id_result, (unsigned long)diag->drive_error);
    printf("  Reset: %ld   Identify: %ld\n",
           (long)diag->reset_result, (long)diag->identify_result);
    printf("  Inspect: %ld   partitions: %lu   game offset: %08lx   requested IOS: %u\n",
           (long)diag->inspect_result, (unsigned long)diag->partition_count,
           (unsigned long)diag->game_partition_offset, diag->requested_ios);
    printf("  Reload: %ld   now IOS: %ld   /dev/di fd: %ld   encrypted read: %ld\n",
           (long)diag->reload_result, (long)diag->post_reload_ios,
           (long)diag->post_reload_di_init, (long)diag->encrypted_read_result);
    printf("  State: %s\n", mfb_disc_status_string(status));
    printf("  Video: framebuffer %u px   USB Gecko: %s\n\n",
           mfb_video_framebuffer_width(), mfb_log_usb_gecko() ? "connected" : "not detected");
    puts("  Latest events:");
    const size_t count = mfb_log_count() < 12 ? mfb_log_count() : 12;
    for (size_t i = count; i > 0; --i) {
        const char *line = mfb_log_get(i - 1);
        if (line != NULL)
            printf("  %s\n", line);
    }
    puts("\n  PLUS/Z/B return   HOME/START exit");
}

static void change_setting(mfb_launch_config *config, int row, int direction)
{
    switch (row) {
    case ROW_VIDEO:
        config->video_mode = (config->video_mode + MFB_VIDEO_COUNT + direction) % MFB_VIDEO_COUNT;
        break;
    case ROW_ASPECT:
        config->aspect_mode = (config->aspect_mode + MFB_ASPECT_COUNT + direction) % MFB_ASPECT_COUNT;
        break;
    case ROW_SCALE:
        config->scale_mode = (config->scale_mode + MFB_SCALE_COUNT + direction) % MFB_SCALE_COUNT;
        mfb_video_apply_preview(config);
        break;
    case ROW_FILTER:
        config->filter_mode = (config->filter_mode + MFB_FILTER_COUNT + direction) % MFB_FILTER_COUNT;
        break;
    case ROW_DITHER:
        config->disable_dithering = !config->disable_dithering;
        break;
    case ROW_480P:
        config->patch_480p = !config->patch_480p;
        break;
    case ROW_REGION:
        config->region_policy = config->region_policy == MFB_REGION_CONSOLE ?
                                MFB_REGION_DISC : MFB_REGION_CONSOLE;
        break;
    case ROW_LAUNCH:
        break;
    }
    mfb_logf("Settings: video=%s aspect=%s scale=%s deflicker=%s dithering=%s 480p=%s region=%s",
             mfb_video_mode_string(config->video_mode),
             mfb_aspect_mode_string(config->aspect_mode),
             mfb_scale_mode_string(config->scale_mode),
             mfb_filter_mode_string(config->filter_mode),
             config->disable_dithering ? "off" : "auto",
             config->patch_480p ? "on" : "off",
             config->region_policy == MFB_REGION_DISC ? "free" : "console");
    if (!mfb_settings_save(config))
        mfb_logf("Settings save failed");
}

int main(int argc, char **argv)
{
    mfb_video_init();
    WPAD_Init();
    PAD_Init();
    mfb_log_init();
    mfb_logf("MFB started");
    mfb_launch_config config = mfb_config_defaults();
    const bool settings_loaded = mfb_settings_init(&config, argc > 0 ? argv[0] : NULL);
    mfb_logf("Settings: %s", settings_loaded ? "loaded" : "defaults");
    mfb_video_apply_preview(&config);
    int row = 0;
    u32 cover_status = 0;
    mfb_disc_status disc_status = mfb_disc_probe(&cover_status);
    mfb_logf("Initial disc state: %s", mfb_disc_status_string(disc_status));
    char disc_id[7] = {0};
    draw_menu(&config, row, disc_status, disc_id);

    unsigned frame = 0;
    bool diagnostics = false;
    while (true) {
        WPAD_ScanPads();
        PAD_ScanPads();
        const u32 wpad_down = WPAD_ButtonsDown(0);
        const u16 pad_down = PAD_ButtonsDown(0);
        if ((wpad_down & WPAD_BUTTON_HOME) || (pad_down & PAD_BUTTON_START))
            break;

        if ((wpad_down & WPAD_BUTTON_PLUS) || (pad_down & PAD_TRIGGER_Z) ||
            (diagnostics && (pad_down & PAD_BUTTON_B))) {
            diagnostics = !diagnostics;
            if (diagnostics)
                draw_diagnostics(disc_status);
            else
                draw_menu(&config, row, disc_status, disc_id);
            VIDEO_WaitVSync();
            continue;
        }

        if (diagnostics) {
            if (++frame % 60 == 0) {
                const mfb_disc_status next = mfb_disc_probe(&cover_status);
                if (next != disc_status) {
                    disc_status = next;
                    disc_id[0] = '\0';
                    mfb_logf("Disc state: %s (cover=%08lx, call=%ld)",
                             mfb_disc_status_string(disc_status),
                             (unsigned long)mfb_disc_get_diagnostics()->cover_register,
                             (long)mfb_disc_get_diagnostics()->cover_result);
                    draw_diagnostics(disc_status);
                }
            }
            VIDEO_WaitVSync();
            continue;
        }

        bool redraw = false;
        if ((wpad_down & WPAD_BUTTON_UP) || (pad_down & PAD_BUTTON_UP)) {
            row = (row + ROW_COUNT - 1) % ROW_COUNT;
            redraw = true;
        } else if ((wpad_down & WPAD_BUTTON_DOWN) || (pad_down & PAD_BUTTON_DOWN)) {
            row = (row + 1) % ROW_COUNT;
            redraw = true;
        }
        if ((wpad_down & WPAD_BUTTON_LEFT) || (pad_down & PAD_BUTTON_LEFT)) {
            change_setting(&config, row, -1);
            redraw = true;
        } else if ((wpad_down & (WPAD_BUTTON_RIGHT | WPAD_BUTTON_A)) ||
                   (pad_down & (PAD_BUTTON_RIGHT | PAD_BUTTON_A))) {
            if (row == ROW_LAUNCH) {
                if (mfb_disc_inspect() == 0) {
                    mfb_disc_read_id(disc_id);
                    printf("\x1b[2J\x1b[H\n\n  Launching %s with stock IOS%u...\n", disc_id,
                           mfb_disc_get_diagnostics()->requested_ios);
                    VIDEO_Flush();
                    VIDEO_WaitVSync();
                    WPAD_Shutdown();
                    const s32 launch_result = mfb_disc_launch(&config);
                    WPAD_Init();
                    PAD_Init();
                    mfb_logf("Launch handoff returned %ld", (long)launch_result);
                }
            } else {
                change_setting(&config, row, 1);
            }
            redraw = true;
        }

        if (++frame % 60 == 0) {
            const mfb_disc_status next = mfb_disc_probe(&cover_status);
            if (next != disc_status) {
                disc_status = next;
                mfb_logf("Disc state: %s (cover=%08lx, call=%ld)",
                         mfb_disc_status_string(disc_status),
                         (unsigned long)mfb_disc_get_diagnostics()->cover_register,
                         (long)mfb_disc_get_diagnostics()->cover_result);
                disc_id[0] = '\0';
                redraw = true;
            }
        }
        if (redraw)
            draw_menu(&config, row, disc_status, disc_id);
        VIDEO_WaitVSync();
    }
    mfb_settings_shutdown();
    return EXIT_SUCCESS;
}
