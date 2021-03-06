#include "config.h"	// macros

#include "print.h"	// fatal
#include "ctl_unit.h"	// signal_interrupt
#include "util.h"	// likely/unlikely
#include "sgherm.h"	// emu_state


#define LCDC_BGWINDOW_SHOW	0x01
#define LCDC_OBJ_DISPLAY	0x02
#define LCDC_OBJ_LGSIZE		0x04
#define LCDC_BGTILE_MAP_HI	0x08
#define LCDC_BGWINDOW_TILE_LO	0x10
#define LCDC_WINDOW_SHOW	0x20
#define LCDC_WINTILE_MAP_HI	0x40
#define LCDC_ENABLE		0x80

void init_lcdc(emu_state *restrict state)
{
	state->lcdc.lcd_control.params.enable = true;
	state->lcdc.lcd_control.params.bg_char_sel = true;
	state->lcdc.lcd_control.params.dmg_bg = true;

	state->lcdc.stat.params.mode_flag = 2;

	state->lcdc.ly = 0;
	state->lcdc.lyc = 0;
}

void lcdc_tick(emu_state *restrict state)
{
	if(unlikely(state->stop) ||
	   unlikely(!state->lcdc.lcd_control.params.enable))
	{
		return;
	}

	state->lcdc.curr_clk++;

	switch(state->lcdc.stat.params.mode_flag)
	{
	case 2:
		/* first mode - reading OAM for h scan line */
		if(state->lcdc.curr_clk >= 80)
		{
			state->lcdc.curr_clk = 0;
			state->lcdc.stat.params.mode_flag = 3;
		}
		break;
	case 3:
		/* second mode - reading VRAM for h scan line */
		if(state->lcdc.curr_clk >= 172)
		{
			state->lcdc.curr_clk = 0;
			state->lcdc.stat.params.mode_flag = 0;
		}
		break;
	case 0:
		/* third mode - h-blank */
		if(state->lcdc.curr_clk >= 204)
		{
			uint16_t next_tile = 0x1800;
			uint8_t skip = 0, curr_tile = 0;
			uint16_t start = (state->lcdc.lcd_control.params.bg_char_sel) ? 0x0 : 0x800;
			uint8_t pixel_y_offset = state->lcdc.ly % 8;
			uint32_t val[4] = { 0x009CBD0F, 0x008CAD0F, 0x00306230, 0x000F380F };
			static const uint16_t letter_a[8] = { 0x7C7C, 0x00C6, 0xC600, 0x00FE, 0xC6C6, 0x00C6, 0xC600, 0x0000 };

			// Silence GCC
			(void)letter_a;

			if (state->lcdc.lcd_control.params.bg_code_sel)
			{
				next_tile += 0x400;
			}
			next_tile += (state->lcdc.ly >> 3) << 5;

			for (; curr_tile < 20; curr_tile++, next_tile++, skip += 8)
			{
				uint8_t tile = state->lcdc.vram[0x0][next_tile];
				uint8_t pixel_temp;
				uint16_t *mem;
				if (!state->lcdc.lcd_control.params.bg_char_sel)
				{
					tile -= 0x80;
				}
				mem = (uint16_t *)(state->lcdc.vram[0x0] + start + (tile * 16) + (pixel_y_offset * 2));

				pixel_temp = ((*mem & 0x01) << 1) | (*mem & 0x100 >> 8);
				state->lcdc.out[state->lcdc.ly][skip + 7] = val[pixel_temp];
				pixel_temp = ((*mem & 0x02)) | ((*mem & 0x200) >> 9);
				state->lcdc.out[state->lcdc.ly][skip + 6] = val[pixel_temp];
				pixel_temp = ((*mem & 0x04) >> 1) | ((*mem & 0x400) >> 10);
				state->lcdc.out[state->lcdc.ly][skip + 5] = val[pixel_temp];
				pixel_temp = ((*mem & 0x08) >> 2) | ((*mem & 0x800) >> 11);
				state->lcdc.out[state->lcdc.ly][skip + 4] = val[pixel_temp];
				pixel_temp = ((*mem & 0x10) >> 3) | ((*mem & 0x1000) >> 12);
				state->lcdc.out[state->lcdc.ly][skip + 3] = val[pixel_temp];
				pixel_temp = ((*mem & 0x20) >> 4) | ((*mem & 0x2000) >> 13);
				state->lcdc.out[state->lcdc.ly][skip + 2] = val[pixel_temp];
				pixel_temp = ((*mem & 0x40) >> 5) | ((*mem & 0x4000) >> 14);
				state->lcdc.out[state->lcdc.ly][skip + 1] = val[pixel_temp];
				pixel_temp = ((*mem & 0x80) >> 6) | ((*mem & 0x8000) >> 15);
				state->lcdc.out[state->lcdc.ly][skip] = val[pixel_temp];
			}

			state->lcdc.curr_clk = 0;
			if((++state->lcdc.ly) == 144)
			{
				/* going to v-blank */
				state->lcdc.stat.params.mode_flag = 1;
			}
			else
			{
				/* start another scan line */
				state->lcdc.stat.params.mode_flag = 2;
			}
		}
		break;
	case 1:
		/* v-blank */
		if(state->lcdc.ly == 144 &&
		   state->lcdc.curr_clk == 1)
		{
			// Fire the vblank interrupt
			signal_interrupt(state, INT_VBLANK);

			// Blit
			BLIT_CANVAS(state);
		}

		if(state->lcdc.curr_clk % 456 == 0)
		{
			state->lcdc.ly++;
		};

		if(state->lcdc.ly == 153)
		{
			state->lcdc.curr_clk = 0;
			state->lcdc.ly = 0;
			state->lcdc.stat.params.mode_flag = 2;
		}

		break;
	default:
		fatal(state, "somehow wound up in an unknown impossible video mode");
	}

	if(state->lcdc.ly == state->lcdc.lyc)
	{
		state->lcdc.stat.params.lyc_state = true;
		if(state->lcdc.stat.params.lyc)
		{
			signal_interrupt(state, INT_LCD_STAT);
		}
	}
	else
	{
		state->lcdc.stat.params.lyc_state = false;
	}
}

inline uint8_t lcdc_read(emu_state *restrict state, uint16_t reg)
{
	error(state, "lcdc: unknown register %04X (R)", reg);
	return 0xFF;
}

inline uint8_t vram_read(emu_state *restrict state, uint16_t reg)
{
	uint8_t curr_mode = state->lcdc.stat.params.mode_flag;
	uint8_t bank = state->lcdc.vram_bank;
	if(curr_mode > 2)
	{
		// Game freak write shitty code and write to VRAM anyway.
		// Pokémon RGB break if we fatal here.
		warning(state, "read from VRAM while not in h/v-blank");
		return 0xFF;
	}

	return state->lcdc.vram[bank][reg - 0x8000];
}

inline uint8_t lcdc_control_read(emu_state *restrict state, uint16_t reg UNUSED)
{
	return state->lcdc.lcd_control.reg;
}

inline uint8_t lcdc_stat_read(emu_state *restrict state, uint16_t reg UNUSED)
{
	return state->lcdc.stat.reg;
}

inline uint8_t lcdc_scroll_read(emu_state *restrict state, uint16_t reg)
{
	if(reg == 0xFF42)
	{
		return state->lcdc.scroll_y;
	}
	else if(reg == 0xFF43)
	{
		return state->lcdc.scroll_x;
	}
	else
	{
		fatal(state, "BUG: attempt to read scroll stuff from non-scroll reg");
		return 0xFF;
	}
}

inline uint8_t lcdc_ly_read(emu_state *restrict state, uint16_t reg UNUSED)
{
	return state->lcdc.ly;
}

inline uint8_t lcdc_lyc_read(emu_state *restrict state, uint16_t reg UNUSED)
{
	return state->lcdc.lyc;
}

inline uint8_t lcdc_window_read(emu_state *restrict state, uint16_t reg)
{
	if(reg == 0xFF4A)
	{
		return state->lcdc.window_y;
	}
	else if(reg == 0xFF4B)
	{
		return state->lcdc.window_x;
	}
	else
	{
		fatal(state, "BUG: Attempt to read window stuff from non-window reg");
		return 0xFF;
	}
}

inline uint8_t bg_pal_ind_read(emu_state *restrict state, uint16_t reg)
{
	if(state->system != SYSTEM_CGB)
	{
		return no_hardware(state, reg);
	}

	// TODO
	return state->memory[reg];
}

inline uint8_t bg_pal_data_read(emu_state *restrict state, uint16_t reg)
{
	if(state->system != SYSTEM_CGB)
	{
		return no_hardware(state, reg);
	}

	// TODO
	return state->memory[reg];
}

inline uint8_t sprite_pal_ind_read(emu_state *restrict state, uint16_t reg)
{
	if(state->system != SYSTEM_CGB)
	{
		return no_hardware(state, reg);
	}

	// TODO
	return state->memory[reg];
}

inline uint8_t sprite_pal_data_read(emu_state *restrict state, uint16_t reg)
{
	if(state->system != SYSTEM_CGB)
	{
		return no_hardware(state, reg);
	}

	// TODO
	return state->memory[reg];
}

void dump_lcdc_state(emu_state *restrict state)
{
	debug(state, "---LCDC---");
	debug(state, "CTRL: %02X (%s %s %s %s %s %s %s %s)",
	      state->lcdc.lcd_control.reg,
	      (state->lcdc.lcd_control.params.dmg_bg) ? "BG" : "bg",
	      (state->lcdc.lcd_control.params.obj) ? "OBJ" : "obj",
	      (state->lcdc.lcd_control.params.obj_block_size) ? "8x16" : "8x8",
	      (state->lcdc.lcd_control.params.bg_code_sel) ? "9C" : "98",
	      (state->lcdc.lcd_control.params.bg_char_sel) ? "sign" : "SIGN",
	      (state->lcdc.lcd_control.params.win) ? "WIN" : "win",
	      (state->lcdc.lcd_control.params.win_code_sel) ? "9C" : "98",
	      (state->lcdc.lcd_control.params.enable) ? "E" : "e"
	);
	debug(state, "STAT: %02X (MODE=%d)",
	      state->lcdc.stat.reg, state->lcdc.stat.params.mode_flag);
	debug(state, "ICLK: %02X", state->lcdc.curr_clk);
	debug(state, "LY  : %02X", state->lcdc.ly);
}

inline void lcdc_write(emu_state *restrict state, uint16_t reg, uint8_t data UNUSED)
{
	error(state, "lcdc: unknown register %04X (W)", reg);
}

inline void vram_write(emu_state *restrict state, uint16_t reg, uint8_t data)
{
	uint8_t curr_mode = state->lcdc.stat.params.mode_flag;
	uint8_t bank = state->lcdc.vram_bank;
	if(curr_mode > 2)
	{
		// Game freak write shitty code and write to VRAM anyway.
		// Pokémon RGB break if we fatal here.
		warning(state, "write to VRAM while not in h/v-blank");
		return;
	}

	state->lcdc.vram[bank][reg - 0x8000] = data;
}

inline void lcdc_control_write(emu_state *restrict state, uint16_t reg UNUSED, uint8_t data)
{
	state->lcdc.lcd_control.reg = data;
}

inline void lcdc_stat_write(emu_state *restrict state, uint16_t reg UNUSED, uint8_t data)
{
	state->lcdc.stat.params.lyc = ((data & 0x60) == 0x60);
}

inline void lcdc_scroll_write(emu_state *restrict state, uint16_t reg, uint8_t data)
{
	if(reg == 0xFF42)
	{
		state->lcdc.scroll_y = data;
	}
	else if(reg == 0xFF43)
	{
		state->lcdc.scroll_x = data;
	}
	else
	{
		fatal(state, "BUG: attempt to write scroll stuff to non-scroll reg");
	}
}

inline void lcdc_ly_write(emu_state *restrict state, uint16_t reg UNUSED, uint8_t data UNUSED)
{
#ifndef NDEBUG
	fatal(state, "write to LY (FF44); you can't just vsync yourself!");
#else
	error(state, "write to LY (FF44) is being ignored");
#endif
}

inline void lcdc_lyc_write(emu_state *restrict state, uint16_t reg UNUSED, uint8_t data)
{
	state->lcdc.lyc = data;
}

inline void lcdc_window_write(emu_state *restrict state, uint16_t reg, uint8_t data)
{
	if(reg == 0xFF4A)
	{
		state->lcdc.window_y = data;
	}
	else if(reg == 0xFF4B)
	{
		state->lcdc.window_x = data;
	}
	else
	{
		fatal(state, "BUG: Attempt to write window data to non-window register");
	}
}

void bg_pal_ind_write(emu_state *restrict state, uint16_t reg, uint8_t data)
{
	if(state->system != SYSTEM_CGB)
	{
		doofus_write(state, reg, data);
		return;
	}

	// TODO
	state->memory[reg] = data;
}

void bg_pal_data_write(emu_state *restrict state, uint16_t reg, uint8_t data)
{
	if(state->system != SYSTEM_CGB)
	{
		doofus_write(state, reg, data);
		return;
	}

	// TODO
	state->memory[reg] = data;
}

void sprite_pal_ind_write(emu_state *restrict state, uint16_t reg, uint8_t data)
{
	if(state->system != SYSTEM_CGB)
	{
		doofus_write(state, reg, data);
		return;
	}

	// TODO
	state->memory[reg] = data;
}

void sprite_pal_data_write(emu_state *restrict state, uint16_t reg, uint8_t data)
{
	if(state->system != SYSTEM_CGB)
	{
		doofus_write(state, reg, data);
		return;
	}

	// TODO
	state->memory[reg] = data;
}

void magical_mystery_cure(void)
{
	lcdc_read(NULL, 0);
	lcdc_control_read(NULL, 0);
	lcdc_stat_read(NULL, 0);
	lcdc_scroll_read(NULL, 0);
	lcdc_ly_read(NULL, 0);
	lcdc_lyc_read(NULL, 0);
	lcdc_window_read(NULL, 0);
	bg_pal_ind_read(NULL, 0);
	bg_pal_data_read(NULL, 0);
	sprite_pal_ind_read(NULL, 0);
	sprite_pal_data_read(NULL, 0);
	vram_read(NULL, 0);
	lcdc_write(NULL, 0, 0);
	lcdc_control_write(NULL, 0, 0);
	lcdc_stat_write(NULL, 0, 0);
	lcdc_scroll_write(NULL, 0, 0);
	lcdc_ly_write(NULL, 0, 0);
	lcdc_lyc_write(NULL, 0, 0);
	lcdc_window_write(NULL, 0, 0);
	vram_write(NULL, 0, 0);
}
