/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#ifndef MFB_IPC_H
#define MFB_IPC_H
#include "mfb_types.h"
typedef struct { void *data; u32 length; } mfb_iovec;
void mfb_ipc_init(void);
s32 mfb_ipc_finish_reboot(u8 expected_ios);
s32 mfb_ios_open(const char *path, u32 mode);
s32 mfb_ios_close(s32 fd);
s32 mfb_ios_ioctl(s32 fd, u32 command, void *input, u32 input_length,
                  void *output, u32 output_length);
s32 mfb_ios_ioctlv(s32 fd, u32 command, u32 input_count, u32 output_count,
                   mfb_iovec *vectors);
s32 mfb_ios_ioctlv_reboot(s32 fd, u32 command, u32 input_count,
                          mfb_iovec *vectors);
#endif
