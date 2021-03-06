#ifndef __DEBUG_H_
#define __DEBUG_H_

#include "config.h"	// Various macros
#include "typedefs.h"	// typedefs
#include "ctl_unit.h"	// flags

extern const char * const mnemonics[0x100];
extern const char * const mnemonics_cb[0x100];
extern const char * const flags_expect[0x100];
extern const char * const flags_cb_expect[0x100];

void print_cpu_state(emu_state *restrict);
void print_cycles(emu_state *restrict);
void print_flags(emu_state *restrict);
void dump_all_state(emu_state *restrict);

#endif /*!__DEBUG_H_*/
