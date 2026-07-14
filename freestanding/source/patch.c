/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 matthewdavidrichardanderson */

#include "mfb_patch.h"
#include "mfb_memory.h"
#include "mfb_cache.h"

/* Opcode signatures and patch values were informed by the GPL implementations
 * in WiiFlow Lite and Gecko OS.  This includes PatchFix480p by leseratte,
 * based on the issue identified by Extrems, and Gecko OS patch work by Nuke. */

/* Binary layout of the publicly documented Revolution/libogc render mode. */
typedef struct {
    u32 tv_mode;
    u16 fb_width, efb_height, xfb_height;
    u16 vi_x_origin, vi_y_origin, vi_width, vi_height;
    u32 xfb_mode;
    u8 field_rendering, antialiasing;
    u8 sample_pattern[24];
    u8 vertical_filter[7];
} rvl_mode;

enum { SCALE_AUTO=0, SCALE_1_TO_1=1 };
enum { FILTER_AUTO=0, FILTER_OFF=1, FILTER_ON=2 };
enum { VIDEO_AUTO=0, VIDEO_480I60=1, VIDEO_576I50=2, VIDEO_480P60=3 };

static const u8 sample_box[24]={
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6
};
static const u8 sample_aa[24]={
    3,2,9,6,3,10,3,2,9,6,3,10,9,2,3,6,9,10,9,2,3,6,9,10
};
static const u8 filter_off[7]={0,0,21,22,21,0,0};
static const u8 filter_on[7]={4,8,12,16,12,8,4};

static int equal(const void *left,const void *right,u32 length)
{
    const u8 *a=left,*b=right;
    while(length--)if(*a++!=*b++)return 0;
    return 1;
}

static int is_mode(const rvl_mode *mode)
{
    const int samples=equal(mode->sample_pattern,sample_box,24) ||
                      equal(mode->sample_pattern,sample_aa,24);
    return samples &&
           (mode->fb_width==512 || mode->fb_width==608 || mode->fb_width==640) &&
           mode->efb_height>=200 && mode->efb_height<=576 &&
           mode->xfb_height>=200 && mode->xfb_height<=1152 &&
           mode->vi_width>=480 && mode->vi_width<=720 &&
           mode->vi_height>=200 && mode->vi_height<=1152 &&
           mode->field_rendering<=1 && mode->antialiasing<=1;
}

static u16 selected_width(u32 scale,u16 framebuffer_width)
{
    static const u16 explicit_widths[7]={0,0,640,656,672,704,720};
    if(scale==SCALE_1_TO_1)return framebuffer_width;
    return scale<7?explicit_widths[scale]:0;
}

static void force_video_mode(rvl_mode *mode,u32 video)
{
    if(video==VIDEO_480I60) {
        mode->tv_mode=0;
        mode->efb_height=480;
        mode->xfb_height=480;
        mode->vi_y_origin=0;
        mode->vi_height=480;
        mode->xfb_mode=1;
        mode->field_rendering=0;
    } else if(video==VIDEO_576I50) {
        /* PAL's 576-line signal has a 528-line active render area in the
         * standard Revolution render mode. */
        mode->tv_mode=4;
        mode->efb_height=528;
        mode->xfb_height=528;
        mode->vi_y_origin=23;
        mode->vi_height=528;
        mode->xfb_mode=1;
        mode->field_rendering=0;
    } else if(video==VIDEO_480P60) {
        /* Preserve the title's EFB/XFB geometry.  Games such as Twilight
         * Princess use a non-480 EFB and depend on that value when setting
         * their viewport and display-copy scale.  Replacing it with 480
         * produces a sharp but vertically displaced picture. */
        mode->tv_mode=2;
        mode->xfb_mode=0;
        mode->field_rendering=0;
    }
}

static void patch_modes(mfb_patch_report *report,const u32 config[6],
                        u8 *data,u32 length)
{
    const u32 scale=config[0], filter=config[1], video=config[2];
    for(u32 offset=0;offset+sizeof(rvl_mode)<=length;offset+=4) {
        rvl_mode *mode=(rvl_mode*)(data+offset);
        if(!is_mode(mode))continue;
        ++report->modes_seen;
        if(video!=VIDEO_AUTO && video<=VIDEO_480P60) {
            force_video_mode(mode,video);
            ++report->video_modes_changed;
        }
        if(scale!=SCALE_AUTO) {
            const u16 width=selected_width(scale,mode->fb_width);
            if(width>=480 && width<=720) {
                mode->vi_width=width;
                mode->vi_x_origin=(u16)((720u-width)/2u);
                ++report->widths_changed;
            }
        }
        if(filter==FILTER_OFF || filter==FILTER_ON) {
            mfb_memcpy(mode->vertical_filter,
                       filter==FILTER_OFF?filter_off:filter_on,7);
            ++report->filters_changed;
        }
        offset+=sizeof(rvl_mode)-4;
    }
}

/* Some titles request filtering again at runtime. For OFF, change the branch
 * in the known GXSetCopyFilter SDK implementation so it always takes the
 * unfiltered path. The full signature prevents a broad code patch. */
static void patch_gx_filter(mfb_patch_report *report,const u32 config[6],
                            u8 *data,u32 length)
{
    static const u32 signature[18]={
        0x3d20cc01,0x39400061,0x99498000,0x2c050000,0x38800053,
        0x39600000,0x90098000,0x38000054,0x39800000,0x508bc00e,
        0x99498000,0x500cc00e,0x90698000,0x99498000,0x90e98000,
        0x99498000,0x91098000,0x41820040
    };
    if(config[1]!=FILTER_OFF || length<sizeof(signature))return;
    for(u32 offset=0;offset+sizeof(signature)<=length;offset+=4) {
        if(equal(data+offset,signature,sizeof(signature))) {
            *(u32*)(data+offset+68)=0x48000040;
            ++report->gx_filter_functions_changed;
        }
    }
}

/* Replace the final return value in the SDK IPL aspect query wrapper.  This
 * changes projection selection only in titles which use the supported query;
 * it does not stretch the VI framebuffer itself. */
static void patch_aspect(mfb_patch_report *report,const u32 config[6],
                         u8 *data,u32 length)
{
    static const u32 head[5]={
        0x9421fff0,0x7c0802a6,0x38800001,0x90010014,0x38610008
    };
    static const u32 tail[15]={
        0x2c030000,0x40820010,0x38000000,0x98010008,0x48000018,
        0x88010008,0x28000001,0x4182000c,0x38000000,0x98010008,
        0x80010014,0x88610008,0x7c0803a6,0x38210010,0x4e800020
    };
    const u32 aspect=config[5];
    if(aspect==0 || aspect>2)return;
    for(u32 offset=0;offset+84u<=length;offset+=4) {
        if(equal(data+offset,head,sizeof(head)) &&
           equal(data+offset+24,tail,sizeof(tail))) {
            *(u32*)(data+offset+68)=0x38600000u|(aspect==2?1u:0u);
            ++report->aspect_functions_changed;
        }
    }
}

static u32 branch(u32 from,u32 to)
{
    return 0x48000000u|((to-from)&0x03fffffcu);
}

static u8 *safe_code_cave(u8 *data,u32 length)
{
    static const u8 prefix_a[9]={0x80,0x1e,0,0,0x3c,0x60,0x80,0,0x83};
    static const u8 prefix_b[9]={0x80,0x1f,0,0,0x3c,0x60,0x80,0,0x83};
    for(u32 offset=0;offset+80u<=length;offset+=4) {
        if((equal(data+offset,prefix_a,9)||equal(data+offset,prefix_b,9)) &&
           *(u32*)(data+offset+36)==0x38000001)return data+offset+36;
    }
    return 0;
}

static void patch_480p(mfb_patch_report *report,const u32 config[6])
{
    static const u32 variant_a[6]={
        0x38000065,0x9b810019,0x38810018,0x386000e0,0x98010018,0x38a00002
    };
    static const u32 variant_b[6]={
        0x38000065,0x9801001c,0x3881001c,0x386000e0,0x9b81001d,0x38a00002
    };
    u8 *const base=(u8*)0x80000000;
    const u32 length=0x00900000;
    if(config[3]==0)return;
    u8 *site=0;
    u32 injected0=0,injected1=0;
    for(u32 offset=4;offset+36u<=length;offset+=4) {
        u8 *candidate=base+offset;
        const u32 before=*(u32*)(candidate-4), after=*(u32*)(candidate+32);
        if((before&0xffff0000u)!=0x4bff0000u ||
           (after&0xffff0000u)!=0x4bff0000u)continue;
        if(equal(candidate,variant_a,sizeof(variant_a))) {
            site=candidate+4; injected0=0x38600003; injected1=0x98610019; break;
        }
        if(equal(candidate,variant_b,sizeof(variant_b))) {
            site=candidate+16; injected0=0x38a00003; injected1=0x98a1001d; break;
        }
    }
    if(!site)return;
    u8 *safe=safe_code_cave(base,length);
    if(!safe)return;
    u8 *cave=safe+32;
    *(u32*)(cave+0)=injected0;
    *(u32*)(cave+4)=injected1;
    *(u32*)(cave+8)=branch((u32)cave+8,(u32)site+4);
    *(u32*)site=branch((u32)site,(u32)cave);
    mfb_dc_flush(cave,12); mfb_ic_invalidate(cave,12);
    mfb_dc_flush(site,4); mfb_ic_invalidate(site,4);
    report->pixel_480p_changed=1;
}

static void patch_runtime_width(mfb_patch_report *report,const u32 config[6])
{
    static const u32 signature[8]={
        0x40820008,0x4800001c,0x28090003,0x40820008,
        0x48000010,0x2c030000,0x40820008,0x54a50c3c
    };
    u8 *const base=(u8*)0x80000000;
    const u32 length=0x00900000;
    const u32 scale=config[0];
    if(scale==SCALE_AUTO)return;
    u8 *hook=0;
    u32 first=0,second=0;
    for(u32 offset=0x70;offset+sizeof(signature)<=length;offset+=4) {
        u8 *candidate=base+offset;
        if(!equal(candidate,signature,sizeof(signature)))continue;
        first=*(u32*)(candidate-0x70);
        second=*(u32*)(candidate-0x44);
        if((first&0xfc00ffffu)==0xa000000au &&
           (second&0xfc00ffffu)==0xa000000eu) {
            hook=candidate-0x70;
            break;
        }
    }
    if(!hook)return;
    u8 *safe=safe_code_cave(base,length);
    if(!safe)return;
    u32 *cave=(u32*)(safe+12);
    const u32 origin_reg=(first>>21)&31u;
    const u32 width_reg=(second>>21)&31u;
    if(scale==SCALE_1_TO_1) {
        cave[0]=(first&0xffff0000u)|4u;
        cave[1]=0x200002d0u|(width_reg<<21)|(origin_reg<<16);
        cave[2]=0x38000002u|(origin_reg<<21);
        cave[3]=0x7c000396u|(origin_reg<<21)|(width_reg<<16)|(origin_reg<<11);
        *(u32*)(hook+0x2c)=(second&0xffff0000u)|4u;
        cave[4]=branch((u32)&cave[4],(u32)hook+4);
        mfb_dc_flush(hook+0x2c,4); mfb_ic_invalidate(hook+0x2c,4);
        mfb_dc_flush(cave,20); mfb_ic_invalidate(cave,20);
    } else {
        const u16 width=selected_width(scale,640);
        if(width<480 || width>720)return;
        cave[0]=0x38000000u|(origin_reg<<21)|((720u-width)/2u);
        cave[1]=branch((u32)&cave[1],(u32)hook+4);
        *(u32*)(hook+0x2c)=0x38000000u|(width_reg<<21)|width;
        mfb_dc_flush(hook+0x2c,4); mfb_ic_invalidate(hook+0x2c,4);
        mfb_dc_flush(cave,8); mfb_ic_invalidate(cave,8);
    }
    *(u32*)hook=branch((u32)hook,(u32)cave);
    mfb_dc_flush(hook,4); mfb_ic_invalidate(hook,4);
    report->runtime_width_changed=1;
}

void mfb_patch_init(mfb_patch_report *report)
{
    mfb_memset(report,0,sizeof(*report));
}

void mfb_patch_section(mfb_patch_report *report,const u32 config[6],
                       void *address,u32 length)
{
    ++report->sections;
    patch_modes(report,config,address,length);
    patch_gx_filter(report,config,address,length);
    patch_aspect(report,config,address,length);
}

void mfb_patch_finalize(mfb_patch_report *report,const u32 config[6])
{
    patch_runtime_width(report,config);
    patch_480p(report,config);
}
