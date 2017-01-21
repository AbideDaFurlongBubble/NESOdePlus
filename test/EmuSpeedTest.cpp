#include "Emulator.h"
#include "Test.h"
#include "Helpers.h"
#include "Instructions.h"
#include <vector>
#include <chrono>
#include <iostream>

#define EMUSPEEDTEST EmulationSpeedTest

#define INIT_CPUEMU \
	CREATE_DEFMAP; \
	emu::CPU cpu; \
	emu::initializeCPU(cpu); \
	emu::Emulator2A03 cpuemu(*defmap, cpu); \
	cpu.progcount = 0x8000;

#define TEARDOWN_CPUEMU \
	TEARDOWN_DEFMAP;

TEST(EMUSPEEDTEST, GEN_SPEEDTEST_EMULOOP)
{
	INIT_CPUEMU;
	writeOpToMem(*defmap, OP_INX, 0x8000);
	writeOpToMem(*defmap, OP_JMPABS, 0x8001);
	writeOpToMem(*defmap, 0x00, 0x8002);
	writeOpToMem(*defmap, 0x80, 0x8003);

	std::chrono::time_point<std::chrono::system_clock> start, delta;
	start = std::chrono::system_clock::now();
	delta = start;
	std::chrono::duration<double, std::milli> runtime = delta - start;

	unsigned long emuticks = 0;
	unsigned long loopcycles = 0;

	while (runtime.count() < 1000) {
		loopcycles++;

		cpuemu.emulate_cpu(emu::OPTICK[OP_INX] + emu::OPTICK[OP_JMPABS]);
		emuticks += emu::OPTICK[OP_INX] + emu::OPTICK[OP_JMPABS];

		delta = std::chrono::system_clock::now();
		runtime = delta - start;
	}

	double megaticks = emuticks / 1000000.0;
	std::cout << std::endl;
	std::cout << "\tTotal runtime: " << runtime.count() << " ms\n";
	std::cout << "\tTotal MHz: " << std::setprecision(5) << megaticks << "\n";
	std::cout << "\tTotal Loop Cycles: " << loopcycles << std::endl << '\n';
	TEARDOWN_CPUEMU;
}

TEST(EMUSPEEDTEST, MATH_SPEEDTEST_EMULOOP)
{
	INIT_CPUEMU;
	writeOpToMem(*defmap, OP_ADC | AMODE_IMMED, 0x8000);
	writeOpToMem(*defmap, 0x0F, 0x8001);

	std::chrono::time_point<std::chrono::system_clock> start, delta;
	start = std::chrono::system_clock::now();
	delta = start;
	std::chrono::duration<double, std::milli> runtime = delta - start;

	unsigned long emuticks = 0;
	unsigned long loopcycles = 0;

	while (runtime.count() < 1000) {
		loopcycles++;
		
		cpu.accumulator.unsigned8 = 0xF0;
		cpu.progcount = 0x8000;
		cpuemu.emulate_cpu(emu::OPTICK[OP_ADC | AMODE_IMMED]);
		emuticks += emu::OPTICK[OP_ADC | AMODE_IMMED];

		delta = std::chrono::system_clock::now();
		runtime = delta - start;
	}

	double megaticks = emuticks / 1000000.0;
	std::cout << std::endl;
	std::cout << "\tTotal runtime: " << runtime.count() << " ms\n";
	std::cout << "\tTotal MHz: " << std::setprecision(5) << megaticks << "\n";
	std::cout << "\tTotal Loop Cycles: " << loopcycles << std::endl << '\n';
	TEARDOWN_CPUEMU;
}