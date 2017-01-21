#pragma once

#ifdef __NTDEF_H__
#error __NTDEF_H__ Already defined!
#else
#define __NTDEF_H__
#endif

#include <stdint.h>

#ifndef REG_S8B
//Type definition for 8 bit 6502 registers
union REG_S8B {
	uint8_t unsigned8;
	int8_t signed8;
};
#endif

#ifndef MEM_BYTE
typedef REG_S8B MEM_BYTE;
#endif

#ifndef REG_S16B
//Type definition for 16 bit 6502 registers
typedef uint16_t REG_S16B;
#endif

#ifndef BYTE
//Type definition for a standard 8-bit byte
typedef uint8_t BYTE;
#endif

#ifndef P_VPPU
typedef void* P_VPPU;
#endif

#ifndef P_VSPU
typedef void* P_VSPU;
#endif

#ifndef MEM_CARD
typedef void* MEM_CARD;
#endif

#ifndef NULL
#define NULL 0
#endif

typedef REG_S16B ADDR_16B;
typedef BYTE OPCODE;

//Get Bits
#define BIT(x,y)(((x)>>y & 1))
#define BIT0(x)	(BIT(x,0))
#define BIT1(x) (BIT(x,1))
#define BIT2(x) (BIT(x,2))
#define BIT3(x)	(BIT(x,3))
#define BIT4(x) (BIT(x,4))
#define BIT5(x) (BIT(x,5))
#define BIT6(x)	(BIT(x,6))
#define BIT7(x) (BIT(x,7))

//Set Bits
#define SETBIT(x,y,z)	(x = (x & ~(1<<z)) | ((y)<<z))
#define SETBIT0(x,y)	(SETBIT(x,y,0))
#define SETBIT1(x,y)	(SETBIT(x,y,1))
#define SETBIT2(x,y)	(SETBIT(x,y,2))
#define SETBIT3(x,y)	(SETBIT(x,y,3))
#define SETBIT4(x,y)	(SETBIT(x,y,4))
#define SETBIT5(x,y)	(SETBIT(x,y,5))
#define SETBIT6(x,y)	(SETBIT(x,y,6))
#define SETBIT7(x,y)	(SETBIT(x,y,7))

//ROM Control Byte 1 Flags
#define F_MIRRORING(x)	(BIT0(x))
#define F_HASBRAM(x)	(BIT1(x))
#define F_HAS512T(x)	(BIT2(x))
#define F_USE4SCRN(x)	(BIT3(x))
#define F_LOBMAPN(x)	(x & 0xF0)

//ROM Control Byte 2 Flags
#define F_RCBRES(x)		(x & 0x0F)
#define F_HIBMAPN(x)	(x & 0xF0)

//INES header for NES ROMs
struct INES_Header {
	char NES[5];
	BYTE cnt_prgblocks;
	BYTE cnt_chrblocks;
	BYTE rom_cr1;
	BYTE rom_cr2;
	BYTE cnt_rambanks;
	BYTE vstndrd;
	BYTE fill[6];
};

#define CARRY_BIT	0x01
#define ZERO_BIT	0x02
#define INTDIS_BIT	0x04
#define DMODE_BIT	0x08
#define BRK_BIT		0x10
#define OVRFLOW_BIT 0x40
#define NEG_BIT		0x80

//Status Register Flags
#define F_CARRY(x)	(BIT0(x))
#define F_ZERO(x)	(BIT1(x))
#define F_INTDIS(x)	(BIT2(x))
#define F_DMODE(x)	(BIT3(x))
#define F_BRKCMD(x)	(BIT4(x))
#define F_OVRFLOW(x)(BIT6(x))
#define F_NEG(x)	(BIT7(x))
#define SETF_CARRY(x,y)		(SETBIT0(x,y))
#define SETF_ZERO(x,y)		(SETBIT1(x,y))
#define SETF_INTDIS(x,y)	(SETBIT2(x,y))
#define SETF_DMODE(x,y)		(SETBIT3(x,y))
#define SETF_BRKCMD(x,y)	(SETBIT4(x,y))
#define SETF_OVRFLOW(x,y)	(SETBIT6(x,y))
#define SETF_NEG(x,y)		(SETBIT7(x,y))

//non-standard
#define F_DEBUG(x)	(BIT5(x))
#define SETF_DEBUG(x,y)		(SETBIT5(x,y))

//PPU Control Register 1 Flags
#define F_NTA(x)		(x & 0x3)
#define F_VERTWRT(x)	(BIT2(x))
#define F_SPRPTA(x)		(BIT3(x))
#define F_SCRPTA(x)		(BIT4(x))
#define F_SPRSIZE(x)	(BIT5(x))
#define F_PPUMSM(x)		(BIT6(x))
#define F_VBLANKENB(x)	(BIT7(x))

//PPU Control Register 2 Flags
#define F_IMGMASK(x)	(BIT1(x))
#define F_SPRMASK(x)	(BIT2(x))
#define F_SCRENABLE(x)	(BIT3(x))
#define F_SPRENABLE(x)	(BIT4(x))
#define F_BGCOLOR(x)	(x & 0xE0)

//PPU Status Register Flags
#define F_HITF(x)		(BIT6(x))
#define F_VBLANK(x)		(BIT7(x))

//Sound Channel Switch Flags
#define F_SCHANL1(x)	(BIT0(x))
#define F_SCHANL2(x)	(BIT1(x))
#define F_SCHANL3(x)	(BIT2(x))
#define F_SCHANL4(x)	(BIT3(x))
#define F_SCHANL5(x)	(BIT4(x))

//Joystick Flags
#define F_JSTICKDATA(x)	(BIT0(x))
#define F_JSTICKPRES(x)	(BIT1(x))

//Memory Map
#define L_ZPAGE		0x0000
#define L_RAM		0x0200
#define L_SRAM		0x6000
#define L_PRGROM	0x8000

#define L_IOREGBLOCK1	0x2000
#define L_IOREGBLOCK2	0x4000

#define SZ_PRGROM_BLOCK	0x4000
#define SZ_PRGRAM_BLOCK 0x2000
#define SZ_CHRROM_BLOCK 0x2000

#define SZ_IOREGBLOCK1	0x2000
#define SZ_IOREGBLOCK2	0x0020

#define L_JOYSTICK1		0x4016
#define	L_JOYSTICK2		0x4017
#define L_ENBLSND		0x4015
#define L_STACKT		0x0100

#define L_NMIHNDL	0xFFFA
#define L_PORHNDL	0xFFFC
#define L_BRKHNDL	0xFFFE