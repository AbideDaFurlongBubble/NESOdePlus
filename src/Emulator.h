#pragma once

#ifdef __EMULATOR_H__
#error __EMULATOR_H__ Already defined!
#else
#define __EMULATOR_H__
#endif

#include "NTDef.h"
#include "Mapper.h"
#include "Debug.h"
#include <mutex>
#include <memory>
namespace emu {
	extern const BYTE OPTICK[];

	enum ERROR_STATE { NONE, CPU_LOCK, UNKNOWN_INSTRUCTION };

	struct CPU {
		REG_S16B progcount;
		REG_S8B accumulator;
		REG_S8B xindex;
		REG_S8B yindex;
		REG_S8B stackp;
		uint8_t procstat;
		REG_S8B temp;
		REG_S8B debug;
	};

	void initializeCPU(CPU& cpu);

	class Emulator2A03 {
	public:
		Emulator2A03(Mapper& mappa, CPU& proc) : 
			cpu(proc), mapper(mappa), 
			stopemulation(false),  errstate(ERROR_STATE::NONE)
		{};
		/* Emulate the CPU for a specified number of cycles.
		 * @exec_ticks A parameter that specifies the number of clock cycles to execute.
		 * @return A number >= 0 specifies the number of ticks left. A number less than 0
		 * means an error occured.
		 */
		int emulate_cpu(int exec_ticks);
		/* Returns the number of cycles emulated thus far. */
		int getCycleCount() const { return clocks_used; }
		/* Stop the CPU and return the remaining ticks. Is thread-safe. */
		int stopEmulation();
		/* Return a copy of the CPU. */
		CPU getCopyCPU() const { return cpu; }
		/* Set the CPU parameters. */
		void setCPU(CPU& proc) { cpu = proc; }
		/* Return a copy of the last CPU error state. */
		ERROR_STATE getErrorState() const { return errstate; }
	private:
		long clocks_used;
		ERROR_STATE errstate;
		int ticks_remaining;
		std::mutex ticks_remaining_m;
		Mapper& mapper;
		CPU& cpu;
		bool stopemulation;
	};

}