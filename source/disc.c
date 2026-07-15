/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#include <ogc/ios.h>
#include <di/di.h>
#include <string.h>
#include <ogc/ipc.h>

#include "mfb/disc.h"
#include "mfb/log.h"
#include "mfb/launch.h"
#include "mfb/video.h"

extern const u8 mfb_freestanding_bin[];
extern const u8 mfb_freestanding_bin_end[];

static mfb_disc_diagnostics s_diagnostics = {
    .di_init_result = -999,
    .cover_result = -999,
    .disc_id_result = -999,
    .inspect_result = -999,
    .reload_result = -999,
    .post_reload_di_init = -999,
    .encrypted_read_result = -999,
};
static bool s_initialized;

typedef struct {
    u32 count;
    u32 table_offset;
    u32 padding[6];
} mfb_partition_info;

typedef struct {
    u32 offset;
    u32 type;
} mfb_partition_entry;

static mfb_partition_info s_partition_info ATTRIBUTE_ALIGN(32);
static mfb_partition_entry s_partitions[32] ATTRIBUTE_ALIGN(32);
static u8 s_tmd[0x49e4] ATTRIBUTE_ALIGN(32);
static u32 s_disc_header[8] ATTRIBUTE_ALIGN(32);
static u32 s_open_in[8] ATTRIBUTE_ALIGN(32);
static u32 s_open_out[8] ATTRIBUTE_ALIGN(32);
static s32 s_stock_di_fd = -1;

static s32 normalize_di_result(s32 result)
{
    return result == 1 ? 0 : (result < 0 ? result : -result);
}

static s32 active_di_fd(void)
{
    return s_stock_di_fd >= 0 ? s_stock_di_fd : di_fd;
}

static s32 stock_ioctl(u32 command)
{
    memset(s_open_in, 0, sizeof(s_open_in));
    memset(s_open_out, 0, sizeof(s_open_out));
    s_open_in[0] = command << 24;
    if (command == DVD_RESET)
        s_open_in[1] = 1;
    return normalize_di_result(IOS_Ioctl(active_di_fd(), command, s_open_in, 0x20,
                                         s_open_out, 0x20));
}

static s32 stock_read(void *buffer, u32 size, u32 offset)
{
    memset(s_open_in, 0, sizeof(s_open_in));
    s_open_in[0] = DVD_LOW_READ << 24;
    s_open_in[1] = size;
    s_open_in[2] = offset;
    return normalize_di_result(IOS_Ioctl(active_di_fd(), DVD_LOW_READ,
                                         s_open_in, 0x20, buffer, size));
}

static s32 stock_get_cover(u32 *status)
{
    const s32 result = stock_ioctl(DVD_GETCOVER);
    if (result == 0)
        *status = s_open_out[0];
    return result;
}

static s32 open_partition_with_tmd(u32 offset)
{
    ioctlv vectors[5] ATTRIBUTE_ALIGN(32);
    memset(s_open_in, 0, sizeof(s_open_in));
    memset(s_open_out, 0, sizeof(s_open_out));
    memset(s_tmd, 0, sizeof(s_tmd));
    s_open_in[0] = DVD_OPEN_PARTITION << 24;
    s_open_in[1] = offset;
    vectors[0] = (ioctlv){s_open_in, 0x20};
    vectors[1] = (ioctlv){NULL, 0x2a4};
    vectors[2] = (ioctlv){NULL, 0};
    vectors[3] = (ioctlv){s_tmd, sizeof(s_tmd)};
    vectors[4] = (ioctlv){s_open_out, 0x20};
    return normalize_di_result(IOS_Ioctlv(active_di_fd(), DVD_OPEN_PARTITION,
                                          3, 2, vectors));
}

/*
 * Physical discs intentionally go through libogc's stock DVD interface.
 * IOS 249/250/251 are conventional cIOS slots and are rejected: their DIP
 * modules can change the observable timing and semantics we are preserving.
 */
mfb_disc_status mfb_disc_probe(u32 *cover_status)
{
    const s32 ios = IOS_GetVersion();
    s_diagnostics.ios_version = ios;
    s_diagnostics.ios_revision = IOS_GetRevision();
    if (ios == 249 || ios == 250 || ios == 251)
        return MFB_DISC_UNSUPPORTED_IOS;

    if (cover_status == NULL)
        return MFB_DISC_IO_ERROR;

    if (!s_initialized && s_stock_di_fd < 0) {
        s_diagnostics.di_init_result = DI_Init();
        mfb_logf("DI_Init -> %ld", (long)s_diagnostics.di_init_result);
        if (s_diagnostics.di_init_result < 0)
            return MFB_DISC_IO_ERROR;
        s_initialized = true;
    }

    const s32 result = s_stock_di_fd >= 0 ? stock_get_cover(cover_status) :
                                           DI_GetCoverRegister(cover_status);
    s_diagnostics.cover_result = result;
    s_diagnostics.cover_register = *cover_status;
    if (result < 0)
    {
        if (DI_GetError(&s_diagnostics.drive_error) < 0)
            s_diagnostics.drive_error = 0xffffffff;
        return MFB_DISC_IO_ERROR;
    }
    if ((*cover_status & DVD_COVER_DISC_INSERTED) == 0) {
        s_diagnostics.di_status = DI_GetStatus();
        return MFB_DISC_NO_MEDIA;
    }
    s_diagnostics.di_status = DI_GetStatus();
    return MFB_DISC_READY;
}

s32 mfb_disc_reload_and_verify(void)
{
    if (s_diagnostics.inspect_result != 0 || s_diagnostics.requested_ios == 0 ||
        s_diagnostics.game_partition_offset == 0)
        return -1100;

    const u8 requested_ios = s_diagnostics.requested_ios;
    const u32 partition_offset = s_diagnostics.game_partition_offset;
    mfb_logf("Reload gate: closing DI, requesting stock IOS%u", requested_ios);
    DI_Close();
    s_initialized = false;

    s_diagnostics.reload_result = IOS_ReloadIOS(requested_ios);
    s_diagnostics.post_reload_ios = IOS_GetVersion();
    mfb_logf("IOS_ReloadIOS(%u) -> %ld; now IOS%ld r%ld", requested_ios,
             (long)s_diagnostics.reload_result, (long)s_diagnostics.post_reload_ios,
             (long)IOS_GetRevision());
    if (s_diagnostics.reload_result < 0)
        return s_diagnostics.reload_result;

    static const char di_path[] ATTRIBUTE_ALIGN(32) = "/dev/di";
    s_stock_di_fd = IOS_Open(di_path, 2);
    s_diagnostics.post_reload_di_init = s_stock_di_fd;
    mfb_logf("Post-reload IOS_Open(/dev/di) -> %ld", (long)s_stock_di_fd);
    if (s_stock_di_fd < 0)
        return s_diagnostics.post_reload_di_init;

    s_diagnostics.reset_result = stock_ioctl(DVD_RESET);
    if (s_diagnostics.reset_result < 0)
        return s_diagnostics.reset_result;
    s_diagnostics.identify_result = stock_ioctl(DVD_IDENTIFY);
    if (s_diagnostics.identify_result < 0)
        return s_diagnostics.identify_result;
    s_diagnostics.disc_id_result = stock_ioctl(DVD_READ_DISCID);
    if (s_diagnostics.disc_id_result < 0)
        return s_diagnostics.disc_id_result;

    s32 result = open_partition_with_tmd(partition_offset);
    if (result < 0)
        return result;
    s_diagnostics.encrypted_read_result = stock_read(s_disc_header, sizeof(s_disc_header), 0);
    stock_ioctl(DVD_CLOSE_PARTITION);
    if (s_diagnostics.encrypted_read_result < 0)
        return s_diagnostics.encrypted_read_result;
    if (s_disc_header[6] != 0x5d1c9ea3)
        return -1101;

    mfb_logf("Reload gate OK: IOS%ld, partition reopened, encrypted header valid",
             (long)s_diagnostics.post_reload_ios);
    return 0;
}

s32 mfb_disc_launch(const mfb_launch_config *config)
{
    if (!mfb_config_valid(config) || s_diagnostics.inspect_result != 0 ||
        s_diagnostics.requested_ios == 0 || s_diagnostics.game_partition_offset == 0)
        return -1200;
    const u32 payload_size = (u32)(mfb_freestanding_bin_end - mfb_freestanding_bin);
    if (payload_size == 0 || payload_size > 0x10000)
        return -1201;
    mfb_launch_params params;
    memset(&params, 0, sizeof(params));
    params.magic = MFB_LAUNCH_MAGIC;
    params.partition_offset = s_diagnostics.game_partition_offset;
    params.requested_ios = s_diagnostics.requested_ios;
    params.config = *config;
    params.diagnostic_xfb = (u32)mfb_video_framebuffer();
    params.vi_vtrdcr = *(volatile u32 *)0xcc002000;
    params.vi_vto = *(volatile u32 *)0xcc00200c;
    params.vi_vte = *(volatile u32 *)0xcc002010;
    void *const payload_address = (void *)0x80f00000;
    mfb_logf("Launch handoff: %lu-byte freestanding baseline, IOS%u, partition %08lx",
             (unsigned long)payload_size, params.requested_ios,
             (unsigned long)params.partition_offset);
    /* Install the self-contained loader before leaving libogc.  It owns the
     * minimal ES reload and DI handoff after libogc shuts down. */
    memcpy(MFB_LAUNCH_PARAM_ADDR, &params, sizeof(params));
    memcpy(payload_address, mfb_freestanding_bin, payload_size);
    DCFlushRange(MFB_LAUNCH_PARAM_ADDR, sizeof(params));
    DCFlushRange(payload_address, payload_size);
    ICInvalidateRange(payload_address, payload_size);

    DI_Close();
    s_initialized = false;
    VIDEO_SetBlack(true);
    VIDEO_Flush();
    VIDEO_WaitVSync();

    /* Do not initialize libogc under the game's IOS.  The payload takes IPC
     * ownership, launches the requested IOS through ES, and opens only DI. */
    const s32 shutdown_result = __IOS_ShutdownSubsystems();
    if (shutdown_result < 0)
        return shutdown_result;

    ((void (*)(void))payload_address)();
    return -1202;
}

bool mfb_disc_read_id(char id[7])
{
    u64 raw_id = 0;
    if (id == NULL)
        return false;
    s_diagnostics.disc_id_result = DI_ReadDiscID(&raw_id);
    if (s_diagnostics.disc_id_result < 0) {
        if (DI_GetError(&s_diagnostics.drive_error) < 0)
            s_diagnostics.drive_error = 0xffffffff;
        mfb_logf("DI_ReadDiscID -> %ld, drive error %08lx",
                 (long)s_diagnostics.disc_id_result,
                 (unsigned long)s_diagnostics.drive_error);
        return false;
    }
    memcpy(id, &raw_id, 6);
    id[6] = '\0';
    mfb_logf("Disc ID: %s", id);
    return true;
}

const mfb_disc_diagnostics *mfb_disc_get_diagnostics(void)
{
    return &s_diagnostics;
}

s32 mfb_disc_inspect(void)
{
    s_diagnostics.partition_count = 0;
    s_diagnostics.game_partition_offset = 0;
    s_diagnostics.requested_ios = 0;

    s_diagnostics.reset_result = DI_Reset();
    mfb_logf("Inspect: DI_Reset -> %ld", (long)s_diagnostics.reset_result);
    if (s_diagnostics.reset_result < 0) {
        s_diagnostics.inspect_result = s_diagnostics.reset_result;
        return s_diagnostics.inspect_result;
    }

    DI_DriveID drive_id ATTRIBUTE_ALIGN(32);
    s_diagnostics.identify_result = DI_Identify(&drive_id);
    mfb_logf("Inspect: DI_Identify -> %ld", (long)s_diagnostics.identify_result);
    if (s_diagnostics.identify_result < 0) {
        s_diagnostics.inspect_result = s_diagnostics.identify_result;
        return s_diagnostics.inspect_result;
    }

    u64 raw_id ATTRIBUTE_ALIGN(32) = 0;
    s_diagnostics.disc_id_result = DI_ReadDiscID(&raw_id);
    if (s_diagnostics.disc_id_result < 0) {
        DI_GetError(&s_diagnostics.drive_error);
        s_diagnostics.inspect_result = s_diagnostics.disc_id_result;
        mfb_logf("Inspect: DI_ReadDiscID -> %ld, error=%08lx",
                 (long)s_diagnostics.disc_id_result,
                 (unsigned long)s_diagnostics.drive_error);
        return s_diagnostics.inspect_result;
    }

    s32 result = DI_UnencryptedRead(s_disc_header, sizeof(s_disc_header), 0);
    if (result < 0 || s_disc_header[6] != 0x5d1c9ea3) {
        s_diagnostics.inspect_result = result < 0 ? result : -1000;
        mfb_logf("Inspect: Wii magic %08lx, read %ld",
                 (unsigned long)s_disc_header[6], (long)result);
        return s_diagnostics.inspect_result;
    }

    result = DI_UnencryptedRead(&s_partition_info, sizeof(s_partition_info), 0x10000);
    if (result < 0) {
        s_diagnostics.inspect_result = result;
        mfb_logf("Inspect: partition info read -> %ld", (long)result);
        return result;
    }
    if (s_partition_info.count == 0 || s_partition_info.count > 32) {
        s_diagnostics.inspect_result = -1001;
        mfb_logf("Inspect: invalid partition count %lu", (unsigned long)s_partition_info.count);
        return s_diagnostics.inspect_result;
    }

    s_diagnostics.partition_count = s_partition_info.count;
    /* IOS DI requires the IPC transfer size to be 32-byte aligned. Reading the
     * fixed-capacity table also matches the retail-loader/TinyLoad sequence. */
    result = DI_UnencryptedRead(s_partitions, sizeof(s_partitions),
                                s_partition_info.table_offset);
    if (result < 0) {
        s_diagnostics.inspect_result = result;
        mfb_logf("Inspect: partition table read -> %ld", (long)result);
        return result;
    }

    for (u32 i = 0; i < s_partition_info.count; ++i) {
        if (s_partitions[i].type == 0) {
            s_diagnostics.game_partition_offset = s_partitions[i].offset;
            break;
        }
    }
    if (s_diagnostics.game_partition_offset == 0) {
        s_diagnostics.inspect_result = -1002;
        mfb_logf("Inspect: no game partition");
        return s_diagnostics.inspect_result;
    }

    result = open_partition_with_tmd(s_diagnostics.game_partition_offset);
    if (result < 0) {
        s_diagnostics.inspect_result = result;
        mfb_logf("Inspect: open partition %08lx -> %ld",
                 (unsigned long)s_diagnostics.game_partition_offset, (long)result);
        return result;
    }
    s_diagnostics.requested_ios = s_tmd[0x18b];
    const s32 close_result = DI_ClosePartition();
    if (close_result < 0) {
        s_diagnostics.inspect_result = close_result;
        mfb_logf("Inspect: close partition -> %ld", (long)close_result);
        return close_result;
    }

    s_diagnostics.inspect_result = 0;
    mfb_logf("Inspect OK: %lu partitions, game=%08lx, IOS%u",
             (unsigned long)s_diagnostics.partition_count,
             (unsigned long)s_diagnostics.game_partition_offset,
             s_diagnostics.requested_ios);
    return 0;
}

const char *mfb_disc_status_string(mfb_disc_status status)
{
    switch (status) {
    case MFB_DISC_READY: return "Disc inserted";
    case MFB_DISC_INITIALIZING: return "Initializing retail disc";
    case MFB_DISC_NO_MEDIA: return "Insert a Wii disc";
    case MFB_DISC_UNSUPPORTED_IOS: return "Refusing cIOS: retail timing not guaranteed";
    default: return "Disc interface error";
    }
}
