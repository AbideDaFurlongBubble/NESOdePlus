#pragma once

#ifdef __INSTRUCTIONS_H__
#error __INSTRUCTIONS_H__ Already defined!
#else
#define __INSTRUCTIONS_H__
#endif

/*==================================
 * Opcode Masks
 * In the 6502 architecture, opcodes are based on a simple
 * three field bit pattern (AAA-BBB-CC).
 * AAA and CC collectively determine the operation and BBB
 * determines the address mode utilized by the operation.
 *==================================*/

//Mask for opcode bits in instruction
#define MASK_AAACC 0xE3
//Mask for address mode bits in instruction
#define MASK_BBB 0x1C
//CC defines
#define CMODE_01 0x01
#define CMODE_10 0x02
#define CMODE_00 0x00

//CC : 01
#define OP_ORA 0x01
#define OP_AND 0x21
#define OP_EOR 0x41
#define OP_ADC 0x61
#define OP_STA 0x81
#define OP_LDA 0xA1
#define OP_CMP 0xC1
#define OP_SBC 0xE1

//BBB : CC : 01
#define AMODE_ZPAGEINX	0x00
#define AMODE_ZPAGE		0x04
#define AMODE_IMMED		0x08
#define AMODE_ABS		0x0C
#define AMODE_ZPAGEINY	0x10
#define AMODE_ZPAGEX	0x14
#define AMODE_ABSY		0x18
#define AMODE_ABSX		0x1C

//CC : 10
#define OP_ASL 0x02
#define OP_ROL 0x22
#define OP_LSR 0x42
#define OP_ROR 0x62
#define OP_STX 0x82
#define OP_LDX 0xA2
#define OP_DEC 0xC2
#define OP_INC 0xE2

//BBB : CC : 10
//Immediate Address Mode
#define AMODE_10IMMED		0x00
//Accumulator Address Mode
#define AMODE_ACCUM			0x08
#define AMODE_10ABSX		0x1C
#define AMODE_10ZPAGEX		0x14

//CC : 00
#define OP_BIT		0x20
#define OP_JMP		0x4C
#define OP_JMPABS	0x6C
#define OP_STY		0x80
#define OP_LDY		0xA0
#define OP_CPY		0xC0
#define OP_CPX		0xE0

//-----------------------------------
//-----Single byte instructions------
//-----------------------------------
#define OP_INX 0xE8
#define OP_INY 0xC8
#define OP_TAY 0xA8
#define OP_DEY 0x88
#define OP_PLA 0x68
#define OP_PHA 0x48
#define OP_PLP 0x28
#define OP_PHP 0x08

#define OP_CLC 0x18
#define OP_SEC 0x38
#define OP_CLI 0x58
#define OP_SEI 0x78
#define OP_TYA 0x98
#define OP_CLV 0xB8
#define OP_CLD 0xD8
#define OP_SED 0xF8

#define OP_TXA 0x8A
#define OP_TXS 0x9A
#define OP_TAX 0xAA
#define OP_TSX 0xBA
#define OP_DEX 0xCA
#define OP_NOP 0xEA

#define OP_BRK		0x00
#define OP_JSRABS	0x20
#define OP_RTI		0x40
#define OP_RTS		0x60

//Branching instructions

#define OP_BPL		0x10
#define OP_BMI		0x30
#define OP_BVC		0x50
#define OP_BVS		0x70
#define OP_BCC		0x90
#define OP_BCS		0xB0
#define OP_BNE		0xD0
#define OP_BEQ		0xF0

//-----------------------------------
//-----Undocumented instructions-----
//-----------------------------------

//NOP
#define OP_NOP0 0x1A
#define OP_NOP1 0x3A
#define OP_NOP2 0x5A
#define OP_NOP3 0x7A
#define OP_NOP4 0xDA
#define OP_NOP5 0xFA

//Double NOP
#define OP_DNOP0 0x04
#define OP_DNOP1 0x14
#define OP_DNOP2 0x34
#define OP_DNOP3 0x44
#define OP_DNOP4 0x54
#define OP_DNOP5 0x64
#define OP_DNOP6 0x74
#define OP_DNOP7 0x80
#define OP_DNOP8 0x82
#define OP_DNOP9 0x89
#define OP_DNOPA 0xC2
#define OP_DNOPB 0xD4
#define OP_DNOPC 0xE2
#define OP_DNOPD 0xF4

//KIL
#define OP_KIL0 0x02
#define OP_KIL1 0x12
#define OP_KIL2 0x22
#define OP_KIL3 0x32
#define OP_KIL4 0x42
#define OP_KIL5 0x52
#define OP_KIL6 0x62
#define OP_KIL7 0x72
#define OP_KIL8 0x92
#define OP_KIL9 0xB2
#define OP_KILA 0xD2
#define OP_KILB 0xF2

//Pad Byte
#define OP_PADB 0x00