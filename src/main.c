#include <genesis.h>
#include "resources.h"

#define PALETTE_MAX 256

#define MSG_FMTD        0
#define MSG_SAVED       1
#define MSG_PREVSAVED   2

#define INPUTMODE_MODIFY    0
#define INPUTMODE_SWAPSET   1

#define SEC2FRAME(x)    (x) * (60 - (10 * IS_PAL_SYSTEM))

typedef struct
{
    s8 x,y;
}Vect2D_s8;

const Vect2D_s8 basepos_line = {0,0};
const u16 basetile_cursor = TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,1);
u8 pal_idx = 0;
u8 line_idx = PAL0;
u8 clr_idx = 0;
u16 palette[64];
u8 timer = 0;
bool run_timer = false;
const char* msgs[] = {"SRAM cleared.", "Current palette saved.","Previous palette saved."};

static void move_cursor(Vect2D_s8 delta)
{
    clr_idx += delta.x;
    clr_idx %= 16;
    line_idx += delta.y;
    line_idx %= 4;
    VDP_clearPlane(BG_B,TRUE);
    VDP_drawImageEx(BG_B,&img_cursor,basetile_cursor,basepos_line.x+clr_idx+2,(line_idx<<1)+basepos_line.y+1,FALSE,TRUE);
    char clr_hdr[27];
    sprintf(clr_hdr,"Current Hex Value: $%04X",PAL_getColor((line_idx<<4)+clr_idx));
    VDP_drawText(clr_hdr,basepos_line.x,basepos_line.y+10);
    VDP_setBackgroundColor(line_idx << 4);
}

static void mod_cram(s8 dr, s8 dg, s8 db)
{
    u16 basecolor = PAL_getColor((line_idx<<4)+clr_idx);
    u8 br = ((basecolor & 0x000F)+dr)&0x0F; u8 bg = (((basecolor & 0x00F0) >> 4)+dg)&0x0F; u8 bb = (((basecolor & 0x0F00) >> 8)+db)&0x0F;
    basecolor = 0;
    basecolor += (bb<<8)|(bg<<4)|br;
    PAL_setColor((line_idx<<4)+clr_idx,basecolor);
    move_cursor((Vect2D_s8){0,0});
}

static void reset_timer()
{
    run_timer = true; timer = SEC2FRAME(1.5f);
}

static void draw_msg(u8 idx,...)
{
    const char* msgptr = msgs[idx];
    va_list args;
    va_start(args,msgptr);
    char fmtd[41];
    vsprintf(fmtd,msgptr,args);
    va_end(args);
    VDP_clearTextLine(27);
    VDP_drawText(fmtd,0,27);
    reset_timer();
}

static void load_slot(s8 delta)
{
    pal_idx += delta;
    SRAM_enable();
    for (u8 i = 0; i < sizeof(palette)/sizeof(u16); i++)
    {
        palette[i] = SRAM_readWord((pal_idx*sizeof(palette))+(i<<1));
    }
    SRAM_disable();
    PAL_setColors(0,palette,64,DMA);
    char slot_hdr[19];
    sprintf(slot_hdr,"Using Slot %03u/%03u",pal_idx+1,PALETTE_MAX);
    VDP_drawText(slot_hdr,basepos_line.x,basepos_line.y);
    move_cursor((Vect2D_s8){0,0});
}

static void fmt_sram()
{
    memcpy(palette,pal_default,sizeof(palette));
    SRAM_enable();
    for (u16 i = 0; i < PALETTE_MAX; i++)
    {
        for (u8 j = 0; j < 64; j++)
        {
            SRAM_writeWord((i*128)+(j*sizeof(u16)),palette[j]);
        }
    }
    SRAM_disable();
    load_slot(0);
    draw_msg(MSG_FMTD);
}

static void save_slot()
{
    PAL_getColors(0,palette,sizeof(palette)/sizeof(u16));
    SRAM_enable();
    for (u8 i = 0; i < sizeof(palette)/sizeof(u16); i++)
    {
        SRAM_writeWord((pal_idx*sizeof(palette))+(i<<1),palette[i]);
    }
    SRAM_disable();
    draw_msg(MSG_SAVED);
}

static void handle_input(u16 joy, u16 changed, u16 state)
{
            if (changed & state & BUTTON_DOWN)
            {
                if (!(state & (BUTTON_A | BUTTON_B | BUTTON_C)))
                {
                    move_cursor((Vect2D_s8){0,1});
                    return;
                }
                if (state & BUTTON_A)
                {
                    mod_cram(-0x02,0x00,0x00);
                }
                if (state & BUTTON_B)
                {
                    mod_cram(0x00,-0x02,0x00);
                }
                if (state & BUTTON_C)
                {
                    mod_cram(0x00,0x00,-0x02);
                }
            }
            else if (changed & state & BUTTON_UP)
            {
                if (!(state & (BUTTON_A | BUTTON_B | BUTTON_C)))
                {
                    move_cursor((Vect2D_s8){0,-1});
                    return;
                }
                if (state & BUTTON_A)
                {
                    mod_cram(0x02,0x00,0x00);
                }
                if (state & BUTTON_B)
                {
                    mod_cram(0x00,0x02,0x00);
                }
                if (state & BUTTON_C)
                {
                    mod_cram(0x00,0x00,0x02);
                }
            }
            if (changed & state & BUTTON_LEFT)
            {
                if (state & BUTTON_START)
                {
                    save_slot();
                    draw_msg(MSG_PREVSAVED);
                    load_slot(-1);
                    return;
                }
                move_cursor((Vect2D_s8){-1,0}); 
            }
            else if (changed & state & BUTTON_RIGHT)
            {
                if (state & BUTTON_START)
                {
                    save_slot();
                    draw_msg(MSG_PREVSAVED);
                    load_slot(1);
                    return;
                }
                move_cursor((Vect2D_s8){1,0});
            }
            if ((changed & state & BUTTON_START))
            {
                if (((state & BUTTON_A)&&(state & BUTTON_B)&&(state & BUTTON_C))){fmt_sram();}
                else if (state & BUTTON_A){save_slot();}
            }
}

int main()
{
    for (u8 i = 0; i < 4; i++)
    {
        VDP_drawImageEx(BG_A,&img_line,(i<<13)+2,basepos_line.x+2,basepos_line.y+(i<<1)+2,FALSE,TRUE);
        char idx_string[2];
        intToStr(i,idx_string,1);
        VDP_drawText(idx_string,basepos_line.x,basepos_line.y+(i<<1)+2);
    }
    load_slot(0);
    move_cursor((Vect2D_s8){0,0});
    JOY_setEventHandler(&handle_input);
    while(1)
    {
        if (run_timer)
        {
            timer--;
            if (timer == 0)
            {
                VDP_clearTextArea(0,26,screenWidth >> 3,2);
                run_timer = false;
            }
        }
        SYS_doVBlankProcess();
    }
    return (0);
}