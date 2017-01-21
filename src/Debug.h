#pragma once

#ifdef __DEBUG_H__
#error __DEBUG_H__ Already defined!
#else
#define __DEBUG_H__
#endif

/* STRICT enforcement increases the fidelity of the emulator at the cost of speed.
 * MINIMUM enforcement is recommended but not required for a correctly programmed ROM. At the
 * very least make sure ENFORCE_READONLY_MINIMUM is defined to ensure that ROM space is not written to.
 */

//Ensure integer wrap. Useful for architectures with different wrap mechanisms from 6502.
//#define ENFORCE_OVRFLOWSEMANTICS

//Ensure data values are written to all stack mirrors.
#define ENFORCE_STACK_MIRROR

//Ensure no trainer is used in ROM.
#define ENFORCE_NOTRAINER

//Enforce no read only ranges.
#define ENFORCE_READONLY_NONE

//Enforce no write only ranges.
#define ENFORCE_WRITEONLY_NONE
