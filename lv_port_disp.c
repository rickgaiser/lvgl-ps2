#include <stdio.h>

#include <malloc.h>
#include <gsKit.h>
#include <dmaKit.h>

#include "lv_port_disp.h"

#define MY_DISP_HOR_RES    640
#define MY_DISP_VER_RES    480

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

static GSGLOBAL *gsGlobal;
static GSTEXTURE displayTex;
static lv_disp_draw_buf_t draw_buf_dsc;
static lv_disp_drv_t disp_drv;

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    gsGlobal = gsKit_init_global();

    gsGlobal->Mode = GS_MODE_DTV_480P;
    gsGlobal->Interlace = GS_NONINTERLACED;
    gsGlobal->Field = GS_FRAME;
    gsGlobal->Width = 640;
    gsGlobal->Height = 480;

    gsGlobal->PSM = GS_PSM_CT16S;
    gsGlobal->PSMZ = GS_PSMZ_16S;
    gsGlobal->Dithering = GS_SETTING_ON;
    gsGlobal->DoubleBuffering = GS_SETTING_OFF;
    gsGlobal->ZBuffering = GS_SETTING_OFF;

    displayTex.PSM = GS_PSM_CT32;
    displayTex.Filter = GS_FILTER_NEAREST;
    displayTex.Mem = memalign(128, MY_DISP_HOR_RES * MY_DISP_VER_RES * 4);

    dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
            D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gsGlobal);
    gsKit_vram_clear(gsGlobal);

    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/

    /**
     * LVGL requires a buffer where it internally draws the widgets.
     * Later this buffer will passed to your display driver's `flush_cb` to copy its content to your display.
     * The buffer has to be greater than 1 display row
     *
     * There are 3 buffering configurations:
     * 1. Create ONE buffer:
     *      LVGL will draw the display's content here and writes it to your display
     *
     * 2. Create TWO buffer:
     *      LVGL will draw the display's content to a buffer and writes it your display.
     *      You should use DMA to write the buffer's content to the display.
     *      It will enable LVGL to draw the next part of the screen to the other buffer while
     *      the data is being sent form the first buffer. It makes rendering and flushing parallel.
     *
     * 3. Double buffering
     *      Set 2 screens sized buffers and set disp_drv.full_refresh = 1.
     *      This way LVGL will always provide the whole rendered screen in `flush_cb`
     *      and you only need to change the frame buffer's address.
     */

    lv_color_t *buf_1 = memalign(128, MY_DISP_HOR_RES * MY_DISP_VER_RES * 4);
    //lv_color_t *buf_2 = memalign(128, MY_DISP_HOR_RES * MY_DISP_VER_RES * 4);
    lv_color_t *buf_2 = NULL;
    lv_disp_draw_buf_init(&draw_buf_dsc, buf_1, buf_2, MY_DISP_HOR_RES * MY_DISP_VER_RES);

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf_dsc;
    disp_drv.direct_mode = 0; // 1 = LVGL draws to full sized buffer
    disp_drv.full_refresh = 0;
    lv_disp_drv_register(&disp_drv);
}

#pragma GCC push_options
#pragma GCC optimize ("-O3")
static inline void convert_rgba_bgra(uint64_t *dst, uint64_t *src, int count)
{
    while(count > 0) {
        uint64_t s1 = src[0];
        uint64_t s2 = src[1];
        uint64_t t1, t2;

        asm inline (
            "pextlb %[t1], %[s2], %[s1]\n\t"      // Parallel Extend Lower from Byte       - 2x 64bit to 1x 128bit
            "pexeh  %[t1], %[t1]\n\t"             // Parallel Exchange Even Halfword       - swap 4x 'R' and 'B'
            "psrlh  %[t2], %[t1], 8\n\t"          // Parallel Shift Right Logical Halfword
            "ppacb  %[t1], %[t2], %[t1]\n\t"      // Parallel Pack to Byte
            "sq     %[t1], %[dst]\n\t"
             : [dst] "=m" (*dst), [t1] "=r" (t1), [t2] "=r" (t2) // outputs
             : [s1] "r" (s1), [s2] "r" (s2)                      // inputs
             :                                                   // clobbers
        );

        src += 2;
        dst += 2;
        count -= 4; // 4 pixels at a time
    }
}
#pragma GCC pop_options

/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
	static u64 TexCol = GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0x00);

    displayTex.Width = area->x2 - area->x1 + 1;
    displayTex.Height = area->y2 - area->y1 + 1;
    //printf("%d x %d\n", displayTex.Width, displayTex.Height);
    convert_rgba_bgra((void *)displayTex.Mem, (void *)color_p, displayTex.Width * displayTex.Height);
    gsKit_TexManager_invalidate(gsGlobal, &displayTex);
    gsKit_TexManager_bind(gsGlobal, &displayTex);

    if ((displayTex.Width * displayTex.Height) >= (8 * 64 * 64)) {
        // Draws faster in small stripes
        gsKit_prim_sprite_striped_texture(gsGlobal, &displayTex,
                                    (float)area->x1 - 0.5f, // X1
                                    (float)area->y1 - 0.5f, // Y2
                                    0.0f,                   // U1
                                    0.0f,                   // V1
                                    (float)area->x2 + 1.0f - 0.5f, // X2
                                    (float)area->y2 + 1.0f - 0.5f, // Y2
                                    displayTex.Width,       // U2
                                    displayTex.Height,      // V2
                                    2,
                                    TexCol);
    } else {
        // Draw single sprite
        gsKit_prim_sprite_texture(gsGlobal, &displayTex,
                                    (float)area->x1 - 0.5f, // X1
                                    (float)area->y1 - 0.5f, // Y2
                                    0.0f,                   // U1
                                    0.0f,                   // V1
                                    (float)area->x2 + 1.0f - 0.5f, // X2
                                    (float)area->y2 + 1.0f - 0.5f, // Y2
                                    displayTex.Width,       // U2
                                    displayTex.Height,      // V2
                                    2,
                                    TexCol);
    }
    gsKit_queue_exec(gsGlobal);
    //gsKit_sync_flip(gsGlobal);
    gsKit_TexManager_nextFrame(gsGlobal);

    // Inform the graphics library that you are ready with the flushing
    lv_disp_flush_ready(disp_drv);
}
