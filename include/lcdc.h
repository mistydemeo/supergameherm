#ifndef __LCDC_H_
#define __LCDC_H_

#include "config.h"	// macros, uint*_t


typedef struct _emu_state emu_state;

typedef struct _oam
{
	uint8_t y;
	uint8_t x;
	uint8_t chr;	/*! CHR code */
	struct
	{
		bool priority:1;	/*! Priority to obj/bg */
		bool vflip:1;		/*! Vertical flip flag */
		bool hflip:1;		/*! Horizontal flip */
		uint8_t pal_dmg:1;	/*! Palette selectin (DMG only) */
		uint8_t char_bak:1;	/*! Character bank (CGB only) */
		uint8_t pal_cgb:3;	/*! Palette selection CGB only) */
	} flags;
} oam;

typedef struct _cps
{
	bool pal_sel:1;		/*! Palette changes on next write */
	uint8_t notused:1;	/*! Not used */
	uint8_t pal_num:3;	/*! Select palette number */
	uint8_t pal_data_num:2;	/*! Select palette data number */
	bool hl:1;		/*! Specify H/L */
} cps;

typedef struct _lcdc_state
{
	uint32_t curr_clk;		/*! current clock */
	uint8_t vram_bank;		/*! Present VRAM bank */
	uint8_t vram[0x2][0x2000];	/*! VRAM banks (DMG only uses 1) */
	oam oam_store[40];		/*! OAM */
	uint8_t lcd_control;		/*! LCD control register */
	struct
	{
		uint8_t notused:1;	/*! Upper bit padding */
		bool lyc:1;		/*! int on LY matching selection */
		bool mode_10:1;		/*! int on mode 10 selection */
		bool mode_01:1;		/*! int on mode 01 selection */
		bool mode_00:1;		/*! int on mode 00 selection */
		bool lyc_state:1;	/*! LYC matches LY */

		/*! Mode flag
		 * Mode 00: enable CPU access to VRAM
		 * Mode 01: V-Blank interval
		 * Mode 10: OAM search
		 * Mode 11: LCD transfer
		 */
		uint8_t mode_flag:2;
	} stat;

	uint8_t scroll_y;	/*! Y position in scrolling map */
	uint8_t scroll_x;	/*! X position in scrolling map */

	uint8_t ly;		/*! Present line being transferred (144-153 = V-Blank) */
	uint8_t lyc;		/*! LY comparison (set stat.lyc_state when == ly) */

	union
	{
		/*! CGB only */
		struct
		{
			cps bcps;
			uint8_t bcpd;	/*! BG write data */

			cps ocps;
			uint8_t ocpd;	/*! OBJ write data */
		};

		/*! DMG only */
		struct
		{
			struct
			{
				uint8_t shade_1:2;
				uint8_t shade_2:2;
				uint8_t shade_3:2;
				uint8_t shade_4:2;
			} dmg_pal;

			struct
			{
				uint8_t shade_1:2;
				uint8_t shade_2:2;
				uint8_t shade_3:2;
				uint8_t notused:2;
			} obp[2];
		};
	};

	uint8_t window_y;	/*! Window Y coordinate (0 <= windowy <= 143) */
	uint8_t window_x;	/*! Window X coordinate (7 <= windowx <= 166) */
} lcdc;

#define LCDC_ON			0x80	/*! LCD active */
#define LCDC_WIN_CODE_SEL	0x40	/*! Active window at 9800-9BFF or 9C00-9FFF */
#define LCDC_WIN_ON		0x20	/*! Windowing on */
#define LCDC_BG_CHAR_SEL	0x10	/*! BG character data at 8800-97FF or 8000-8FFF */
#define LCDC_BG_CODE_SEL	0x08	/*! BG code data at 9800-9BFF or 9C00-9FFF */
#define LCDC_OBJ_BLOCK_SIZE	0x04	/*! Size of OBJ's are 8x8 or 8x16 */
#define LCDC_OBJ_ON		0x02	/*! OBJ's are off or on */
#define LCDC_DMG_BG_DISPLAY	0x01	/*! BG is on or off (DMG only) */


uint8_t lcdc_read(emu_state *, uint16_t);
void lcdc_write(emu_state *, uint16_t, uint8_t);
void lcdc_tick(emu_state *);

uint8_t bg_pal_ind_read(emu_state *restrict, uint16_t);
uint8_t bg_pal_data_read(emu_state *restrict, uint16_t);
uint8_t sprite_pal_ind_read(emu_state *restrict, uint16_t);
uint8_t sprite_pal_data_read(emu_state *restrict, uint16_t);
void bg_pal_ind_write(emu_state *restrict, uint16_t, uint8_t);
void bg_pal_data_write(emu_state *restrict, uint16_t, uint8_t);
void sprite_pal_ind_write(emu_state *restrict, uint16_t, uint8_t);
void sprite_pal_data_write(emu_state *restrict, uint16_t, uint8_t);

#endif /*!__LCDC_H_*/
