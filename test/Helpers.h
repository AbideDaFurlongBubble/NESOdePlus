#pragma once

#ifdef __HELPERS_H__
#error __HELPERS_H__ Already defined!
#else
#define __HELPERS_H__
#endif

#include "Emulator.h"
#include <assert.h>

void writeOpToMem(emu::Mapper& mm, OPCODE op, ADDR_16B start = L_PRGROM, int count = 1, int inc = 1);
void writePatternToMem(emu::Mapper& mm, OPCODE* patt, int pattsize, ADDR_16B start = L_PRGROM, int count = 1, int inc = 1);
int countOpTick(OPCODE* ins, BYTE const * const optable, int size);
void autoCC01Mapper(emu::Mapper& mm, emu::CPU& cpu, OPCODE op);