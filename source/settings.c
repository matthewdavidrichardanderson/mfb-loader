/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#include <fat.h>
#include <ogc/usbstorage.h>
#include <sdcard/wiisd_io.h>
#include <stdio.h>
#include <string.h>

#include "mfb/settings.h"

#define MFB_SETTINGS_MAGIC 0x4d464243u
#define MFB_SETTINGS_VERSION 1u

typedef struct {
    u32 magic;
    u32 version;
    u32 size;
    u32 scale_mode;
    u32 filter_mode;
    u32 video_mode;
    u32 patch_480p;
    u32 region_policy;
    u32 aspect_mode;
    u32 checksum;
} mfb_settings_record;

static bool s_mounted;
static char s_volume[4];
static char s_path[256];

static u32 checksum(const mfb_settings_record *record)
{
    const u32 *words = (const u32 *)record;
    u32 value = 2166136261u;
    for (u32 i = 0; i < (sizeof(*record) / sizeof(*words)) - 1; ++i) {
        value ^= words[i];
        value *= 16777619u;
    }
    return value;
}

static mfb_settings_record encode(const mfb_launch_config *config)
{
    mfb_settings_record record = {
        .magic = MFB_SETTINGS_MAGIC,
        .version = MFB_SETTINGS_VERSION,
        .size = sizeof(mfb_settings_record),
        .scale_mode = config->scale_mode,
        .filter_mode = config->filter_mode,
        .video_mode = config->video_mode,
        .patch_480p = config->patch_480p ? 1u : 0u,
        .region_policy = config->region_policy,
        .aspect_mode = config->aspect_mode,
        .checksum = 0,
    };
    record.checksum = checksum(&record);
    return record;
}

static bool decode(const mfb_settings_record *record, mfb_launch_config *config)
{
    if (record->magic != MFB_SETTINGS_MAGIC ||
        record->version != MFB_SETTINGS_VERSION ||
        record->size != sizeof(*record) ||
        record->checksum != checksum(record))
        return false;

    const mfb_launch_config candidate = {
        .scale_mode = record->scale_mode,
        .filter_mode = record->filter_mode,
        .video_mode = record->video_mode,
        .patch_480p = record->patch_480p != 0,
        .region_policy = record->region_policy,
        .aspect_mode = record->aspect_mode,
    };
    if (!mfb_config_valid(&candidate))
        return false;
    *config = candidate;
    return true;
}

bool mfb_settings_init(mfb_launch_config *config, const char *launch_path)
{
    if (config == NULL)
        return false;

    const bool from_usb = launch_path != NULL &&
                          strncmp(launch_path, "usb:/", 5) == 0;
    DISC_INTERFACE *interface = from_usb ? &__io_usbstorage : &__io_wiisd;
    strcpy(s_volume, from_usb ? "usb" : "sd");
    if (!fatMountSimple(s_volume, interface))
        return false;
    s_mounted = true;

    const char *slash = launch_path == NULL ? NULL : strrchr(launch_path, '/');
    const size_t directory_length = slash == NULL ? 0 : (size_t)(slash - launch_path + 1);
    if (directory_length > 0 && directory_length + sizeof("settings.cfg") <= sizeof(s_path)) {
        memcpy(s_path, launch_path, directory_length);
        strcpy(s_path + directory_length, "settings.cfg");
    } else {
        snprintf(s_path, sizeof(s_path), "%s:/apps/mfb-loader/settings.cfg", s_volume);
    }
    FILE *file = fopen(s_path, "rb");
    if (file == NULL)
        return false;
    mfb_settings_record record;
    const bool read_ok = fread(&record, sizeof(record), 1, file) == 1;
    fclose(file);
    return read_ok && decode(&record, config);
}

void mfb_settings_save(const mfb_launch_config *config)
{
    if (!s_mounted || !mfb_config_valid(config))
        return;

    char temporary[sizeof(s_path) + 4];
    snprintf(temporary, sizeof(temporary), "%s.tmp", s_path);
    FILE *file = fopen(temporary, "wb");
    if (file == NULL)
        return;
    const mfb_settings_record record = encode(config);
    const bool wrote = fwrite(&record, sizeof(record), 1, file) == 1;
    const bool closed = fclose(file) == 0;
    if (!wrote || !closed) {
        remove(temporary);
        return;
    }
    remove(s_path);
    if (rename(temporary, s_path) != 0)
        remove(temporary);
}

void mfb_settings_shutdown(void)
{
    if (!s_mounted)
        return;
    fatUnmount(s_volume);
    s_mounted = false;
}
