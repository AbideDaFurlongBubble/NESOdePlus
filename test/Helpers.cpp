#include "Helpers.h"
#include "Test.h"
#include "Instructions.h"

/* Writes a single byte to memory <count> times. 
   Offset between bytes specified with <inc>. */
void writeOpToMem(emu::Mapper& mm, OPCODE op, ADDR_16B start, int count, int inc)
{
	assert(count > 0);
	assert(inc > 0);
	for (int offset = 0; offset < count * inc; offset += inc)
	{
		mm.writeMemory(start + offset, op);
	}
}

/* Write a byte pattern to mapper memory. */
void writePatternToMem(emu::Mapper& mm, OPCODE* patt, int pattsize, ADDR_16B start, int count, int inc)
{
	assert(count > 0);
	assert(inc > 0);
	assert(pattsize > 0);
	for (int offsetm = 0; offsetm < count * pattsize + (inc - 1) * (count - 1); offsetm += pattsize - 1 + inc)
	{
		for (int pattindx = 0; pattindx < pattsize; pattindx++)
		{
			mm.writeMemory(start + offsetm + pattindx, patt[pattindx]);
		}
	} 
}

/* Counts the number of opticks of single-byte ops in optable. */
int countOpTick(OPCODE* ins, BYTE const * const optable, int size) 
{
	assert(size > 0);
	int sum = 0;
	for (int c = 0; c < size; c++)
	{
		sum += optable[ins[c]];
	}
	return sum;
}

/* Writes test pattern to 0x8000 - 0x8012 as well as 
 * with test addresses for zero page (0x00 - 0x03).
 * data value is always 0x0F. */
void autoCC01Mapper(emu::Mapper& mm, emu::CPU& cpu, OPCODE op) {
	//Table CC: 01
	OPCODE CC01TBL[] = {
		//ZPage Indirect Indexed w/ X
		AMODE_ZPAGEINX,
		0x02, //X = 1

		//ZPage:
		AMODE_ZPAGE,
		0x01,

		//Immediate:
		AMODE_IMMED,
		0x0F,

		//Absolute:
		AMODE_ABS,
		0x01,
		0x00,

		//ZPage Indexed Indirect w/ Y:
		AMODE_ZPAGEINY,
		0x02, //Y = 1

		//ZPage Indexed w/ X:
		AMODE_ZPAGEX,
		0x00, //X = 1

		//Absolute Indexed w/ Y:
		AMODE_ABSY,
		0x00, //Y = 1
		0x00,

		//Absolute Indexed w/ X:
		AMODE_ABSX,
		0x00, //X = 1
		0x00
	};

	CC01TBL[0] |= op;
	CC01TBL[2] |= op;
	CC01TBL[4] |= op;
	CC01TBL[6] |= op;
	CC01TBL[9] |= op;
	CC01TBL[11] |= op;
	CC01TBL[13] |= op;
	CC01TBL[16] |= op;

	for (int i = 0; i < 19; i++)
	{
		writeOpToMem(mm, CC01TBL[i], 0x8000 + i);
	}

	//Byte to perform operation on for non-immediate addressing modes
	writeOpToMem(mm, 0x0F, 0x0000); //For indirect indexed X
	writeOpToMem(mm, 0x0F, 0x0001); //For indexed indirect Y

	//Indirect address
	writeOpToMem(mm, 0x00, 0x0002); //LSB
	writeOpToMem(mm, 0x00, 0x0003); //MSB
	writeOpToMem(mm, 0x00, 0x0004);

	//Setup for indexed addressing modes
	cpu.xindex.unsigned8 = 0x01;
	cpu.yindex.unsigned8 = 0x01;
}