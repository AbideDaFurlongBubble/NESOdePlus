#define IGNORE_STACK_MIRROR

#include "Mapper.h"
#include "Test.h"


//======================================================
//Tests for the Default Mapper
//======================================================

#define DEFAULTMAPPERTEST DefaultMapperTest

TEST(MapperTest, initTest) {
	CREATE_DEFMAP;

	//Check beg and end
	EXPECT_EQ(defmap->getMemory()[L_PRGROM], 0x20);
	EXPECT_EQ(defmap->getMemory()[L_PRGROM + 0x20], 0x01);
	EXPECT_EQ(defmap->getMemory()[L_PRGROM + SZ_PRGROM_BLOCK - 1], 0xFF);

	//Check mirror
	EXPECT_EQ(defmap->getMemory()[L_PRGROM + SZ_PRGROM_BLOCK], 0x20);
	EXPECT_EQ(defmap->getMemory()[L_PRGROM + SZ_PRGROM_BLOCK + 0x20], 0x01);
	EXPECT_EQ(defmap->getMemory()[L_PRGROM + SZ_PRGROM_BLOCK * 2 - 1], 0xFF);

	TEARDOWN_DEFMAP;
}

TEST(DEFAULTMAPPERTEST, writeTest) {
	CREATE_DEFMAP;

	//Test write to zeropage with getMemory
	ADDR_16B addr = L_ZPAGE;
	BYTE data = 5;
	defmap->writeMemory(addr, data);
	ASSERT_EQ(defmap->getMemory()[L_ZPAGE], data);

#ifdef ENFORCE_READONLY_STRICT
	addr = 0x2002;
	data = 5;
	ASSERT_THROW(defmap->writeMemory(addr,data), emu::BadWriteException);
#endif

#ifdef ENFORCE_READONLY_MINIMUM
	addr = 0x8002;
	data = 95;
	ASSERT_THROW(defmap->writeMemory(addr, data), emu::BadWriteException);
#endif

#ifdef ENFORCE_STACK_MIRROR
	addr = 0x10;
	data = 15;
	defmap->writeMemory(addr, data);
	ASSERT_EQ(defmap->getMemory()[0x0010], data);
	ASSERT_EQ(defmap->getMemory()[0x0810], data);
	ASSERT_EQ(defmap->getMemory()[0x1010], data);
	ASSERT_EQ(defmap->getMemory()[0x1810], data);
#endif

	TEARDOWN_DEFMAP;
}


TEST(DEFAULTMAPPERTEST, readTest) {
	CREATE_DEFMAP;

	//Test write to zeropage with getMemory
	ADDR_16B addr = L_ZPAGE;
	BYTE data = 22;
	defmap->writeMemory(addr, data);

	//Check if read is valid
	ASSERT_EQ(defmap->readMemory(addr), data);

#ifdef ENFORCE_STACK_MIRROR
	addr = 0x0025;
	data = 15;
	defmap->writeMemory(addr, data);
	ASSERT_EQ(defmap->readMemory(addr), data);
	ASSERT_EQ(defmap->readMemory(addr + 0x0800), data);
	ASSERT_EQ(defmap->readMemory(addr + 0x1000), data);
	ASSERT_EQ(defmap->readMemory(addr + 0x1800), data);
#endif

	//$STUB$ include ENFORCE_WRITEONLY_MINIMUM case

	TEARDOWN_DEFMAP;
}


TEST(DEFAULTMAPPERTEST, pagecountTest) {
	CREATE_DEFMAP;
	ASSERT_EQ(defmap->getMemoryPageCount(), 0);
	TEARDOWN_DEFMAP;
}


TEST(DEFAULTMAPPERTEST, memorypageTest) {
	CREATE_DEFMAP;
	ASSERT_EQ(const_cast<BYTE**>(defmap->getMemoryPages()), (unsigned char** const)NULL);
	TEARDOWN_DEFMAP;
}