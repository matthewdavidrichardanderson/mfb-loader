/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#include "mfb_types.h"
#include "mfb_cache.h"
#include "mfb_di.h"
#include "mfb_es.h"
#include "mfb_ipc.h"
#include "mfb_memory.h"
#include "mfb_patch.h"

/* The physical-disc and apploader foundation follows TinyLoad by Hector
 * Martin. TinyLoad includes Homebrew Channel/geckoloader stub lineage with
 * copyright notices for Andre Heider and Hector Martin. */

#define PARAM ((volatile mfb_params *)0x90100000)
#define MAGIC 0x4d46424c
typedef struct {
    u32 magic, partition_offset;
    u8 requested_ios, reserved[3];
    u32 config[6];
    u32 diagnostic_xfb;
    u32 vi_vtrdcr, vi_vto, vi_vte;
} mfb_params;
typedef struct { char revision[16]; void *entry; u32 size, trailer, pad; } app_header;
typedef void (*entry_fn)(void (**)(void (*)(const char *, ...)), int (**)(void **,int *,int *), void *(**)(void));
static app_header header ALIGNED(32);
static u8 disc_id[32] ALIGNED(32);
static mfb_patch_report patch_report;
extern u8 __stack_top[];

static void report(const char *format, ...) { (void)format; }
static u64 ticks(void)
{
    u32 hi1, lo, hi2;
    do {
        __asm__ volatile("mftbu %0" : "=r"(hi1));
        __asm__ volatile("mftb %0" : "=r"(lo));
        __asm__ volatile("mftbu %0" : "=r"(hi2));
    } while (hi1 != hi2);
    return ((u64)hi1 << 32) | lo;
}
static void wait_us(u32 us)
{
    const u64 until=ticks()+(u64)us*61u;
    while(ticks()<until){}
}
static void restore_vi(void)
{
    *(volatile u32*)0xcc002000=PARAM->vi_vtrdcr;
    *(volatile u32*)0xcc00200c=PARAM->vi_vto;
    *(volatile u32*)0xcc002010=PARAM->vi_vte;
}
static NORETURN void stop(u32 code)
{
    *(volatile u32 *)0x800030f0=code;
    mfb_dc_flush((void*)0x800030e0,32);
    restore_vi();
    /* Preserve the launch screen but paint one bright bar per failed stage.
     * This remains readable over composite and does not require libogc. */
    if (PARAM->diagnostic_xfb) {
        volatile u32 *xfb=(volatile u32*)PARAM->diagnostic_xfb;
        const u32 stage=code&15u;
        for(u32 y=0;y<48u;++y) {
            for(u32 x=0;x<320u;++x) {
                const u32 cell=x/32u, within=x%32u;
                xfb[y*320u+x]=(cell<stage&&within<20u)?0xeb80eb80:0x10801080;
            }
        }
        mfb_dc_flush((void*)xfb,48u*320u*sizeof(*xfb));
    }
    /* Once game sections replace the XFB, repeat the same stage count on the
     * blue disc-slot light: short pulses, followed by a longer pause. */
    for(;;) {
        const u32 stage=code&15u;
        for(u32 i=0;i<stage;++i) {
            *(volatile u32*)0xcd8000c0|=0x20u;
            wait_us(180000);
            *(volatile u32*)0xcd8000c0&=~0x20u;
            wait_us(180000);
        }
        wait_us(900000);
    }
}
static void lowmem(u8 ios,const u32 config[6])
{
    const u32 ios_version=*(volatile u32*)0x80003140;
    (void)ios;
    *(volatile u32*)0x80000020=0x0d15ea5e;
    *(volatile u32*)0x80000024=1;
    *(volatile u32*)0x80000028=0x01800000;
    *(volatile u32*)0x8000002c=0x23;
    *(volatile u32*)0x80000060=0x38a00040;
    *(volatile u32*)0x800000e4=0x8008f7b8;
    *(volatile u32*)0x800000f0=0x01800000;
    *(volatile u32*)0x800000f8=0x0e7be2c0;
    *(volatile u32*)0x800000fc=0x2b73a840;
    if(config[2]==1 || config[2]==3)
        *(volatile u32*)0x800000cc=0;
    else if(config[2]==2)
        *(volatile u32*)0x800000cc=1;
    *(volatile u32*)0x800030c0=0;
    *(volatile u32*)0x800030c4=0;
    *(volatile u32*)0x800030d8=0;
    *(volatile u32*)0x800030dc=0;
    *(volatile u32*)0x800030e0=0;
    *(volatile u32*)0x800030e4=0;
    *(volatile u32*)0x80003100=0x01800000;
    *(volatile u32*)0x80003104=0x01800000;
    *(volatile u32*)0x8000310c=0;
    *(volatile u32*)0x80003110=0;
    *(volatile u32*)0x80003118=0x04000000;
    *(volatile u32*)0x8000311c=0x04000000;
    *(volatile u32*)0x80003120=0x93400000;
    *(volatile u32*)0x80003124=0x90000800;
    *(volatile u32*)0x80003128=0x933e0000;
    *(volatile u32*)0x80003130=0x933e0000;
    *(volatile u32*)0x80003134=0x93400000;
    *(volatile u32*)0x80003138=0x00000011;
    *(volatile u32*)0x8000315c=0x80000113;
    *(volatile u32*)0x80003180=*(volatile u32*)0x80000000;
    *(volatile u32*)0x80003184=0x80000000;
    *(volatile u32*)0x80003188=ios_version;
    *(volatile u32*)0x8000318c=0;
    *(volatile u32*)0x80003190=0;
    *(volatile u32*)0xcd006c00=0;
    mfb_dc_flush((void*)0x80000000,0x3400);
}
void mfb_payload_main(void)
{
    mfb_params p; mfb_memcpy(&p,(const void*)PARAM,sizeof(p));
    if(p.magic!=MAGIC) stop(0xe001);
    mfb_ipc_init();
    if(mfb_es_launch_ios(p.requested_ios)<0) stop(0xe00f);
    wait_us(100000);
    mfb_ipc_init();
    if(mfb_di_open()<0) stop(0xe002);
    /* The drive was reset and identified during inspection.  After switching
     * to the title's IOS, preserve that state: reopen DI and the partition,
     * matching the proven physical-disc path instead of resetting twice. */
    if(mfb_di_read_id(disc_id)<0) stop(0xe003);
    if(mfb_di_open_partition(p.partition_offset)<0) stop(0xe004);
    if(mfb_di_read((void*)0x80000000,0x20,0)<0||*(u32*)0x80000018!=0x5d1c9ea3) stop(0xe005);
    if(mfb_di_read(&header,0x20,0x910)<0) stop(0xe006);
    const u32 app_size=header.size+header.trailer;
    if(!app_size||app_size>0x100000||mfb_di_read((void*)0x81200000,app_size,0x918)<0) stop(0xe007);
    mfb_dc_flush((void*)0x81200000,app_size); mfb_ic_invalidate((void*)0x81200000,app_size);
    void (*init)(void(*)(const char*,...)); int (*next)(void**,int*,int*); void *(*final)(void);
    ((entry_fn)header.entry)(&init,&next,&final); init(report);
    mfb_patch_init(&patch_report);
    for(;;){ void *dst; int length,offset; if(next(&dst,&length,&offset)!=1)break;
        const u32 section_start=(u32)dst;
        const u32 section_end=section_start+(u32)length;
        if(length<=0||section_end<section_start)stop(0xe008);
        if(section_start<(u32)__stack_top&&section_end>0x80f00000u)stop(0xe00b);
        if(mfb_di_read(dst,(u32)length,(u32)offset)<0)stop(0xe008);
        mfb_patch_section(&patch_report,p.config,dst,(u32)length);
        mfb_dc_flush(dst,(u32)length); mfb_ic_invalidate(dst,(u32)length); }
    *(volatile u32*)0x800030f4=patch_report.modes_seen;
    *(volatile u32*)0x800030f8=patch_report.widths_changed;
    *(volatile u32*)0x800030fc=patch_report.filters_changed;
    mfb_patch_finalize(&patch_report,p.config);
    if(p.config[0]!=0 && patch_report.runtime_width_changed==0)stop(0xe00c);
    if((p.config[3]&0xff000000u)!=0 && patch_report.pixel_480p_changed==0)stop(0xe00d);
    if((p.config[3]&0x00ff0000u)!=0 && patch_report.dither_functions_changed==0)stop(0xe011);
    if(p.config[2]!=0 && patch_report.video_modes_changed==0)stop(0xe00e);
    mfb_di_close();
    lowmem(p.requested_ios,p.config);
    wait_us(60000);
    void (*game)(void)=final(); if(!game)stop(0xe009);
    restore_vi();
    /* The game entry point is a one-way transfer, not an ordinary C call.
     * Match established Wii booters: put the entry in LR and branch without
     * linking, so no return address into MFB survives the handoff. */
    __asm__ volatile("mtlr %0\n\tblr" : : "r"(game));
    __builtin_unreachable();
}
