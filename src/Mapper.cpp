#include "Mapper.h"
#include "Debug.h"

using namespace emu;

//======================================================
//BadRomException
//======================================================

const char* BadRomException::what() const {
	switch (error)
	{
	case BADSIZE:
		return "ROM is too small.";
	case BADRBITS:
		return "Nonzero value(s) in reserved space.";
	case BADVSTD:
		return "Invalid video standard specified [NTSC(0)/PAL(1)].";
	case BADFILL:
		return "Nonzero value(s) in fill space.";
	case TRAINERUSED:
		return "This ROM uses a trainer. Trainers are not supported at this time.";
	case BADTAG:
		return "A proper NES[\x1A] tag is not present in the ROM header.";
	case NOPRG:
		return "This NES ROM lists 0 PRG-ROM banks.";
	case UNSUPPORTEDMAPPER:
		return "This NES ROM uses an unsupported mapper.";
	default:
		return "An unsupported error occured :(.";
	}
}

//======================================================
//Mapper
//======================================================

/* MEMMAP -> N** map (4k pages) to set
 * PAGEMAP -> N** page map (16k pages) to configure map for
 * PAGE -> PAGE number in PAGEMAP
 * HILOW -> Put page at HI=1 (0xC000) or LOW=0 (0x8000) 
 */
#define HI_PAGE 1
#define LOW_PAGE 0
#define SWAP_PAGE(MEMMAP, PAGEMAP, PAGE, HILOW) \
	MEMMAP[8 + 4*HILOW] = PAGEMAP[PAGE]; \
	MEMMAP[9 + 4*HILOW] = PAGEMAP[PAGE] + 0x1000; \
	MEMMAP[10 + 4*HILOW] = PAGEMAP[PAGE] + 0x2000; \
	MEMMAP[11 + 4*HILOW] = PAGEMAP[PAGE] + 0x3000;

//Minimum sensible ROM size.
const long MINROMSIZE = sizeof(INES_Header)+(unsigned)SZ_PRGROM_BLOCK + (unsigned)SZ_CHRROM_BLOCK;

/* Creates a mapper from an INES ROM stream. */
void Mapper::createMapper(std::istream& iNesRom, Mapper*& mapper)
{
	INES_Header nesh;

	//Get ROM size
	iNesRom.seekg(0, iNesRom.end);
	std::streampos romsize = iNesRom.tellg();
	romsize += 1;
	iNesRom.seekg(0, iNesRom.beg);
	
	//Make sure rom is at least 16 bytes + 1 PRG BLOCK + 1 CHR BLOCK
	if (romsize < MINROMSIZE)
		throw BadRomException(BadRomException::BADSIZE);
	
	iNesRom.read(nesh.NES, 4);
	nesh.NES[4] = '\0';

	
	//Validate that the NES\x1A tag is present.
	if (strcmp(nesh.NES, "NES\x1A") != 0)
		throw BadRomException(BadRomException::BADTAG);


	//Read iNES header
	iNesRom.read(reinterpret_cast<char*>(&nesh.cnt_prgblocks), 1);
	iNesRom.read(reinterpret_cast<char*>(&nesh.cnt_chrblocks), 1);
	iNesRom.read(reinterpret_cast<char*>(&nesh.rom_cr1), 1);
	iNesRom.read(reinterpret_cast<char*>(&nesh.rom_cr2), 1);
	iNesRom.read(reinterpret_cast<char*>(&nesh.cnt_rambanks), 1);
	iNesRom.read(reinterpret_cast<char*>(&nesh.vstndrd), 1);
	iNesRom.read(reinterpret_cast<char*>(&nesh.fill), 6);

	//Make sure reserved bits are zero'd
	if ((nesh.rom_cr2 & 0x0E) != 0)
		throw BadRomException(BadRomException::BADRBITS);
	//Adjust for older .NES versions
	if (nesh.cnt_rambanks == 0)
		nesh.cnt_rambanks = 1;

#define HAS_BATTERY BIT1(nesh.rom_cr1)
#define HAS_TRAINER BIT2(nesh.rom_cr1)
#define HAS_VSUNI BIT0(nesh.rom_cr2)
#define HAS_PLAYCHOICE BIT1(nesh.rom_cr2)
#define HAS_NES2 BIT2(nesh.rom_cr2)

#ifdef ENFORCE_NOTRAINER
	//Make sure a trainer is not included $STUB$ SUPPORT TRAINERS
	if (HAS_TRAINER)
		throw BadRomException(BadRomException::TRAINERUSED);
#endif
	//Check if SRAM is battery backed
	//if (HAS_BATTERY);
	//$STUB$
	//if (HAS_PLAYCHOICE);
	//$STUB$
	//if (HAS_VSUNI);
	//$STUB$
	//if (HAS_NES2);
	//$STUB
	if (nesh.cnt_prgblocks < 1)
		nesh.cnt_prgblocks = 1;

	//Determine video standard (NTSC = 0 / PAL = 1)
	if (nesh.vstndrd > 1)
		throw BadRomException(BadRomException::BADVSTD);

	//Make sure the fill is zero'd (standard)
	for (int i = 0; i < 5; i++){
		if (nesh.fill[i] != 0 && !HAS_NES2)
			throw BadRomException(BadRomException::BADFILL);
	}

	//Get mapper number from flags
	int mappernumber = (nesh.rom_cr1 >> 4) & (nesh.rom_cr2 & 0xF0);
	
	//Create Mapper
	switch (mappernumber)
	{
	case 0:
		mapper = new DefaultMapper();
		break;
	default:
		throw BadRomException(BadRomException::UNSUPPORTEDMAPPER);
	}
	
	//Initialize Mapper (duh)
	mapper->initialize();
	
	//Initialize Mapper PRG-ROM
	if (nesh.cnt_prgblocks > 2) //Bank Switching
	{
		mapper->rompages = new BYTE*[nesh.cnt_prgblocks];
		mapper->rp_count = nesh.cnt_prgblocks;

		//Load ROM Pages
		for (int i = 0; i < nesh.cnt_prgblocks; i++) {
			mapper->rompages[i] = new BYTE[(unsigned)SZ_PRGROM_BLOCK];
			iNesRom.read(reinterpret_cast<char*>(mapper->rompages[i]), (unsigned)SZ_PRGROM_BLOCK);
		}
		
		SWAP_PAGE(mapper->map, mapper->rompages, 0, LOW_PAGE);
		SWAP_PAGE(mapper->map, mapper->rompages, 1, HI_PAGE);
	}
	else                        //Direct Mapping
	{
		//Map blocks to memory
		mapper->rompages = NULL;
		mapper->rp_count = 0;
		BYTE* taddr = mapper->memory + (unsigned)L_PRGROM;
		iNesRom.read(reinterpret_cast<char*>(taddr) , (unsigned)SZ_PRGROM_BLOCK * nesh.cnt_prgblocks);

		//Mirror if one block
		if (nesh.cnt_prgblocks == 1)
			memcpy(taddr + (unsigned)SZ_PRGROM_BLOCK, taddr, (unsigned)SZ_PRGROM_BLOCK);
	}

	//Initialize SRAM
	//$STUB$ Has 8k (0x2000) bank size L_SRAM SZ_PRGRAM_BLOCK

	//Initialize Mapper CHR-ROM
	//$STUB$ Has 8K (0x2000) bank size
}

/* Initialize mapper arrays and set some default values. */
void Mapper::initialize() {
	//16 * 4Kb = 64Kb
	memory = new BYTE[0x10000];
	map = new BYTE*[16];
	map[0] = memory;
	for (int i = 1; i < 16; i++)
		map[i] = map[i - 1] + 0x1000;

	//Zero out the i/o registers
	memset(memory + L_IOREGBLOCK1, 0, SZ_IOREGBLOCK1);
	memset(memory + L_IOREGBLOCK2, 0, SZ_IOREGBLOCK2);
	//Set joysticks to defaults
	memory[L_JOYSTICK1] = 0x80;
	memory[L_JOYSTICK2] = 0x80;
	//Turn on sound channels
	memory[L_ENBLSND] = 0x1F;
}

Mapper::~Mapper() {
	delete[] map;
	delete[] rompages;
	delete memory;
}

//======================================================
//Default Mapper
//======================================================

/* Read a BYTE from mapper memory. */
BYTE DefaultMapper::readMemory(ADDR_16B addr) {
#ifdef ENFORCE_WRITEONLY_MINIMUM
	//$STUB$ More selectively include cases
	switch (addr)
	{
	case 0x2000:
	case 0x2001:
	case 0x2003:
	case 0x2005:
	case 0x2006:
	case 0x4000:
	case 0x4001:
	case 0x4002:
	case 0x4003:
	case 0x4004:
	case 0x4005:
	case 0x4006:
	case 0x4007:
	case 0x4008:
	case 0x4009:
	case 0x400A:
	case 0x400B:
	case 0x400C:
	case 0x400D:
	case 0x400E:
	case 0x400F:
	case 0x4010:
	case 0x4011:
	case 0x4012:
	case 0x4013:
	case 0x4014:
		throw BadReadException(addr);
	}
#endif
	return map[addr >> 12][addr & 0x0FFF];
}

/* Write a BYTE to mapper memory. */
void DefaultMapper::writeMemory(ADDR_16B addr, BYTE data) {
#ifdef ENFORCE_READONLY_STRICT
	if(addr == 0x2002)
		throw BadWriteException(addr);
#endif
#ifdef ENFORCE_READONLY_MINIMUM
	//Make sure we do not write to PRG-ROM or Expansion ROM
	if (addr >> 15 || addr >= 0x4020 && addr <= 0x5FFF)
		throw BadWriteException(addr);
#endif
#ifdef ENFORCE_STACK_MIRROR
	//Stack
	if(addr < 0x2000)
	{
		map[addr >> 12][addr & 0x07FF] = data;
		map[addr >> 12][(addr & 0x07FF) + 0x800] = data;
		map[addr >> 12][(addr & 0x07FF) + 0x1000] = data;
		map[addr >> 12][(addr & 0x07FF) + 0x1800] = data;
	}
#endif
	map[addr >> 12][addr & 0x0FFF] = data;
}