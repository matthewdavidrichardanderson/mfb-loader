/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#ifndef MFB_SETTINGS_H
#define MFB_SETTINGS_H

#include "mfb/config.h"

bool mfb_settings_init(mfb_launch_config *config, const char *launch_path);
void mfb_settings_save(const mfb_launch_config *config);
void mfb_settings_shutdown(void);

#endif
