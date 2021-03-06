#ifndef __TIMER_H_
#define __TIMER_H_

#include "config.h"	// Various macros, uint[XX]_t
#include "typedefs.h"	// typedefs

#include <stdio.h>	// perror


struct timer_state_t
{
	uint8_t div;			/*! DIV register */
	uint8_t tima;			/*! TIMA register */
	uint8_t rounds;			/*! TMA register */
	uint16_t ticks_per_tima;	/*! ticks per TIMA++ */
	uint8_t curr_clk;		/*! ticks passed */
	bool enabled;			/*! timer armed */
};

typedef enum
{
	CPU_FREQ_DMG = 4194304,
	CPU_FREQ_SGB = 4295454,
	CPU_FREQ_CGB = 8388608,
} cpu_freq;


uint8_t timer_read(emu_state *restrict, uint16_t);
void timer_write(emu_state *restrict, uint16_t, uint8_t);
void timer_tick(emu_state *restrict);

#endif /*!__TIMER_H_*/
