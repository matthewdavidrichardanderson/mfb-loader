/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#ifndef MFB_TYPES_H
#define MFB_TYPES_H
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long long u64;
#define ALIGNED(n) __attribute__((aligned(n)))
#define NORETURN __attribute__((noreturn))
#endif
