/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#include <gccore.h>
#include <ogc/lwp_watchdog.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "mfb/log.h"

#define LOG_CAPACITY 32
#define LOG_LINE_LENGTH 96

static char s_lines[LOG_CAPACITY][LOG_LINE_LENGTH];
static size_t s_next;
static size_t s_count;
static bool s_gecko;

void mfb_log_init(void)
{
    s_next = 0;
    s_count = 0;
    s_gecko = usb_isgeckoalive(EXI_CHANNEL_1) != 0;
}

void mfb_logf(const char *format, ...)
{
    char message[72];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    const u32 millis = ticks_to_millisecs(gettime());
    snprintf(s_lines[s_next], LOG_LINE_LENGTH, "[%lu.%03lu] %s",
             (unsigned long)(millis / 1000), (unsigned long)(millis % 1000), message);

    if (s_gecko) {
        usb_sendbuffer_safe(EXI_CHANNEL_1, s_lines[s_next], strlen(s_lines[s_next]));
        usb_sendbuffer_safe(EXI_CHANNEL_1, "\r\n", 2);
    }
    s_next = (s_next + 1) % LOG_CAPACITY;
    if (s_count < LOG_CAPACITY)
        ++s_count;
}

size_t mfb_log_count(void)
{
    return s_count;
}

const char *mfb_log_get(size_t newest_offset)
{
    if (newest_offset >= s_count)
        return NULL;
    const size_t index = (s_next + LOG_CAPACITY - 1 - newest_offset) % LOG_CAPACITY;
    return s_lines[index];
}

bool mfb_log_usb_gecko(void)
{
    return s_gecko;
}
