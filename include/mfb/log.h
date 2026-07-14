/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#ifndef MFB_LOG_H
#define MFB_LOG_H

#include <stddef.h>

void mfb_log_init(void);
void mfb_logf(const char *format, ...);
size_t mfb_log_count(void);
const char *mfb_log_get(size_t newest_offset);
bool mfb_log_usb_gecko(void);

#endif
