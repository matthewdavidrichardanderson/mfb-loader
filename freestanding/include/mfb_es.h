/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#ifndef MFB_ES_H
#define MFB_ES_H
#include "mfb_types.h"
enum {
    MFB_ES_FAIL_OPEN = -201,
    MFB_ES_FAIL_VIEW_COUNT = -202,
    MFB_ES_FAIL_VIEW_RANGE = -203,
    MFB_ES_FAIL_VIEWS = -204,
    MFB_ES_FAIL_LAUNCH = -205,
    MFB_ES_FAIL_READY = -206,
    MFB_ES_FAIL_VERSION = -207,
};
s32 mfb_es_launch_ios(u8 ios);
#endif
