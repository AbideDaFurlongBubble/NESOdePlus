#include "Emulator.h"
#include "Instructions.h"
using namespace emu;

/*Array of ticks corresponding to each OPCODE. Some opcodes have variable tick counts up to +2
but this set ignores ticks added due to crossing pages and assumes +1 when conditional branching
can occur. If there are ticks with 1, they are undocumented bytecodes that I could not locate
information on (exception: KIL ops are 1 because they lock the processor. These ops are 0x02,
0x12, 0x22, 0x32, 0x42, 0x52, 0x62, 0x72, 0x92, 0xB2, 0xD2, 0xF2).*/
const BYTE emu::OPTICK[] = {								//  OPCODES
	7, 6, 1, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6, //0x00 - 0x0F
	3, 5, 1, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, //0x10 - 0x1F
	6, 6, 1, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6, //0x20 - 0x2F
	3, 5, 1, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, //0x30 - 0x3F
	6, 6, 1, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6, //0x40 - 0x4F
	3, 5, 1, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, //0x50 - 0x5F
	6, 6, 1, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6, //0x60 - 0x6F
	3, 5, 1, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, //0x70 - 0x7F
	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, //0x80 - 0x8F
	3, 6, 1, 6, 4, 4, 4, 4, 2, 5, 2, 2, 5, 5, 5, 5, //0x90 - 0x9F
	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, //0xA0 - 0xAF
	3, 5, 1, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4, //0xB0 - 0xBF
	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, //0xC0 - 0xCF
	3, 5, 1, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, //0xD0 - 0xDF
	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, //0xE0 - 0xEF
	3, 5, 1, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7  //0xF0 - 0xFF
};

/* Initializes the CPU to sensible defaults. Just over-ride any you don't like in post. */
void emu::initializeCPU(CPU& cpu) {
	cpu.accumulator.unsigned8 = 0x00;
	cpu.xindex.unsigned8 = 0x00;
	cpu.yindex.unsigned8 = 0x00;
	cpu.stackp.unsigned8 = 0xFF;
	cpu.temp.unsigned8 = 0x00;
	cpu.debug.unsigned8 = 0x00;
	cpu.procstat = 0x00;
	cpu.progcount = L_PRGROM;
}

//======================================================
//Emulator Helpers
//======================================================

//Read a 2-byte value from memory
#define READ_WORD(MAPPER,ADDR) \
	MAPPER.readMemory(ADDR) | (MAPPER.readMemory(ADDR + 1) << 8)

//Memory GETs ==========================================

MEM_BYTE GET_IMMEDIATE(Mapper& mapper, CPU& cpu)
{
	MEM_BYTE byte;
	byte.unsigned8 = mapper.readMemory(cpu.progcount);
	++cpu.progcount;
	return byte;
}

MEM_BYTE GET_ZPAGE(Mapper& mapper, CPU& cpu)
{
	BYTE zpage = mapper.readMemory(cpu.progcount);
	MEM_BYTE byte;
	byte.unsigned8 = mapper.readMemory(zpage);
	++cpu.progcount;
	return byte;
}

MEM_BYTE GET_ZPAGEX(Mapper& mapper, CPU& cpu)
{
	BYTE zpage = mapper.readMemory(cpu.progcount);
	zpage += cpu.xindex.unsigned8;
	MEM_BYTE byte;
	byte.unsigned8 = mapper.readMemory(zpage);
	++cpu.progcount;
	return byte;
}

MEM_BYTE GET_ZPAGEY(Mapper& mapper, CPU& cpu)
{
	BYTE zpage = mapper.readMemory(cpu.progcount);
	zpage += cpu.yindex.unsigned8;
	MEM_BYTE byte;
	byte.unsigned8 = mapper.readMemory(zpage);
	++cpu.progcount;
	return byte;
}

MEM_BYTE GET_ABSOLUTE(Mapper& mapper, CPU& cpu)
{
	ADDR_16B abs = READ_WORD(mapper, cpu.progcount);
	MEM_BYTE byte;
	byte.unsigned8 = mapper.readMemory(abs);
	cpu.progcount += 2;
	return byte;
}


MEM_BYTE GET_ABSOLUTEX(Mapper& mapper, CPU& cpu)
{
	ADDR_16B abs = READ_WORD(mapper, cpu.progcount);
	abs += cpu.xindex.unsigned8;
	MEM_BYTE byte;
	byte.unsigned8 = mapper.readMemory(abs);
	cpu.progcount += 2;
	return byte;
}

MEM_BYTE GET_ABSOLUTEY(Mapper& mapper, CPU& cpu)
{
	ADDR_16B abs = READ_WORD(mapper, cpu.progcount);
	abs += cpu.yindex.unsigned8;
	MEM_BYTE byte;
	byte.unsigned8 = mapper.readMemory(abs);
	cpu.progcount += 2;
	return byte;
}

MEM_BYTE GET_INDXINDIRECT(Mapper& mapper, CPU& cpu)
{
	BYTE zpageaddr = mapper.readMemory(cpu.progcount);
	zpageaddr += cpu.xindex.unsigned8;
	ADDR_16B indirect = READ_WORD(mapper, zpageaddr);
	MEM_BYTE byte;
	byte.unsigned8 = mapper.readMemory(indirect);
	++cpu.progcount;
	return byte;
}

MEM_BYTE GET_INDIRECTINDXY(Mapper& mapper, CPU& cpu)
{
	BYTE zpageaddr = mapper.readMemory(cpu.progcount);
	ADDR_16B indirect = READ_WORD(mapper, zpageaddr);
	indirect += cpu.yindex.unsigned8;
	MEM_BYTE byte;
	byte.unsigned8 = mapper.readMemory(indirect);
	++cpu.progcount;
	return byte;
}

//Memory SETs ==========================================

void SET_ZPAGE(Mapper& mapper, CPU& cpu, BYTE val)
{
	ADDR_16B zpage = GET_IMMEDIATE(mapper, cpu).unsigned8;
	mapper.writeMemory(zpage, val);
}

void SET_ABSOLUTE(Mapper& mapper, CPU& cpu, BYTE val)
{
	ADDR_16B addr = READ_WORD(mapper, cpu.progcount);
	mapper.writeMemory(addr, val);
	cpu.progcount += 2;
}

void SET_ABSOLUTEX(Mapper& mapper, CPU& cpu, BYTE val)
{
	ADDR_16B addr = READ_WORD(mapper, cpu.progcount);
	addr += cpu.xindex.unsigned8;
	mapper.writeMemory(addr, val);
	cpu.progcount += 2;
}

void SET_ABSOLUTEY(Mapper& mapper, CPU& cpu, BYTE val)
{
	ADDR_16B addr = READ_WORD(mapper, cpu.progcount);
	addr += cpu.yindex.unsigned8;
	mapper.writeMemory(addr, val);
	cpu.progcount += 2;
}

void SET_ZPAGEX(Mapper& mapper, CPU& cpu, BYTE val)
{
	ADDR_16B zpage = GET_IMMEDIATE(mapper, cpu).unsigned8;
	zpage += cpu.xindex.unsigned8;
	mapper.writeMemory(zpage, val);
}

void SET_ZPAGEY(Mapper& mapper, CPU& cpu, BYTE val)
{
	ADDR_16B zpage = GET_IMMEDIATE(mapper, cpu).unsigned8;
	zpage += cpu.yindex.unsigned8;
	mapper.writeMemory(zpage, val);
}

void SET_INDXINDIRECT(Mapper& mapper, CPU& cpu, BYTE val)
{
	BYTE zpageaddr = mapper.readMemory(cpu.progcount);
	zpageaddr += cpu.xindex.unsigned8;
	ADDR_16B indirect = READ_WORD(mapper, zpageaddr);
	mapper.writeMemory(indirect, val);
	++cpu.progcount;
}

void SET_ZPAGEINY(Mapper& mapper, CPU& cpu, BYTE val)
{
	BYTE zpageaddr = mapper.readMemory(cpu.progcount);
	ADDR_16B indirect = READ_WORD(mapper, zpageaddr);
	indirect += cpu.yindex.unsigned8;
	mapper.writeMemory(indirect, val);
	++cpu.progcount;
}

//Helper Functions =====================================

/* Operations to perform. */
namespace operations {
	enum OperationT { BIT_OR, BIT_XOR, BIT_AND, ADD, SUB, EQL, CMP, ASL, ROL };
}

/* Perform an operation between an operand and a register. */
void perform_operation(CPU& cpu, REG_S8B& reg, MEM_BYTE rvalue, operations::OperationT operation)
{
	using namespace operations;
#ifdef ENFORCE_OVRFLOWSEMANTICS
	int16_t tval;
#endif

	int8_t add_result;
	const uint8_t CARRY_UNSET = 2;
	uint8_t carry = CARRY_UNSET;

	switch (operation)
	{
	case BIT_OR:
		reg.unsigned8 |= rvalue.unsigned8;
		//Negative Flag
		SETF_NEG(cpu.procstat, reg.signed8 < 0);
		//Zero Flag
		SETF_ZERO(cpu.procstat, reg.unsigned8 == 0);
		break;

	case BIT_XOR:
		reg.unsigned8 ^= rvalue.unsigned8;
		//Negative Flag
		SETF_NEG(cpu.procstat, reg.signed8 < 0);
		//Zero Flag
		SETF_ZERO(cpu.procstat, reg.unsigned8 == 0);
		break;

	case BIT_AND:
		reg.unsigned8 &= rvalue.unsigned8;
		//Negative Flag
		SETF_NEG(cpu.procstat, reg.signed8 < 0);
		//Zero Flag
		SETF_ZERO(cpu.procstat, reg.unsigned8 == 0);
		break;

	case SUB:
		//$STUB$ add enforce_ovrflowsemantics w/ alternate for carry *= -1 and rvalue *= -1
		carry = (~F_CARRY(cpu.procstat)) + 1; //Two's complement
		rvalue.unsigned8 = (~rvalue.unsigned8) + 1;
	case ADD:
#ifdef ENFORCE_OVRFLOWSEMANTICS  //Generic implementation $STUB$ revisit and ensure valid
		if (carry == CARRY_UNSET)
			carry = F_CARRY(cpu.procstat);

		tval = reg.signed8;
		tval += rvalue.signed8;
		tval += carry;

		if (tval > 127) {
			SETF_OVRFLOW(cpu.procstat, 1);
			if (reg.unsigned8 + rvalue.unsigned8 < reg.unsigned8)
				SETF_CARRY(cpu.procstat, 1);

			reg.signed8 = -128 + (tval - 127); //Wrap to bottom and continue (for negative subtraction or addition)
		} else if(tval < -128) {
			SETF_OVRFLOW(cpu.procstat, 1);
			if (reg.unsigned8 + rvalue.unsigned8 < reg.unsigned8)
				SETF_CARRY(cpu.procstat, 1);

			reg.signed8 = 127 - (tval + 128); //Wrap to top and continue (for subtraction or negative addition)
		}
		else
			reg.signed8 += rvalue.signed8;
#else   //For environments that handle overflows in the same manner as 6502.
		if (carry == CARRY_UNSET)
			carry = F_CARRY(cpu.procstat);

		add_result = reg.signed8;
		add_result += rvalue.signed8;
		add_result += carry;

		//Sign of both bits different from result
		SETF_OVRFLOW(cpu.procstat, ((reg.signed8 ^ add_result) &
									(rvalue.signed8 ^ add_result) & 0x80)
									!= 0);

		//Detect unsigned integer wrap-around
		SETF_CARRY(cpu.procstat, reg.unsigned8 > static_cast<uint8_t>(reg.unsigned8 + rvalue.unsigned8));

		reg.signed8 = add_result;
#endif
		//Negative Flag
		SETF_NEG(cpu.procstat, reg.signed8 < 0);

		//Zero Flag
		SETF_ZERO(cpu.procstat, reg.unsigned8 == 0);
		break;

	case EQL:
		reg = rvalue;

		//Negative Flag
		SETF_NEG(cpu.procstat, reg.signed8 < 0);
		//Zero Flag
		SETF_ZERO(cpu.procstat, reg.unsigned8 == 0);
		break;

	case CMP:
		add_result = reg.signed8 - rvalue.signed8;

		SETF_ZERO(cpu.procstat, add_result == 0);
		SETF_NEG(cpu.procstat, add_result < 0);
		SETF_CARRY(cpu.procstat, reg.signed8 >= rvalue.signed8);
		break;
		
	case ROL:
		//$STUB$
		break;
		
	case ASL:
		//$STUB$
		break;

	default:
		SETF_DEBUG(cpu.procstat, 1);
	}
}

/* Translation routine for operations with CC01 opcode pattern. */
void translate_CC01_get(Mapper& mapper, CPU& cpu, OPCODE code, REG_S8B& reg, operations::OperationT operation)
{
	MEM_BYTE rvalue;
	switch (code & MASK_BBB)
	{
	case AMODE_ZPAGEINX:
		rvalue = GET_INDXINDIRECT(mapper, cpu);
		perform_operation(cpu, reg, rvalue, operation);
		break;
	case AMODE_ZPAGE:
		rvalue = GET_ZPAGE(mapper, cpu);
		perform_operation(cpu, reg, rvalue, operation);
		break;
	case AMODE_ZPAGEINY:
		rvalue = GET_INDIRECTINDXY(mapper, cpu);
		perform_operation(cpu, reg, rvalue, operation);
		break;
	case AMODE_ABS:
		rvalue = GET_ABSOLUTE(mapper, cpu);
		perform_operation(cpu, reg, rvalue, operation);
		break;
	case AMODE_ABSX:
		rvalue = GET_ABSOLUTEX(mapper, cpu);
		perform_operation(cpu, reg, rvalue, operation);
		break;
	case AMODE_ABSY:
		rvalue = GET_ABSOLUTEY(mapper, cpu);
		perform_operation(cpu, reg, rvalue, operation);
		break;
	case AMODE_IMMED:
		rvalue = GET_IMMEDIATE(mapper, cpu);
		perform_operation(cpu, reg, rvalue, operation);
		break;
	case AMODE_ZPAGEX:
		rvalue = GET_ZPAGEX(mapper, cpu);
		perform_operation(cpu, reg, rvalue, operation);
		break;
		
	default: //Should never reach here!
		SETF_DEBUG(cpu.procstat, 1);
	}
}

/* Translation for memory set for CC01 opcode pattern. */
void translate_CC01_set(Mapper& mapper, CPU& cpu, OPCODE code, REG_S8B& reg)
{
	switch (code & MASK_BBB)
	{
		case AMODE_ABS:
			SET_ABSOLUTE(mapper, cpu, reg.unsigned8);
			break;
		case AMODE_ABSX:
			SET_ABSOLUTEX(mapper, cpu, reg.unsigned8);
			break;
		case AMODE_ABSY:
			SET_ABSOLUTEY(mapper, cpu, reg.unsigned8);
			break;
		case AMODE_ZPAGE:
			SET_ZPAGE(mapper, cpu, reg.unsigned8);
			break;
		case AMODE_ZPAGEINX:
		    SET_INDXINDIRECT(mapper, cpu, reg.unsigned8);
			break;
		case AMODE_ZPAGEX:
			SET_ZPAGEX(mapper, cpu, reg.unsigned8);
			break;
		case AMODE_ZPAGEINY:
			SET_ZPAGEINY(mapper, cpu, reg.unsigned8);
			break;

		default: //Should never reach here!
			SETF_DEBUG(cpu.procstat, 1);
	}	
}

/* Translation routine for operations with CC10 opcode pattern. */
void translate_CC10_get(Mapper& mapper, CPU& cpu, OPCODE code, REG_S8B& reg, operations::OperationT operation)
{
	MEM_BYTE rvalue;
	OPCODE msk_code;

	switch (code & MASK_BBB)
	{
	case AMODE_ZPAGE:
		rvalue = GET_ZPAGE(mapper, cpu);
		perform_operation(cpu, reg, rvalue, operation);
		break;
	case AMODE_ABS:
		rvalue = GET_ABSOLUTE(mapper, cpu);
		perform_operation(cpu, reg, rvalue, operation);
		break;
	case AMODE_10IMMED:
		rvalue = GET_IMMEDIATE(mapper, cpu);
		perform_operation(cpu, reg, rvalue, operation);
		break;	

	case AMODE_10ABSX:
		msk_code = MASK_AAACC & code;

		if (msk_code == OP_LDX) //Exceptional case
			rvalue = GET_ABSOLUTEY(mapper, cpu);
		else
			rvalue = GET_ABSOLUTEX(mapper, cpu);

		perform_operation(cpu, reg, rvalue, operation);
		break;

	case AMODE_10ZPAGEX:
		msk_code = MASK_AAACC & code;

		if (msk_code == OP_LDX) //Exceptional case
			rvalue = GET_ZPAGEY(mapper, cpu);
		else
			rvalue = GET_ZPAGEX(mapper, cpu);

		perform_operation(cpu, reg, rvalue, operation);
		break;

	case AMODE_ACCUM:
		//$STUB$
		break;

	default: //Should never reach here!
		SETF_DEBUG(cpu.procstat, 1);
	}
}

/* Translation for memory set for CC10 opcode pattern. */
void translate_CC10_set(Mapper& mapper, CPU& cpu, OPCODE code, REG_S8B& reg)
{
	OPCODE msk_code;

	switch (code & MASK_BBB)
	{
	case AMODE_ZPAGE:
		SET_ZPAGE(mapper, cpu, reg.unsigned8);
		break;
	case AMODE_ABS:
		SET_ABSOLUTE(mapper, cpu, reg.unsigned8);
		break;

	case AMODE_ABSX:
		msk_code = MASK_AAACC & code;

		if (msk_code == OP_STX) //Exceptional case
			SET_ABSOLUTEY(mapper, cpu, reg.unsigned8);
		else
			SET_ABSOLUTEX(mapper, cpu, reg.unsigned8);
		break;

	case AMODE_10IMMED:
		//$STUB$
		break;

	case AMODE_ZPAGEX:
		msk_code = MASK_AAACC & code;

		if (msk_code == OP_STX) //Exceptional case
			SET_ZPAGEY(mapper, cpu, reg.unsigned8);
		else
			SET_ZPAGEX(mapper, cpu, reg.unsigned8);
		break;

	case AMODE_ACCUM:
		//$STUB$
		break;

	default: //Should never reach here!
		SETF_DEBUG(cpu.procstat, 1);
	}
}

/* Translation routine for operations with CC00 opcode pattern. */
void translate_CC00_get(Mapper& mapper, CPU& cpu, OPCODE code, REG_S8B& reg, operations::OperationT operation)
{
	MEM_BYTE rvalue;
	switch (code & MASK_BBB)
	{
		case AMODE_ZPAGE:
			rvalue = GET_ZPAGE(mapper, cpu);
			perform_operation(cpu, reg, rvalue, operation);
			break;
		case AMODE_ABS:
			rvalue = GET_ABSOLUTE(mapper, cpu);
			perform_operation(cpu, reg, rvalue, operation);
			break;
		case AMODE_ABSX:
			rvalue = GET_ABSOLUTEX(mapper, cpu);
			perform_operation(cpu, reg, rvalue, operation);
			break;
		case AMODE_10IMMED:
			rvalue = GET_IMMEDIATE(mapper, cpu);
			perform_operation(cpu, reg, rvalue, operation);
			break;
		case AMODE_ZPAGEX:
			rvalue = GET_ZPAGEX(mapper, cpu);
			perform_operation(cpu, reg, rvalue, operation);
			break;

		default: //Should never reach here!
			SETF_DEBUG(cpu.procstat, 1);
	}
}

/* Translation for memory set for CC00 opcode pattern. */
void translate_CC00_set(Mapper& mapper, CPU& cpu, OPCODE code, REG_S8B& reg)
{
	switch (code & MASK_BBB)
	{
		case AMODE_ZPAGEX:
			SET_ZPAGEX(mapper, cpu, reg.unsigned8);
			break;
		case AMODE_ZPAGE:
			SET_ZPAGE(mapper, cpu, reg.unsigned8);
			break;
		case AMODE_ABS:
			SET_ABSOLUTE(mapper, cpu, reg.unsigned8);
			break;

		default: //Should never reach here!
			SETF_DEBUG(cpu.procstat, 1);
	}
}

//======================================================
//Emulator
//======================================================

/* Stop the emulation cycle. Meant to be called from thread or handler. */
int Emulator2A03::stopEmulation()
{
	std::lock_guard<std::mutex> lock(ticks_remaining_m);
	ticks_remaining = 0;
	return ticks_remaining;
}

/* Emulate the CPU for a number of cycles. */
int Emulator2A03::emulate_cpu(int exec_ticks)
{
	stopemulation = false;
	ticks_remaining = exec_ticks;

	while (ticks_remaining > 0) {

		//Make sure we weren't stopped by another thread.
		/* $STUB$ REVISE
		ticks_remaining_m.lock();
		if (stopemulation)
			return ticks_remaining;
		ticks_remaining_m.unlock();
		*/
		OPCODE code = mapper.readMemory(cpu.progcount);
		++cpu.progcount;
		ticks_remaining -= OPTICK[code];

		switch (code)
		{
		//KIL
		case OP_KIL0:
		case OP_KIL1:
		case OP_KIL2:
		case OP_KIL3:
		case OP_KIL4:
		case OP_KIL5:
		case OP_KIL6:
		case OP_KIL7:
		case OP_KIL8:
		case OP_KIL9:
		case OP_KILA:
		case OP_KILB:
			errstate = ERROR_STATE::CPU_LOCK;
			return ticks_remaining;

		//NOPs
		case OP_NOP:
		case OP_NOP0:
		case OP_NOP1:
		case OP_NOP2:
		case OP_NOP3:
		case OP_NOP4:
		case OP_NOP5:
			break;

		//Double NOPs
		case OP_DNOP0:
		case OP_DNOP1:
		case OP_DNOP2:
		case OP_DNOP3:
		case OP_DNOP4:
		case OP_DNOP5:
		case OP_DNOP6:
		case OP_DNOP7:
		case OP_DNOP8:
		case OP_DNOP9:
		case OP_DNOPA:
		case OP_DNOPB:
		case OP_DNOPC:
		case OP_DNOPD:
			++cpu.progcount;
			break;

		//Increment/Decrement x/y registers
		case OP_INX:
			++cpu.xindex.signed8;
			//Negative Flag
			SETF_NEG(cpu.procstat, cpu.xindex.signed8 < 0);
			//Zero Flag
			SETF_ZERO(cpu.procstat, cpu.xindex.signed8 == 0);
			break;
		case OP_INY:
			++cpu.yindex.signed8;
			//Negative Flag
			SETF_NEG(cpu.procstat, cpu.yindex.signed8 < 0);
			//Zero Flag
			SETF_ZERO(cpu.procstat, cpu.yindex.signed8 == 0);
			break;
		case OP_DEX:
			--cpu.xindex.signed8;
			//Negative Flag
			SETF_NEG(cpu.procstat, cpu.xindex.signed8 < 0);
			//Zero Flag
			SETF_ZERO(cpu.procstat, cpu.xindex.signed8 == 0);
			break;
		case OP_DEY:
			--cpu.yindex.signed8;
			//Negative Flag
			SETF_NEG(cpu.procstat, cpu.yindex.signed8 < 0);
			//Zero Flag
			SETF_ZERO(cpu.procstat, cpu.yindex.signed8 == 0);
			break;


		//Branch instructions
#define JMP_CODE cpu.progcount += static_cast<int>(mapper.readMemory(cpu.progcount)) + 1;

		case OP_BCC:
			if (!F_CARRY(cpu.procstat))
			{
				JMP_CODE;
			}
			else
				++cpu.progcount;
			break;
		case OP_BCS:
			if (F_CARRY(cpu.procstat))
			{
				JMP_CODE;
			}
			else
				++cpu.progcount;
			break;
		case OP_BEQ:
			if (F_ZERO(cpu.procstat))
			{
				JMP_CODE;
			}
			else
				++cpu.progcount;
			break;
		case OP_BMI:
			if (F_NEG(cpu.procstat))
			{
				JMP_CODE;
			}
			else
				++cpu.progcount;
			break;
		case OP_BNE:
			if (!F_ZERO(cpu.procstat))
			{
				JMP_CODE;
			}
			else
				++cpu.progcount;
			break;
		case OP_BPL:
			if (!F_NEG(cpu.procstat))
			{
				JMP_CODE;
			}
			else
				++cpu.progcount;
			break;
		case OP_BVC:
			if (!F_OVRFLOW(cpu.procstat))
			{
				JMP_CODE;
			}
			else
				++cpu.progcount;
			break;
		case OP_BVS:
			if (F_OVRFLOW(cpu.procstat))
			{
				JMP_CODE;
			}
			else
				++cpu.progcount;
			break;

		//Transfer instructions
		case OP_TAX:
			cpu.xindex = cpu.accumulator;
			//Negative Flag
			SETF_NEG(cpu.procstat, cpu.xindex.signed8 < 0);
			//Zero Flag
			SETF_ZERO(cpu.procstat, cpu.xindex.signed8 == 0);
			break;
		case OP_TXA:
			cpu.accumulator = cpu.xindex;
			//Negative Flag
			SETF_NEG(cpu.procstat, cpu.accumulator.signed8 < 0);
			//Zero Flag
			SETF_ZERO(cpu.procstat, cpu.accumulator.signed8 == 0);
			break;
		case OP_TAY:
			cpu.yindex = cpu.accumulator;
			//Negative Flag
			SETF_NEG(cpu.procstat, cpu.yindex.signed8 < 0);
			//Zero Flag
			SETF_ZERO(cpu.procstat, cpu.yindex.signed8 == 0);
			break;
		case OP_TYA:
			cpu.accumulator = cpu.yindex;
			//Negative Flag
			SETF_NEG(cpu.procstat, cpu.accumulator.signed8 < 0);
			//Zero Flag
			SETF_ZERO(cpu.procstat, cpu.xindex.signed8 == 0);
			break;
		case OP_TSX:
			cpu.xindex = cpu.stackp;
			//Negative Flag
			SETF_NEG(cpu.procstat, cpu.xindex.signed8 < 0);
			//Zero Flag
			SETF_ZERO(cpu.procstat, cpu.xindex.signed8 == 0);
			break;
		case OP_TXS:
			cpu.stackp = cpu.xindex;
			//Negative Flag
			SETF_NEG(cpu.procstat, cpu.stackp.signed8 < 0);
			//Zero Flag
			SETF_ZERO(cpu.procstat, cpu.stackp.signed8 == 0);
			break;

		//Stack instructions - $STUB$ Implement stack overflow
		case OP_PHA:
			mapper.writeMemory(L_STACKT + cpu.stackp.unsigned8, cpu.accumulator.unsigned8);
			--cpu.stackp.unsigned8;
			break;
		case OP_PLA:
			++cpu.stackp.unsigned8;
			cpu.accumulator.unsigned8 = mapper.readMemory(L_STACKT + cpu.stackp.unsigned8);
			//Negative Flag
			SETF_NEG(cpu.procstat, cpu.accumulator.signed8 < 0);
			//Zero Flag
			SETF_ZERO(cpu.procstat, cpu.accumulator.signed8 == 0);
			break;
		case OP_PHP:
			mapper.writeMemory(L_STACKT + cpu.stackp.unsigned8, cpu.procstat);
			--cpu.stackp.unsigned8;
			break;
		case OP_PLP:
			++cpu.stackp.unsigned8;
			cpu.procstat = mapper.readMemory(L_STACKT + cpu.stackp.unsigned8);
			break;

		//Set and Clear
		case OP_SEC:
			SETF_CARRY(cpu.procstat, 1);
			break;
		case OP_SED:
			SETF_DMODE(cpu.procstat, 1);
			break;
		case OP_SEI:
			SETF_INTDIS(cpu.procstat, 1);
			break;
		case OP_CLC:
			SETF_CARRY(cpu.procstat, 0);
			break;
		case OP_CLD:
			SETF_DMODE(cpu.procstat, 0);
			break;
		case OP_CLI:
			SETF_INTDIS(cpu.procstat, 0);
			break;
		case OP_CLV:
			SETF_OVRFLOW(cpu.procstat, 0);
			break;

		//Flow Control Instructions
		case OP_BRK:
			cpu.progcount += 2;
			//Push PC
			mapper.writeMemory(L_STACKT + cpu.stackp.unsigned8, (cpu.progcount - 1));
			--cpu.stackp.unsigned8;
			mapper.writeMemory(L_STACKT + cpu.stackp.unsigned8, (cpu.progcount >> 8));
			--cpu.stackp.unsigned8;
			//Push PSW
			mapper.writeMemory(L_STACKT + cpu.stackp.unsigned8, cpu.procstat);
			--cpu.stackp.unsigned8;
			//Go to BRK address
			cpu.progcount = mapper.readMemory(L_BRKHNDL) | mapper.readMemory(L_BRKHNDL + 1) << 8;
			SETF_BRKCMD(cpu.procstat, 1);
			break;
		case OP_JSRABS:
			//Push PC
			mapper.writeMemory(L_STACKT + cpu.stackp.unsigned8, cpu.progcount + 1);
			--cpu.stackp.unsigned8;
			mapper.writeMemory(L_STACKT + cpu.stackp.unsigned8, (cpu.progcount + 1) >> 8);
			--cpu.stackp.unsigned8;
			//Go to address specified by operand
			cpu.progcount = mapper.readMemory(cpu.progcount) | (mapper.readMemory(cpu.progcount + 1) << 8);
			break;
		case OP_RTI:
			//Pop PSW
			cpu.procstat = mapper.readMemory(L_STACKT + cpu.stackp.unsigned8);
			++cpu.stackp.unsigned8;
			//Pop PC - Return address
			++cpu.stackp.unsigned8;
			cpu.progcount = mapper.readMemory(L_STACKT + cpu.stackp.unsigned8) << 8;
			++cpu.stackp.unsigned8;
			cpu.progcount |= mapper.readMemory(L_STACKT + cpu.stackp.unsigned8);
			break;
		case OP_RTS:
			//Pop PC - Return address
			++cpu.stackp.unsigned8;
			cpu.progcount = mapper.readMemory(L_STACKT + cpu.stackp.unsigned8) << 8;
			++cpu.stackp.unsigned8;
			cpu.progcount |= mapper.readMemory(L_STACKT + cpu.stackp.unsigned8);
			++cpu.progcount;
			break;
		case OP_JMP:
			//Indirection
			cpu.progcount = mapper.readMemory(cpu.progcount) |
				(mapper.readMemory(cpu.progcount + 1) << 8);
			cpu.progcount = mapper.readMemory(cpu.progcount) |
				(mapper.readMemory(cpu.progcount + 1) << 8);
			break;
		case OP_JMPABS:
			cpu.progcount = mapper.readMemory(cpu.progcount) | (mapper.readMemory(cpu.progcount + 1) << 8);
			break;

		//Handle OPCODEs with AAABBBCC bit patterns
		default:

			//$STUB$ reinterperet_cast to maintain bit patterns
			switch (code & MASK_AAACC)
			{
			//==== Start of CC 01 ====
			case OP_ORA:
				translate_CC01_get(mapper, cpu, code, cpu.accumulator, operations::BIT_OR);
				break;
			case OP_AND:
				translate_CC01_get(mapper, cpu, code, cpu.accumulator, operations::BIT_AND);
				break;
			case OP_EOR:
				translate_CC01_get(mapper, cpu, code, cpu.accumulator, operations::BIT_XOR);
				break;
			case OP_ADC:
				translate_CC01_get(mapper, cpu, code, cpu.accumulator, operations::ADD);
				break;
			case OP_STA:
				translate_CC01_set(mapper, cpu, code, cpu.accumulator);
				break;
			case OP_LDA:
				translate_CC01_get(mapper, cpu, code, cpu.accumulator, operations::EQL);
				break;
			case OP_CMP:
				translate_CC01_get(mapper, cpu, code, cpu.accumulator, operations::CMP);
				break;
			case OP_SBC:
				translate_CC01_get(mapper, cpu, code, cpu.accumulator, operations::SUB);
				break;

			//==== Start of CC 10 ====
			case OP_ASL:
				//translate_CC10_get(mapper, cpu, code, cpu.accumulator, operations::ASL);
				break;
			case OP_ROL:
				//translate_CC10_get(mapper, cpu, code, cpu.accumulator, operations::ROL);
				break;
			case OP_LSR:
				//translate_CC10_get(mapper, cpu, code, cpu.accumulator, operations::LSR);
				break;
			case OP_ROR:
				//translate_CC10_get(mapper, cpu, code, cpu.accumulator, operations::ROR);
				break;
			case OP_STX:
				//translate_CC10_set(mapper, cpu, code, cpu.xindex);
				break;
			case OP_LDX:
				//translate_CC10_get(mapper, cpu, code, cpu.xindex, operations::EQL);
				break;
			case OP_DEC:
				//$STUB$
				break;
			case OP_INC:
				//$STUB$
				break;

		    //==== Start of CC 00 ====
			case OP_BIT:
				//TRANSLATE_CC00_GET(code, mapper, cpu.temp.signed8, = );
				//Zero Flag
				SETF_ZERO(cpu.procstat, (uint8_t)((cpu.accumulator.unsigned8 & cpu.temp.unsigned8) == 0));
				//Negative Flag
				SETF_NEG(cpu.procstat, cpu.temp.signed8 < 0);
				//Overflow Flag
				SETF_OVRFLOW(cpu.procstat, BIT6(cpu.temp.unsigned8));
				break;
			case OP_STY:
				//translate_CC00_set(mapper, cpu, code, cpu.yindex);
				break;
			case OP_LDY:
				//translate_CC00_get(mapper, cpu, code, cpu.yindex, operations::EQL);
				break;
			case OP_CPY:
				//translate_CC00_get(mapper, cpu, code, cpu.yindex, operations::CMP);
				break;
			case OP_CPX:
				//translate_CC00_get(mapper, cpu, code, cpu.xindex, operations::CMP);
				break;

			default:
				errstate = ERROR_STATE::UNKNOWN_INSTRUCTION;
				return ticks_remaining;
			}
		}
	}

	return ticks_remaining;
}