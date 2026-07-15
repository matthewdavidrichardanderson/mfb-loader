/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#include "mfb_es.h"
#include "mfb_ipc.h"

#define ES_LAUNCH_TITLE 0x08
#define ES_GET_VIEW_COUNT 0x12
#define ES_GET_VIEWS 0x13
#define TICKET_VIEW_SIZE 0xd8

static const char es_path[32] ALIGNED(32) = "/dev/es";
static u64 title_id ALIGNED(32);
static u32 view_count ALIGNED(32);
static u8 ticket_views[TICKET_VIEW_SIZE * 4] ALIGNED(32);
static mfb_iovec vectors[3] ALIGNED(32);

s32 mfb_es_launch_ios(u8 ios)
{
    const s32 es = mfb_ios_open(es_path, 0);
    if (es < 0) return es;

    title_id = 0x0000000100000000ULL | ios;
    view_count = 0;
    vectors[0].data = &title_id;
    vectors[0].length = sizeof(title_id);
    vectors[1].data = &view_count;
    vectors[1].length = sizeof(view_count);
    s32 result = mfb_ios_ioctlv(es, ES_GET_VIEW_COUNT, 1, 1, vectors);
    if (result < 0) return result;
    if (view_count == 0 || view_count > 4) return -4;

    vectors[0].data = &title_id;
    vectors[0].length = sizeof(title_id);
    vectors[1].data = &view_count;
    vectors[1].length = sizeof(view_count);
    vectors[2].data = ticket_views;
    vectors[2].length = TICKET_VIEW_SIZE * view_count;
    result = mfb_ios_ioctlv(es, ES_GET_VIEWS, 2, 1, vectors);
    if (result < 0) return result;

    vectors[0].data = &title_id;
    vectors[0].length = sizeof(title_id);
    vectors[1].data = ticket_views;
    vectors[1].length = TICKET_VIEW_SIZE;
    return mfb_ios_ioctlv_reboot(es, ES_LAUNCH_TITLE, 2, vectors);
}
