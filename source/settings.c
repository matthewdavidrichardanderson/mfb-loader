/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#include <fat.h>
#include <ogc/usbstorage.h>
#include <sdcard/wiisd_io.h>
#include <stdio.h>
#include <string.h>

#include "mfb/settings.h"

#define MFB_SETTINGS_MAGIC 0x4d464243u
#define MFB_SETTINGS_VERSION 2u
#define MFB_SETTINGS_VARIANT_PATH_CAPACITY (sizeof(s_path) + 8u)

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
    u32 disable_dithering;
    u32 checksum;
} mfb_settings_record;

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
} mfb_settings_record_v1;

static bool s_mounted;
static char s_volume[4];
static char s_path[256];

static u32 checksum_words(const u32 *words, size_t word_count)
{
    u32 value = 2166136261u;
    for (size_t i = 0; i + 1 < word_count; ++i) {
        value ^= words[i];
        value *= 16777619u;
    }
    return value;
}

static u32 checksum(const mfb_settings_record *record)
{
    return checksum_words((const u32 *)record, sizeof(*record) / sizeof(u32));
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
        .disable_dithering = config->disable_dithering ? 1u : 0u,
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
        .disable_dithering = record->disable_dithering != 0,
    };
    if (!mfb_config_valid(&candidate))
        return false;
    *config = candidate;
    return true;
}

static bool decode_v1(const mfb_settings_record_v1 *record, mfb_launch_config *config)
{
    if (record->magic != MFB_SETTINGS_MAGIC || record->version != 1u ||
        record->size != sizeof(*record) ||
        record->checksum != checksum_words((const u32 *)record,
                                           sizeof(*record) / sizeof(u32)))
        return false;

    const mfb_launch_config candidate = {
        .scale_mode = record->scale_mode,
        .filter_mode = record->filter_mode,
        .video_mode = record->video_mode,
        .patch_480p = record->patch_480p != 0,
        .disable_dithering = false,
        .region_policy = record->region_policy,
        .aspect_mode = record->aspect_mode,
    };
    if (!mfb_config_valid(&candidate))
        return false;
    *config = candidate;
    return true;
}

static bool make_variant_path(char *path, size_t capacity, const char *suffix)
{
    const int length = snprintf(path, capacity, "%s%s", s_path, suffix);
    return length >= 0 && (size_t)length < capacity;
}

static bool read_config_file(const char *path, mfb_launch_config *config)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
        return false;

    mfb_settings_record record = {0};
    const size_t bytes_read = fread(&record, 1, sizeof(record), file);
    const bool close_ok = fclose(file) == 0;
    if (!close_ok)
        return false;
    if (bytes_read == sizeof(record))
        return decode(&record, config);
    if (bytes_read == sizeof(mfb_settings_record_v1))
        return decode_v1((const mfb_settings_record_v1 *)&record, config);
    return false;
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
    char temporary[MFB_SETTINGS_VARIANT_PATH_CAPACITY];
    char backup[MFB_SETTINGS_VARIANT_PATH_CAPACITY];
    if (!make_variant_path(temporary, sizeof(temporary), ".tmp") ||
        !make_variant_path(backup, sizeof(backup), ".bak"))
        return false;

    if (read_config_file(s_path, config)) {
        remove(temporary);
        remove(backup);
        return true;
    }

    /* Recover the last known-good record if a save was interrupted. */
    if (read_config_file(backup, config)) {
        remove(s_path);
        rename(backup, s_path);
        remove(temporary);
        return true;
    }

    /* A complete temporary record is still protected by its checksum. */
    if (read_config_file(temporary, config)) {
        remove(s_path);
        rename(temporary, s_path);
        remove(backup);
        return true;
    }

    return false;
}

bool mfb_settings_save(const mfb_launch_config *config)
{
    if (!s_mounted || !mfb_config_valid(config))
        return false;

    char temporary[MFB_SETTINGS_VARIANT_PATH_CAPACITY];
    char backup[MFB_SETTINGS_VARIANT_PATH_CAPACITY];
    if (!make_variant_path(temporary, sizeof(temporary), ".tmp") ||
        !make_variant_path(backup, sizeof(backup), ".bak"))
        return false;

    FILE *file = fopen(temporary, "wb");
    if (file == NULL)
        return false;
    const mfb_settings_record record = encode(config);
    const bool wrote = fwrite(&record, sizeof(record), 1, file) == 1;
    const bool closed = fclose(file) == 0;
    if (!wrote || !closed) {
        remove(temporary);
        return false;
    }

    FILE *existing = fopen(s_path, "rb");
    const bool had_existing = existing != NULL;
    if (existing != NULL)
        fclose(existing);

    remove(backup);
    if (had_existing && rename(s_path, backup) != 0) {
        remove(temporary);
        return false;
    }

    if (rename(temporary, s_path) != 0) {
        if (had_existing)
            rename(backup, s_path);
        remove(temporary);
        return false;
    }

    remove(backup);
    return true;
}

void mfb_settings_shutdown(void)
{
    if (!s_mounted)
        return;
    fatUnmount(s_volume);
    s_mounted = false;
}
