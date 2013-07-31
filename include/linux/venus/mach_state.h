/* arch/arm/mach-s3c6400/include/mach/mach_state.h
 *
 * jimmy add for mach state check
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _VENUS_MACH_STATE_H
#define _VENUS_MACH_STATE_H 

#define MACH_NORMAL		1
#define MACH_SUSPEND		2
#define MACH_POWEROFF		3

//extern int get_mach_state(void);
extern int mach_state;
#endif
