#pragma once

#ifdef __MAPPER_H__
#error __MAPPER_H__ Already defined!
#else
#define __MAPPER_H__
#endif

#include "NTDef.h"
#include <exception>
#include <iostream>
#include <sstream>
#include <string>

namespace emu {

	class BadRomException : public std::exception {
	public:
		enum ErrorType {
			BADSIZE, BADRBITS, BADVSTD, BADFILL,
			BADTAG, NOPRG, TRAINERUSED, UNSUPPORTEDMAPPER
		};
		BadRomException(ErrorType err) : error(err) {};
		const char* what() const;
	private:
		ErrorType error;
	};

	class EmulationException : public std::exception {
	public:
		EmulationException(int loc) : memloc(loc) {};
		virtual void initMessage() = 0;
		const char* what() const {
			return msg.str().c_str();
		}
	protected:
		int memloc;
		std::stringstream msg;
	};

	class BadWriteException : public EmulationException {
	public:
		virtual void initMessage() {
			msg << "Bad write from memory location " << std::hex << memloc;
		}
	};

	class BadReadException : public EmulationException {
	public:
		virtual void initMessage() {
			msg << "Bad read from memory location " << std::hex << memloc;
		}
	};

	class Mapper {
	public:
		virtual ~Mapper();
		static void createMapper(std::istream& iNesRom, Mapper*& mapper);
		virtual BYTE readMemory(ADDR_16B addr) = 0;
		virtual void writeMemory(ADDR_16B addr, BYTE data) = 0;

		//Get a pointer to mapper memory (const)
		const BYTE* getMemory() const {
			return memory;
		}

		//Get a pointer to mapper pages (const)
		const BYTE * const * getMemoryPages() const {
			return rompages;
		}

		int getMemoryPageCount() const {
			return rp_count;
		}

	protected:
		int mapper_num;
		//Initialize the memory map into sixteen 4 Kb pages.
		void initialize();
		//4 kb page map of address space
		BYTE** map;
		//64 KB address space
		BYTE* memory;
		//Switchable PRG-ROM pages in 16 KB size - used by map to switch
		BYTE** rompages;
		//Get number of PRG-ROM pages
		int rp_count;
	};

	class DefaultMapper : public Mapper {
	public:
		virtual BYTE readMemory(ADDR_16B addr);
		virtual void writeMemory(ADDR_16B addr, BYTE data);
	};

}