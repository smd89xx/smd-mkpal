#include <genesis.h>
#include "resources.h"

#define PALETTE_MAX 256

#define INPUTMODE_MODIFY    0
#define INPUTMODE_SWAPSET   1

#define TILE2PX(x)  (x) << 3

typedef struct
{
    s8 x,y;
}Vect2D_s8;

const Vect2D_s8 basepos_line = {0,0};
const u16 basetile_cursor = TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,1);
u8 pal_idx = 0;
u8 line_idx = PAL0;
u8 clr_idx = 0;
u8 input_mode = INPUTMODE_MODIFY;
u16 palette[64];

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

static void fmt_sram()
{
    PAL_getColors(0,palette,64);
    SRAM_enable();
    for (u16 i = 0; i < PALETTE_MAX; i++)
    {
        for (u8 j = 0; j < 64; j++)
        {
            SRAM_writeWord((i*128)+(j*sizeof(u16)),palette[j]);
        }
    }
    SRAM_disable();
    VDP_clearTextLine(27);
    VDP_drawText("SRAM cleared.",0,27);
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
    VDP_clearTextArea(0,26,40,2);
    VDP_drawText("Palette saved to selected slot",0,26);
    char mem_str[24];
    sprintf(mem_str,"and memory offset $%04X",(u32)palette&0x0000FFFF);
    VDP_drawText(mem_str,0,27);
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
}

static void handle_input(u16 joy, u16 changed, u16 state)
{
    switch (input_mode)
    {
        default:
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
                if (state & BUTTON_A)
                {
                    save_slot();
                    load_slot(-1);
                    return;
                }
                move_cursor((Vect2D_s8){-1,0}); 
            }
            else if (changed & state & BUTTON_RIGHT)
            {
                if (state & BUTTON_A)
                {
                    save_slot();
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
            break;
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
        SYS_doVBlankProcess();
    }
    return (0);
}