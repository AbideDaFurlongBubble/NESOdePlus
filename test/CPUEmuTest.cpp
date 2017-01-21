#include "Emulator.h"
#include "Test.h"
#include "Helpers.h"
#include "Instructions.h"
#include <chrono>
#include <iostream>

#define CPUEMUTEST CPUEmulationTest

#define INIT_CPUEMU \
	CREATE_DEFMAP; \
	emu::CPU cpu; \
	emu::initializeCPU(cpu); \
	emu::Emulator2A03 cpuemu(*defmap, cpu); \
	cpu.progcount = 0x8000;

#define TEARDOWN_CPUEMU \
	TEARDOWN_DEFMAP;

/* Used to print the registers of the CPU and the core emulator state on assertion failure. */
/*
::testing::AssertionResult ASSERT_COMP_TRUE(emu::CPU& cpu, emu::Emulator2A03& emu, OPCODE code, bool comp)
{
	if (comp)
		return ::testing::AssertionSuccess();
	else
		return ::testing::AssertionFailure()<< "CPU (0x" << code << "):"
											<< "\nAccumulator = " << cpu.accumulator
											<< "\nX = " << cpu.xindex
											<< "\nY = " << cpu.yindex
											<< "\nProgram Counter = " << cpu.progcount
											<< "\nStatus = " << cpu.procstat
											<< "\nStack* = " << cpu.stackp
											<< "\nDebug = " << cpu.debug;
}*/

/* Test for no-op instructions. */
TEST(CPUEMUTEST, NOPTEST) {
	INIT_CPUEMU;
	const int progstart = cpu.progcount;
	const int numinst = 5;
	writeOpToMem(*defmap, OP_NOP, L_PRGROM, numinst);
	int ticksr = cpuemu.emulate_cpu(numinst * emu::OPTICK[OP_NOP]);
	ASSERT_EQ(progstart + numinst, cpu.progcount);
	ASSERT_EQ(ticksr, 0);
	TEARDOWN_CPUEMU;
}

/* Test for double no-op instructions. */
TEST(CPUEMUTEST, DNOPTEST) {
	INIT_CPUEMU;
	const int progstart = cpu.progcount;
	const int numinst = 5;
	writeOpToMem(*defmap, OP_DNOPB, L_PRGROM, numinst, 2);
	int ticksr = cpuemu.emulate_cpu(numinst * emu::OPTICK[OP_DNOPB]);
	ASSERT_EQ(progstart + numinst * 2, cpu.progcount);
	ASSERT_EQ(ticksr, 0);
	TEARDOWN_CPUEMU;
}

/* Test for KIL instructions (halts CPU). */
TEST(CPUEMUTEST, KILTEST) {
	INIT_CPUEMU;
	const int progstart = cpu.progcount;
	const int numinst = 5;
	writeOpToMem(*defmap, OP_KIL0, L_PRGROM, numinst);
	int ticksr = cpuemu.emulate_cpu(numinst * emu::OPTICK[OP_KIL0]);
	ASSERT_EQ(ticksr, numinst * emu::OPTICK[OP_KIL0] - emu::OPTICK[OP_KIL0]);
	ASSERT_EQ(cpuemu.getErrorState(), emu::ERROR_STATE::CPU_LOCK);
	TEARDOWN_CPUEMU;
}

/* Test for increment instructions. */
TEST(CPUEMUTEST, INCXY) {
	INIT_CPUEMU;
	cpu.xindex.signed8 = 0;
	cpu.yindex.signed8 = 0;
	const int progstart = cpu.progcount;
	OPCODE inject[] = { OP_INX, OP_INY };
	writePatternToMem(*defmap, inject, 2, L_PRGROM);
	int ticksr = cpuemu.emulate_cpu(emu::OPTICK[OP_INX] + emu::OPTICK[OP_INY]);
	ASSERT_EQ(ticksr, 0);
	ASSERT_EQ(cpu.xindex.signed8, 1);
	ASSERT_EQ(cpu.yindex.signed8, 1);
	TEARDOWN_CPUEMU;
}

/* Test for decrement instructions. */
TEST(CPUEMUTEST, DECXY) {
	INIT_CPUEMU;
	cpu.xindex.signed8 = 0;
	cpu.yindex.signed8 = 0;
	const int progstart = cpu.progcount;
	OPCODE inject[] = { OP_DEX, OP_DEY };
	writePatternToMem(*defmap, inject, 2, L_PRGROM);
	int ticksr = cpuemu.emulate_cpu(emu::OPTICK[OP_DEX] + emu::OPTICK[OP_DEY]);
	ASSERT_EQ(ticksr, 0);
	ASSERT_EQ(cpu.xindex.signed8, -1);
	ASSERT_EQ(cpu.yindex.signed8, -1);
	TEARDOWN_CPUEMU;
}

/* Test for register transfer instructions. */
TEST(CPUEMUTEST, TRANSFER_INS) {
	INIT_CPUEMU;
	cpu.accumulator.signed8 = 5;
	cpu.xindex.signed8 = 0;
	cpu.yindex.signed8 = 3;
	const int progstart = cpu.progcount;
	//Acc -> X (5); Y -> acc (3);  X -> acc (5); acc -> Y (5); X -> stack (5); stack -> X (10)
	OPCODE inject[] = { OP_TAX, OP_TYA, OP_TXA, OP_TAY, OP_TXS, OP_TSX };
	writePatternToMem(*defmap, inject, 6, L_PRGROM);
	int ticksr = cpuemu.emulate_cpu(emu::OPTICK[OP_TAX] + emu::OPTICK[OP_TYA]);
	ASSERT_EQ(cpu.xindex.signed8, 5);
	ASSERT_EQ(cpu.accumulator.signed8, 3);
	ASSERT_EQ(ticksr, 0);

	ticksr = cpuemu.emulate_cpu(emu::OPTICK[OP_TXA] + emu::OPTICK[OP_TAY]);
	ASSERT_EQ(cpu.accumulator.signed8, 5);
	ASSERT_EQ(cpu.yindex.signed8, 5);
	ASSERT_EQ(ticksr, 0);

	ticksr = cpuemu.emulate_cpu(emu::OPTICK[OP_TXS]);
	ASSERT_EQ(cpu.stackp.signed8, 5);
	ASSERT_EQ(ticksr, 0);
	ASSERT_FALSE(F_NEG(cpu.procstat));
	ASSERT_FALSE(F_ZERO(cpu.procstat));

	cpu.stackp.signed8 = -10;
	ticksr = cpuemu.emulate_cpu(emu::OPTICK[OP_TSX]);
	ASSERT_EQ(cpu.xindex.signed8, -10);
	ASSERT_EQ(ticksr, 0);
	SETBIT7(cpu.procstat, 1);
	ASSERT_EQ(F_NEG(cpu.procstat), 1);
	ASSERT_EQ(F_ZERO(cpu.procstat), 0);
	TEARDOWN_CPUEMU;
}

/* Test for all stack manipulation instructions. */
TEST(CPUEMUTEST, STACK_INS) {
	INIT_CPUEMU;
	cpu.accumulator.signed8 = 10;
	const int progstart = cpu.progcount;
	OPCODE inject[] = { OP_PHA, OP_PLA, OP_PHP, OP_PLP };
	writePatternToMem(*defmap, inject, 4, L_PRGROM);

	int ticksr = cpuemu.emulate_cpu(emu::OPTICK[OP_PHA]);
	ASSERT_EQ(ticksr, 0);
	ASSERT_EQ(defmap->getMemory()[L_STACKT + 0xFF], 10);
	ASSERT_EQ(cpu.stackp.unsigned8, 254);

	cpu.accumulator.signed8 = -5;
	ticksr = cpuemu.emulate_cpu(emu::OPTICK[OP_PLA]);
	ASSERT_EQ(ticksr, 0);
	ASSERT_EQ(cpu.accumulator.signed8, 10);
	ASSERT_EQ(cpu.stackp.unsigned8, 255);

	cpu.procstat = 0;
	SETF_ZERO(cpu.procstat, 1);
	SETF_OVRFLOW(cpu.procstat, 1);
	
	ticksr = cpuemu.emulate_cpu(emu::OPTICK[OP_PHP]);
	ASSERT_EQ(ticksr, 0);
	ASSERT_EQ(defmap->getMemory()[L_STACKT + 0xFF], 0x42);

	ticksr = cpuemu.emulate_cpu(emu::OPTICK[OP_PLP]);
	ASSERT_EQ(ticksr, 0);
	ASSERT_EQ(cpu.procstat, 0x42);

	TEARDOWN_CPUEMU;
}

/* Test for all flag manipulation instructions. */
TEST(CPUEMUTEST, FLAG_INS) {
	INIT_CPUEMU;
	const int progstart = cpu.progcount;
	OPCODE inject[] = { OP_SEC, OP_SED, OP_SEI, OP_CLC, OP_CLD, OP_CLI, OP_CLV };
	writePatternToMem(*defmap, inject, 7, L_PRGROM);

	int ticksr = cpuemu.emulate_cpu(emu::OPTICK[OP_SEC] + emu::OPTICK[OP_SED] + emu::OPTICK[OP_SEI]);
	ASSERT_EQ(ticksr, 0);
	ASSERT_EQ(F_CARRY(cpu.procstat), 1);
	ASSERT_EQ(F_DMODE(cpu.procstat), 1);
	ASSERT_EQ(F_INTDIS(cpu.procstat), 1);

	ticksr = cpuemu.emulate_cpu(emu::OPTICK[OP_CLC] + emu::OPTICK[OP_CLD] + emu::OPTICK[OP_CLI]);
	ASSERT_EQ(ticksr, 0);
	ASSERT_EQ(F_CARRY(cpu.procstat), 0);
	ASSERT_EQ(F_DMODE(cpu.procstat), 0);
	ASSERT_EQ(F_INTDIS(cpu.procstat), 0);

	SETF_OVRFLOW(cpu.procstat, 1);
	ticksr = cpuemu.emulate_cpu(emu::OPTICK[OP_CLV]);
	ASSERT_EQ(ticksr, 0);
	ASSERT_EQ(F_OVRFLOW(cpu.procstat), 0);

	TEARDOWN_CPUEMU;
}

/* Test for BRK instruction. May be re-written to more faithfully emulate 2A05. */
TEST(CPUEMUTEST, BRK_INS) {
	INIT_CPUEMU;
	const int progstart = cpu.progcount;
	cpu.xindex.signed8 = 0;
	//Write BRK to L_PRGROM
	OPCODE opmain[] = { OP_BRK, OP_PADB, OP_INX };
	writePatternToMem(*defmap, opmain, 3, L_PRGROM);

	//Write BRK routine address to memory
	writeOpToMem(*defmap, 0xFF, 0xFFFE);
	writeOpToMem(*defmap, 0x80, 0xFFFF);

	//Write BRK routine to memory
	writeOpToMem(*defmap, OP_INX, 0x80FF);
	writeOpToMem(*defmap, OP_RTI, 0x8100);

	int runticks = countOpTick(opmain, emu::OPTICK, 3);
	runticks += emu::OPTICK[OP_INX] + emu::OPTICK[OP_RTI] - emu::OPTICK[OP_PADB];

	int ticksr = cpuemu.emulate_cpu(runticks);
	ASSERT_EQ(ticksr, 0);
	ASSERT_EQ(F_BRKCMD(cpu.procstat), 0);
	ASSERT_EQ(cpu.xindex.signed8, 2);
	ASSERT_EQ(cpu.progcount, 0x8003);

	TEARDOWN_CPUEMU;
}

/* Test for all branch instructions. */
TEST(CPUEMUTEST, BRANCH_INS) {
	INIT_CPUEMU;
	MEM_BYTE jmpindx;
	jmpindx.signed8 = 126;
	ADDR_16B writeaddr = 0x8000;
	cpu.xindex.signed8 = 0;

	OPCODE opmain[] = {
		OP_BCC,
		OP_BCS,
		OP_BEQ,
		OP_BMI,
		OP_BNE,
		OP_BPL,
		OP_BVC,
		OP_BVS
	};

	//Used to trigger branch instructions
	OPCODE opflags[] = {
		!CARRY_BIT,
		CARRY_BIT,
		ZERO_BIT,
		NEG_BIT,
		!ZERO_BIT,
		!NEG_BIT,
		!OVRFLOW_BIT,
		OVRFLOW_BIT
	};

	//Add relative address to 0x8080 behind the OPs
	for (int k = 0; k < 8; k++)
	{
		writeOpToMem(*defmap, opmain[k], writeaddr);
		writeaddr++;
		writeOpToMem(*defmap, jmpindx.unsigned8, writeaddr);
		writeaddr++;
		jmpindx.signed8 -= 2;
	}

	//Counter to determine branch success
	writeOpToMem(*defmap, OP_INX, 0x8080);

	//Meat and potatos
	for (int j = 0; j < 8; j++)
	{
		cpu.progcount = 0x8000 + j * 2;
		cpu.procstat = opflags[j];
		cpu.xindex.unsigned8 = 0;
		int ticksr = cpuemu.emulate_cpu(emu::OPTICK[opmain[j]] + emu::OPTICK[OP_INX]); 
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.xindex.signed8, 1);
		ASSERT_EQ(cpu.progcount, 0x8081);
	}

	TEARDOWN_CPUEMU;
}

/* Test for relative absolute jump (JMPABS) and direct jump (JMP) */
TEST(CPUEMUTEST, JUMP_INS) {
	INIT_CPUEMU;

	writeOpToMem(*defmap, OP_INX, 0x8080);

	//Absolute Jump
	writeOpToMem(*defmap, OP_JMPABS, 0x8000);
	writeOpToMem(*defmap, 0x80, 0x8001);
	writeOpToMem(*defmap, 0x80, 0x8002);

	//Jump
	writeOpToMem(*defmap, OP_JMP, 0x8003);
	writeOpToMem(*defmap, 0x06, 0x8004);
	writeOpToMem(*defmap, 0x80, 0x8005);

	//Indirect ptr
	writeOpToMem(*defmap, 0x80, 0x8006);
	writeOpToMem(*defmap, 0x80, 0x8007);

	int ticksr = cpuemu.emulate_cpu(emu::OPTICK[OP_INX] + emu::OPTICK[OP_JMPABS]);
	ASSERT_EQ(ticksr, 0);
	ASSERT_EQ(cpu.progcount, 0x8081);
	ASSERT_EQ(cpu.xindex.signed8, 1);

	cpu.progcount = 0x8003;
	ticksr = cpuemu.emulate_cpu(emu::OPTICK[OP_INX] + emu::OPTICK[OP_JMP]);
	ASSERT_EQ(ticksr, 0);
	ASSERT_EQ(cpu.progcount, 0x8081);
	ASSERT_EQ(cpu.xindex.signed8, 2);

	TEARDOWN_CPUEMU;
}

/* Test for subroutine jump (JSRABS) and subroutine return (RTS) */
TEST(CPUEMUTEST, SUBROUTINE_INS)
{
	INIT_CPUEMU;

	//Jump to sub
	writeOpToMem(*defmap, OP_JSRABS, 0x8000);
	writeOpToMem(*defmap, 0x80, 0x8001);
	writeOpToMem(*defmap, 0x80, 0x8002);

	//Subs
	writeOpToMem(*defmap, OP_INX, 0x8080);
	writeOpToMem(*defmap, OP_RTS, 0x8081);

	int ticksr = cpuemu.emulate_cpu(emu::OPTICK[OP_INX] + emu::OPTICK[OP_JSRABS] + emu::OPTICK[OP_RTS]);
	ASSERT_EQ(ticksr, 0);
	ASSERT_EQ(cpu.progcount, 0x8003);
	ASSERT_EQ(cpu.xindex.signed8, 1);

	TEARDOWN_CPUEMU;
}

//============================================================================
//Translate Instructions
//============================================================================
OPCODE CC01Opticks[] = {
	AMODE_ZPAGEINX,
	AMODE_ZPAGE,
	AMODE_IMMED,
	AMODE_ABS,
	AMODE_ZPAGEINY,
	AMODE_ZPAGEX,
	AMODE_ABSY,
	AMODE_ABSX
};

const int SZ_CC01 = 8;

/* Test for ORA instruction. Covers all addressing modes. */
TEST(CPUEMUTEST, ORA_INS)
{
	INIT_CPUEMU;

	autoCC01Mapper(*defmap, cpu, OP_ORA);

	//Byte to perform ORA on for non-immediate addressing modes
	writeOpToMem(*defmap, 0x0F, 0x0000); //For indirect indexed X
	writeOpToMem(*defmap, 0x0F, 0x0001); //For indexed indirect Y

	//Indirect address
	writeOpToMem(*defmap, 0x00, 0x0002); //LSB
	writeOpToMem(*defmap, 0x00, 0x0003); //MSB
	writeOpToMem(*defmap, 0x00, 0x0004);

	for (int j = 0; j < SZ_CC01; j++)
	{
		cpu.accumulator.unsigned8 = 0xF0;
		cpu.xindex.unsigned8 = 0x01;
		cpu.yindex.unsigned8 = 0x01;
		int ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_ORA]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0xFF);
	}

	TEARDOWN_CPUEMU;
}

/* Test for AND instruction. Covers all addressing modes. */
TEST(CPUEMUTEST, AND_INS)
{
	INIT_CPUEMU;

	autoCC01Mapper(*defmap, cpu, OP_AND);

	for (int j = 0; j < SZ_CC01; j++)
	{
		ADDR_16B lastprogc = cpu.progcount;
		cpu.accumulator.unsigned8 = 0x0F;

		int ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_AND]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0x0F);

		cpu.accumulator.unsigned8 = 0xF0;
		cpu.progcount = lastprogc;
		ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_AND]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0x00);

		cpu.accumulator.unsigned8 = 0x00;
		cpu.progcount = lastprogc;
		ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_AND]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0x00);
	}

	TEARDOWN_CPUEMU;
}

/* Test for EOR (XOR) instruction. Covers all addressing modes. */
TEST(CPUEMUTEST, EOR_INS)
{
	INIT_CPUEMU;

	autoCC01Mapper(*defmap, cpu, OP_EOR);

	for (int j = 0; j < SZ_CC01; j++)
	{

		ADDR_16B lastprogc = cpu.progcount;
		cpu.accumulator.unsigned8 = 0x0F;

		int ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_EOR]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0x00);

		cpu.progcount = lastprogc;
		cpu.accumulator.unsigned8 = 0xF0;
		ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_EOR]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0xFF);

		cpu.progcount = lastprogc;
		cpu.accumulator.unsigned8 = 0x00;
		ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_EOR]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0x0F);
	}

	TEARDOWN_CPUEMU;
}

//$STUB$ add chained addition ops (adc -> reg + rhs; adc -> (result/reg + carry) + rhs)
/* Test for ADC instruction. Covers all addressing modes. */
TEST(CPUEMUTEST, ADC_INS)
{
	INIT_CPUEMU;

	autoCC01Mapper(*defmap, cpu, OP_ADC);

	for (int j = 0; j < SZ_CC01; j++)
	{

		ADDR_16B lastprogc = cpu.progcount;
		cpu.accumulator.signed8 = 1;
		cpu.procstat = 0x00;

		//Positive add (+, +)
		int ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_ADC]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.signed8, 16); // 0x01 (1) + 0x0F (15) = 16
		ASSERT_EQ(F_CARRY(cpu.procstat), 0);
		ASSERT_EQ(F_ZERO(cpu.procstat), 0);
		ASSERT_EQ(F_NEG(cpu.procstat), 0);
		ASSERT_EQ(F_OVRFLOW(cpu.procstat), 0);

		//Negative/positive add (-, +)
		cpu.procstat = 0x00;
		cpu.progcount = lastprogc;
		cpu.accumulator.unsigned8 = 0xF0;
		ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_ADC]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0xFF); // 0xF0 (-16) + 0x0F (15) = -1
		ASSERT_EQ(F_CARRY(cpu.procstat), 0);
		ASSERT_EQ(F_ZERO(cpu.procstat), 0);
		ASSERT_EQ(F_NEG(cpu.procstat), 1);
		ASSERT_EQ(F_OVRFLOW(cpu.procstat), 0);

		//Negative add (-, -)
		writeOpToMem(*defmap, 0xF1, 0x0000); //-15
		writeOpToMem(*defmap, 0xF1, 0x0001); //-15
		writeOpToMem(*defmap, 0xF1, 0x8005); //Replace IMMED operand
		cpu.procstat = 0x00;
		cpu.progcount = lastprogc;
		cpu.accumulator.unsigned8 = 0xF1;
		ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_ADC]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0xE2); // 0xF1 (-15) + 0xF1 (-15) = -30
		ASSERT_EQ(F_CARRY(cpu.procstat), 1);
		ASSERT_EQ(F_ZERO(cpu.procstat), 0);
		ASSERT_EQ(F_NEG(cpu.procstat), 1);
		ASSERT_EQ(F_OVRFLOW(cpu.procstat), 0);
		writeOpToMem(*defmap, 0x0F, 0x8005); //Undp IMMED operand
		writeOpToMem(*defmap, 0x0F, 0x0000); //Undo to 15
		writeOpToMem(*defmap, 0x0F, 0x0001); //Undo to 15

		//Integer Overflow
		cpu.procstat = 0x00;
		cpu.progcount = lastprogc;
		cpu.accumulator.unsigned8 = 0x71;
		ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_ADC]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0x80); // 0x71 (113) + 0x0F (15) = -128
		ASSERT_EQ(F_CARRY(cpu.procstat), 0);
		ASSERT_EQ(F_ZERO(cpu.procstat), 0);
		ASSERT_EQ(F_NEG(cpu.procstat), 1);
		ASSERT_EQ(F_OVRFLOW(cpu.procstat), 1);

		//Unsigned carry
		cpu.procstat = 0x00;
		cpu.progcount = lastprogc;
		cpu.accumulator.unsigned8 = 0xF1;
		ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_ADC]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0x00); // 0xF1 (-15) + 0x0F (15) = 0
		ASSERT_EQ(F_CARRY(cpu.procstat), 1);
		ASSERT_EQ(F_ZERO(cpu.procstat), 1);
		ASSERT_EQ(F_NEG(cpu.procstat), 0);
		ASSERT_EQ(F_OVRFLOW(cpu.procstat), 0);
	}

	TEARDOWN_CPUEMU;
}

//$STUB$ add chained subtraction ops (sbc -> reg - rhs; sbc -> (result/reg - carry) - rhs)
/* Test for SBC instruction. Covers all addressing modes. */
TEST(CPUEMUTEST, SBC_INS)
{
	INIT_CPUEMU;

	autoCC01Mapper(*defmap, cpu, OP_SBC);

	for (int j = 0; j < SZ_CC01; j++)
	{

		ADDR_16B lastprogc = cpu.progcount;
		cpu.accumulator.unsigned8 = 0x10;
		cpu.procstat = 0x00;

		//Positive/Positive subtract (+, +)
		int ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_SBC]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0x01); // 0x10 (16) - 0x0F (15) = 1
		ASSERT_EQ(F_CARRY(cpu.procstat), 1);
		ASSERT_EQ(F_ZERO(cpu.procstat), 0);
		ASSERT_EQ(F_NEG(cpu.procstat), 0);
		ASSERT_EQ(F_OVRFLOW(cpu.procstat), 0);

		//Negative/Positive subtract (-, +)
		cpu.procstat = 0x00;
		cpu.progcount = lastprogc;
		cpu.accumulator.unsigned8 = 0xF0;
		ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_SBC]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0xE1); // 0xF0 (-16) - 0x0F (15) = -31
		ASSERT_EQ(F_CARRY(cpu.procstat), 1);
		ASSERT_EQ(F_ZERO(cpu.procstat), 0);
		ASSERT_EQ(F_NEG(cpu.procstat), 1);
		ASSERT_EQ(F_OVRFLOW(cpu.procstat), 0);

		//Negative/Negative subtract (-, -)
		writeOpToMem(*defmap, 0xF1, 0x0000); //-15
		writeOpToMem(*defmap, 0xF1, 0x0001); //-15
		writeOpToMem(*defmap, 0xF1, 0x8005); //Replace IMMED operand
		cpu.procstat = 0x00;
		cpu.progcount = lastprogc;
		cpu.accumulator.unsigned8 = 0xF1;
		ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_SBC]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0x00); // 0xF1 (-15) - 0xF1 (-15) = 0
		ASSERT_EQ(F_CARRY(cpu.procstat), 1);
		ASSERT_EQ(F_ZERO(cpu.procstat), 1);
		ASSERT_EQ(F_NEG(cpu.procstat), 0);
		ASSERT_EQ(F_OVRFLOW(cpu.procstat), 0);
		writeOpToMem(*defmap, 0x0F, 0x8005); //Undp IMMED operand
		writeOpToMem(*defmap, 0x0F, 0x0000); //Undo to 15
		writeOpToMem(*defmap, 0x0F, 0x0001); //Undo to 15

		//Integer Overflow
		cpu.procstat = 0x00;
		cpu.progcount = lastprogc;
		cpu.accumulator.unsigned8 = 0x80;
		ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_SBC]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0x71); // 0x80 (-128) - 0x0F (15) = 113
		ASSERT_EQ(F_CARRY(cpu.procstat), 1);
		ASSERT_EQ(F_ZERO(cpu.procstat), 0);
		ASSERT_EQ(F_NEG(cpu.procstat), 0);
		ASSERT_EQ(F_OVRFLOW(cpu.procstat), 1);

		//No unsigned carry
		cpu.procstat = 0x00;
		cpu.progcount = lastprogc;
		cpu.accumulator.unsigned8 = 0x0E;
		ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_SBC]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0xFF); // 0x0E (14) - 0x0F (15) = -1
		ASSERT_EQ(F_CARRY(cpu.procstat), 0);
		ASSERT_EQ(F_ZERO(cpu.procstat), 0);
		ASSERT_EQ(F_NEG(cpu.procstat), 1);
		ASSERT_EQ(F_OVRFLOW(cpu.procstat), 0);
	}

	TEARDOWN_CPUEMU;
}

/* Test for STA instruction. Covers all addressing modes. */
TEST(CPUEMUTEST, STA_INS)
{
	INIT_CPUEMU;

	//Only one CC01 memory set opcode so local tables will suffice
	OPCODE CC01SETTBL[] = {
		AMODE_ABS | OP_STA,			//Absolute
		0x02,
		0x00,
		AMODE_ABSX | OP_STA,		//Absolute indexed w/ x
		0x01,
		0x00,
		AMODE_ABSY | OP_STA,		//Absolute indexed w/ y
		0x01,
		0x00,
		AMODE_ZPAGE | OP_STA,		//Zpage
		0x02,
		AMODE_ZPAGEX | OP_STA,		//Zpage indexed w/ x
		0x01,
		AMODE_ZPAGEINX | OP_STA,	//Indexed indirect
		0x04,
		AMODE_ZPAGEINY | OP_STA,	//Indirect indexed
		0x03
	};

	OPCODE CC01SETOpticks[] = {
		AMODE_ABS,
		AMODE_ABSX,
		AMODE_ABSY,
		AMODE_ZPAGE,
		AMODE_ZPAGEX,
		AMODE_ZPAGEINX,
		AMODE_ZPAGEINY
	};

	//Copy table to memory
	for (int k = 0; k < 17; k++)
		writeOpToMem(*defmap, CC01SETTBL[k], 0x8000 + k);

	//Setup zpage indirect references to 0x0001
	writeOpToMem(*defmap, 0x01, 0x0003);
	writeOpToMem(*defmap, 0x00, 0x0004);

	//Setup indexed indirect zpage reference to 0x0002
	writeOpToMem(*defmap, 0x02, 0x0005);
	writeOpToMem(*defmap, 0x00, 0x0006);

	cpu.xindex.unsigned8 = 0x01; //Indexed + 1 so that target is 0x0002 for write
	cpu.yindex.unsigned8 = 0x01;
	cpu.accumulator.unsigned8 = 0x0F;
	cpu.procstat = 0xFF;
	cpu.progcount = 0x8000;

	for (int j = 0; j < 7; j++)
	{
		//Reset 0x0002
		writeOpToMem(*defmap, 0x00, 0x0002);

		int ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01SETOpticks[j] | OP_STA]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0x0F); //Accumulator should not be affected
		ASSERT_EQ(defmap->readMemory(0x0002), 0x0F); //Assume write is correct
		ASSERT_EQ(cpu.procstat, 0xFF); //Status register flags should not be affected
	}

	TEARDOWN_CPUEMU;
}

/* Test for LDA instruction. Covers all addressing modes. */
TEST(CPUEMUTEST, LDA_INS)
{
	INIT_CPUEMU;

	autoCC01Mapper(*defmap, cpu, OP_LDA);

	for (int j = 0; j < SZ_CC01; j++)
	{
		cpu.accumulator.unsigned8 = 0x00;
		cpu.procstat = 0x00;

		int ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_LDA]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(cpu.accumulator.unsigned8, 0x0F);
		ASSERT_EQ(F_NEG(cpu.procstat), 0);
		ASSERT_EQ(F_ZERO(cpu.procstat), 0);
	}

	TEARDOWN_CPUEMU;
}

/* Test for CMP instruction. Covers all addressing modes. */
TEST(CPUEMUTEST, CMP_INS)
{
	INIT_CPUEMU;

	autoCC01Mapper(*defmap, cpu, OP_CMP);

	for (int j = 0; j < SZ_CC01; j++)
	{
		ADDR_16B lastprogc = cpu.progcount;
		cpu.procstat = 0x00;

		//0x0F = 0x0F
		cpu.accumulator.unsigned8 = 0x0F;
		int ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_CMP]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(F_NEG(cpu.procstat), 0);
		ASSERT_EQ(F_ZERO(cpu.procstat), 1);
		ASSERT_EQ(F_CARRY(cpu.procstat), 1);

		//0x10 > 0x0F
		cpu.progcount = lastprogc;
		cpu.accumulator.unsigned8 = 0x10;
		ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_CMP]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(F_NEG(cpu.procstat), 0);
		ASSERT_EQ(F_ZERO(cpu.procstat), 0);
		ASSERT_EQ(F_CARRY(cpu.procstat), 1);

		//0x0E < 0x0F
		cpu.progcount = lastprogc;
		cpu.accumulator.unsigned8 = 0x0E;
		ticksr = cpuemu.emulate_cpu(emu::OPTICK[CC01Opticks[j] | OP_CMP]);
		ASSERT_EQ(ticksr, 0);
		ASSERT_EQ(F_NEG(cpu.procstat), 1);
		ASSERT_EQ(F_ZERO(cpu.procstat), 0);
		ASSERT_EQ(F_CARRY(cpu.procstat), 0);
	}

	TEARDOWN_CPUEMU;
}