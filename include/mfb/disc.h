/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#ifndef MFB_DISC_H
#define MFB_DISC_H

#include <gccore.h>
#include "mfb/config.h"

typedef enum {
    MFB_DISC_READY = 0,
    MFB_DISC_INITIALIZING,
    MFB_DISC_NO_MEDIA,
    MFB_DISC_UNSUPPORTED_IOS,
    MFB_DISC_IO_ERROR
} mfb_disc_status;

typedef struct {
    s32 ios_version;
    s32 ios_revision;
    s32 di_init_result;
    s32 cover_result;
    u32 cover_register;
    s32 disc_id_result;
    u32 drive_error;
    s32 di_status;
    s32 reset_result;
    s32 identify_result;
    s32 inspect_result;
    u32 partition_count;
    u32 game_partition_offset;
    u8 requested_ios;
    s32 reload_result;
    s32 post_reload_di_init;
    s32 encrypted_read_result;
    s32 post_reload_ios;
} mfb_disc_diagnostics;

mfb_disc_status mfb_disc_probe(u32 *cover_status);
const char *mfb_disc_status_string(mfb_disc_status status);
bool mfb_disc_read_id(char id[7]);
const mfb_disc_diagnostics *mfb_disc_get_diagnostics(void);
s32 mfb_disc_inspect(void);
s32 mfb_disc_reload_and_verify(void);
s32 mfb_disc_launch(const mfb_launch_config *config);

#endif
