#pragma once

#ifdef __TEST_H__
#error __TEST_H__ Already defined!
#else
#define __TEST_H__
#endif

#include <fstream>

#include "gtest/gtest.h"

#include "Debug.h"
#include "NTDef.h"

#if defined _WIN32 || defined _WIN64
#define TESTROM "testroms\\donkeykong.nes"
#else
#define TESTROM "testroms/donkeykong.nes"
#endif

#define CREATE_DEFMAP  \
	emu::Mapper* defmap = NULL; \
	std::ifstream rom(TESTROM, std::ifstream::binary); \
if (!rom.is_open()) FAIL(); \
	emu::Mapper::createMapper(rom, defmap)

#define TEARDOWN_DEFMAP \
	rom.close(); \
	delete defmap;
