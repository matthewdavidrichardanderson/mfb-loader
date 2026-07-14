/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#include "mfb_ipc.h"
#include "mfb_cache.h"
#include "mfb_memory.h"

#define IPC_BASE 0xcd000000u
#define IPC_MSG_PPC (*(volatile u32 *)(IPC_BASE + 0x00))
#define IPC_CTRL    (*(volatile u32 *)(IPC_BASE + 0x04))
#define IPC_MSG_ARM (*(volatile u32 *)(IPC_BASE + 0x08))

typedef struct {
    u32 command;
    s32 result;
    s32 fd;
    u32 args[5];
    u32 user[8];
} mfb_ipc_request;

static mfb_ipc_request request ALIGNED(64);

static void barrier(void) { __asm__ volatile("eieio; sync" : : : "memory"); }
static u32 physical(const void *p) { return (u32)p & 0x3fffffffu; }

/* Broadway's time base runs at 60.75 MHz.  Older IOS versions need a short
 * settling interval after changing IPC ownership; acknowledging immediately
 * can leave both processors waiting on each other. */
static u64 timebase(void)
{
    u32 upper1, lower, upper2;
    do {
        __asm__ volatile("mftbu %0" : "=r"(upper1));
        __asm__ volatile("mftb %0" : "=r"(lower));
        __asm__ volatile("mftbu %0" : "=r"(upper2));
    } while (upper1 != upper2);
    return ((u64)upper1 << 32) | lower;
}

static void delay_us(u32 microseconds)
{
    const u64 end = timebase() + (u64)microseconds * 61u;
    while (timebase() < end) {}
}

static int wait_control(u32 mask)
{
    const u64 end = timebase() + 3u * 60750000u;
    while ((IPC_CTRL & mask) == 0) {
        if (timebase() >= end) return 0;
    }
    delay_us(500);
    return 1;
}

void mfb_ipc_init(void) { IPC_CTRL = 0x06; barrier(); }

static s32 submit(void)
{
    mfb_dc_flush(&request, sizeof(request));
    IPC_MSG_PPC = physical(&request);
    barrier();
    IPC_CTRL = 1;
    barrier();
    if (!wait_control(2)) return -116;
    IPC_CTRL = 2;
    barrier();
    for (;;) {
        if (!wait_control(4)) return -116;
        const u32 reply = IPC_MSG_ARM;
        IPC_CTRL = 4;
        barrier();
        IPC_CTRL = 8;
        barrier();
        if (reply == physical(&request)) break;
    }
    mfb_dc_invalidate(&request, sizeof(request));
    return request.result;
}

s32 mfb_ios_open(const char *path, u32 mode)
{
    mfb_memset(&request, 0, sizeof(request));
    mfb_dc_flush(path, 32);
    request.command = 1;
    request.args[0] = physical(path);
    request.args[1] = mode;
    return submit();
}

s32 mfb_ios_close(s32 fd)
{
    mfb_memset(&request, 0, sizeof(request));
    request.command = 2;
    request.fd = fd;
    return submit();
}

s32 mfb_ios_ioctl(s32 fd, u32 command, void *input, u32 input_length,
                  void *output, u32 output_length)
{
    mfb_memset(&request, 0, sizeof(request));
    if (input && input_length) mfb_dc_flush(input, input_length);
    if (output && output_length) mfb_dc_flush(output, output_length);
    request.command = 6;
    request.fd = fd;
    request.args[0] = command;
    request.args[1] = physical(input);
    request.args[2] = input_length;
    request.args[3] = physical(output);
    request.args[4] = output_length;
    const s32 result = submit();
    if (output && output_length) mfb_dc_invalidate(output, output_length);
    return result;
}

s32 mfb_ios_ioctlv(s32 fd, u32 command, u32 input_count, u32 output_count,
                   mfb_iovec *vectors)
{
    const u32 count = input_count + output_count;
    mfb_memset(&request, 0, sizeof(request));
    for (u32 i = 0; i < count; ++i) {
        if (vectors[i].data && vectors[i].length) {
            mfb_dc_flush(vectors[i].data, vectors[i].length);
            vectors[i].data = (void *)physical(vectors[i].data);
        }
    }
    mfb_dc_flush(vectors, count * sizeof(*vectors));
    request.command = 7;
    request.fd = fd;
    request.args[0] = command;
    request.args[1] = input_count;
    request.args[2] = output_count;
    request.args[3] = physical(vectors);
    const s32 result = submit();
    for (u32 i = input_count; i < count; ++i) {
        if (vectors[i].data) {
            vectors[i].data = (void *)((u32)vectors[i].data | 0x80000000u);
            mfb_dc_invalidate(vectors[i].data, vectors[i].length);
        }
    }
    return result;
}
