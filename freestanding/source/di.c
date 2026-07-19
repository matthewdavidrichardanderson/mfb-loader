/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#include "mfb_di.h"
#include "mfb_ipc.h"
#include "mfb_memory.h"

static const char di_path[32] ALIGNED(32) = "/dev/di";
static s32 fd = -1;
static u32 input[8] ALIGNED(32);
static u32 output[8] ALIGNED(32);
static u8 tmd[0x49e4] ALIGNED(32);
static mfb_iovec vectors[5] ALIGNED(32);

static s32 normalized(s32 value) { return value == 1 ? 0 : (value < 0 ? value : -value); }
static s32 simple(u32 command)
{
    mfb_memset(input, 0, sizeof(input)); mfb_memset(output, 0, sizeof(output));
    input[0] = command << 24;
    if (command == 0x8a) input[1] = 1;
    return normalized(mfb_ios_ioctl(fd, command, input, 0x20, output, 0x20));
}
s32 mfb_di_open(void) { fd = mfb_ios_open(di_path, 2); return fd < 0 ? fd : 0; }
s32 mfb_di_close(void) { return fd < 0 ? 0 : mfb_ios_close(fd); }
s32 mfb_di_reset(void) { return simple(0x8a); }
s32 mfb_di_identify(void) { return simple(0x12); }
s32 mfb_di_read_id(void *buffer)
{
    mfb_memset(input, 0, sizeof(input)); input[0] = 0x70000000;
    return normalized(mfb_ios_ioctl(fd, 0x70, input, 0x20, buffer, 0x20));
}
s32 mfb_di_open_partition(u32 offset)
{
    mfb_memset(input,0,sizeof(input)); mfb_memset(output,0,sizeof(output)); mfb_memset(tmd,0,sizeof(tmd));
    input[0]=0x8b000000; input[1]=offset;
    vectors[0]=(mfb_iovec){input,0x20}; vectors[1]=(mfb_iovec){0,0x2a4};
    vectors[2]=(mfb_iovec){0,0}; vectors[3]=(mfb_iovec){tmd,sizeof(tmd)};
    vectors[4]=(mfb_iovec){output,0x20};
    return normalized(mfb_ios_ioctlv(fd,0x8b,3,2,vectors));
}
s32 mfb_di_read(void *buffer, u32 length, u32 offset)
{
    mfb_memset(input,0,sizeof(input)); input[0]=0x71000000; input[1]=length; input[2]=offset;
    return normalized(mfb_ios_ioctl(fd,0x71,input,0x20,buffer,length));
}
