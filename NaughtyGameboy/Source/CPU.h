#pragma once

#include "PCH.h"
#include "MMU.h"

class CPU
{
private:
	// The Flag Register(lower 8bit of AF register)
	// Bit  Name  Set  Clr  Expl.
	// 7    z(zf) Z    NZ   Zero Flag
	// 6    n     -    -    Add / Sub - Flag (BCD)
	// 5    h     -    -    Half Carry Flag(BCD)
	// 4    c(cf) C    NC   Carry Flag
	// 3-0  -     -    -    Not used(always zero)

	// The Zero Flag(Z)
	// This bit becomes set(1) if the result of an operation has been zero(0).Used for conditional jumps.

	// The Carry Flag(C, or Cy)
	// Becomes set when the result of an addition became bigger than FFh(8bit) or FFFFh(16bit).
	// Or when the result of a subtraction or comparision became less than zero(much as for Z80 and 80x86 CPUs, but unlike as for 65XX and ARM CPUs).
	// Also the flag becomes set when a rotate / shift operation has shifted - out a "1" - bit.
	// Used for conditional jumps, and for instructions such like ADC, SBC, RL, RLA, etc.

	// The BCD Flags(N, H)
	// - These flags are(rarely) used for the DAA instruction only,
	//   N Indicates whether the previous instruction has been an addition or subtraction,
	//   and H indicates carry for lower 4bits of the result, also for DAA, the C flag must indicate carry for upper 8bits.
	// - After adding / subtracting two BCD numbers, DAA is intended to convert the result into BCD format;
	//   BCD numbers are ranged from 00h to 99h rather than 00h to FFh.
	// - Because C and H flags must contain carry - outs for each digit,
	//   DAA cannot be used for 16bit operations(which have 4 digits), or for INC / DEC operations(which do not affect C - flag).
	static const byte ZeroFlag;
	static const byte SubtractFlag;
	static const byte HalfCarryFlag;
	static const byte CarryFlag;

	static const byte ZeroFlagMask;
	static const byte SubtractFlagMask;
	static const byte HalfCarryFlagMask;
	static const byte CarryFlagMask;
	static const byte AllFlagsMask;

private:
	ulong m_cycles; // Total cycles
	bool m_isHalted;
	byte m_IME; // Interrupt master enabled

	// Registers
	ushort m_AF; // Accumulator & Flags;
	ushort m_BC; // General purpose
	ushort m_DE; // General purpose
	ushort m_HL; // General purpose
	ushort m_SP; // Stack pointer
	ushort m_PC; // Program counter

	byte* m_byteRegisterMap[0x08]; // For easy access to the 8bit registers (A, B, C, D, E, H, L). F is not used
	ushort* m_ushortRegisterMap[0x04]; // For easy access to the 16bit registers (BC, DE, HL, SP)

	std::unique_ptr<MMU> m_MMU;

	typedef ulong(CPU::*InstructionFunction)(byte opcode);
	InstructionFunction m_instructionMap[0x100];
	InstructionFunction m_instructionMapCB[0x100];

public:
	CPU();

	/** Returns the number of cycles each step takes */
	ulong Step();

private:
	/** Read 1 byte and increment PC by 1 */
	byte ReadBytePCI();

	/** Read 2 bytes and increment PC by 2 */
	ushort ReadUShortPCI();

	void InitInstructionMap();

	/** Get an 8bit source register mapped to an opcode */
	byte* GetByteRegister_Src(byte opcode);

	/** Get an 8bit destination register mapped to an opcode  */
	byte* GetByteRegister_Dst(byte opcode);

	/** Get a 16bit register mapped to an opcode */
	ushort* GetUShortRegister(byte opcode);

	/** Push 1 byte to the stack */
	void PushByteToStack(byte value);

	/** Push 1 ushort to the stack */
	void PushUShortToStack(ushort value);

	/** Pop 1 byte from the stack */
	byte PopByteFromStack();

	/** Pop 1 ushort from the stack */
	ushort PopUShortFromStack();

	/** Get a flag in the F register */
	byte GetFlag(byte flag);

	/** Set a flag in the F register */
	void SetFlag(byte flag);

	/** Clear a flag in the F register */
	void ClearFlag(byte flag);

	/** Checks if a flag in the F register is set (1) */
	bool IsFlagSet(byte flag);

	/** Adds 2 bytes and sets/clears the flags in the F register */
	byte AddBytes_Two(byte b1, byte b2, byte affectedFlags = AllFlagsMask);

	/** Adds 3 bytes and sets/clears the flags in the F register */
	byte AddBytes_Three(byte b1, byte b2, byte b3, byte affectedFlags = AllFlagsMask);

	/** Adds 2 ushorts and sets/clears the flags in the F register */
	ushort AddUShorts_Two(ushort s1, ushort s2, byte affectedFlags = AllFlagsMask);

	/** Subtracts 2 bytes and sets/clears the flags in the F register */
	byte SubtractBytes_Two(byte b1, byte b2, byte affectedFlags = AllFlagsMask);

	/** Subtracts 3 bytes and sets/clears the flags in the F register */
	byte SubtractBytes_Three(byte b1, byte b2, byte b3, byte affectedFlags = AllFlagsMask);

	/** Subtracts 2 ushorts and sets/clears the flags in the F register */
	ushort SubtractUShorts_Two(ushort s1, ushort s2, byte affectedFlags = AllFlagsMask);

	/** Compares 2 bytes and return a flags byte */
	byte CompareBytes(byte b1, byte b2);

	/**
	* Rotate a byte left, and set/clear the flags in the F register.
	* The 7th bit is put back into position 0. The 7th bit also goes to the carry flag.
	*/
	byte RotateLeft(byte b, bool clearZeroFlag = false);

	/**
	* Rotate a byte left through carry flag, and set/clear the flags in the F register.
	* The 7th bit is loaded into the carry flag. The old carry is put in position 0.
	*/
	byte RotateLeftThroughCarry(byte b, bool clearZeroFlag = false);

	/**
	* Rotate a byte right, and set/clear the flags in the F register.
	* The 0th bit is put back into position 7. The 0th bit also goes to the carry flag.
	*/
	byte RotateRight(byte b, bool clearZeroFlag = false);

	/**
	* Rotate a byte right through carry flag, and set/clear the flags in the F register.
	* The 0th bit is loaded into the carry flag. The old carry is put in position 7.
	*/
	byte RotateRightThroughCarry(byte b, bool clearZeroFlag = false);

	// ===============
	// INSTRUCTION SET
	// ===============
	// NOTES:
	// - r/R - 8bit register
	// - rr - 16bit register
	// - n - 8bit data
	// - nn - 16bit data
	// - dd - 8bit signed data
	// - 0x in the method names means indirect addressing.
	// - 0xHL (HL) - the address pointed to by the HL register
	// - 0xnn (nn) - the address pointed to by the next 16bit data in memory
	// - 0xFF00 (FF00) - the memory address FF00
	// ===============

	// =======================
	// 8bit load instructions
	// =======================

	/** Load 8bit register R into 8bit register r */
	ulong LD_r_R(byte opcode);

	/** Load byte n into 8bit register r */
	ulong LD_r_n(byte opcode);

	/** Load the byte at address (HL) into 8bit register r */
	ulong LD_r_0xHL(byte opcode);

	/** Load 8bit register r into address (HL) */
	ulong LD_0xHL_r(byte opcode);

	/** Load byte n into address (HL) */
	ulong LD_0xHL_n(byte opcode);

	/** Load the byte at address (BC) into register A */
	ulong LD_A_0xBC(byte opcode);

	/** Load the byte at address (DE) into register A */
	ulong LD_A_0xDE(byte opcode);

	/** Load the byte at address (nn) into register A */
	ulong LD_A_0xnn(byte opcode);

	/** Load register A into address (BC) */
	ulong LD_0xBC_A(byte opcode);

	/** Load resiger A into address (DE) */
	ulong LD_0xDE_A(byte opcode);

	/** Load register A into address (nn) */
	ulong LD_0xnn_A(byte opcode);

	/** Read from IO port n (memory FF00+n) */
	ulong LD_A_0xFF00n(byte opcode);

	/** Write to IO port n (memory FF00+n) */
	ulong LD_0xFF00n_A(byte opcode);

	/** Read from IO port C (memory FF00+C) */
	ulong LD_A_0xFF00C(byte opcode);

	/** Write to IO port C (memory FF00+C) */
	ulong LD_0xFF00C_A(byte opcode);

	/** Load register A into address (HL), and increment HL */
	ulong LDI_0xHL_A(byte opcode);

	/** Load the byte at address (HL) into register A, and increment HL */
	ulong LDI_A_0xHL(byte opcode);

	/** Load register A into address (HL), and decrement HL */
	ulong LDD_0xHL_A(byte opcode);

	/** Load the byte at address (HL) into register A, and decrement HL */
	ulong LDD_A_0xHL(byte opcode);

	// =======================
	// 16bit load instruction
	// =======================

	/** Load ushort nn into 16bit register rr */
	ulong LD_rr_nn(byte opcode);

	/** Load register HL into register SP */
	ulong LD_SP_HL(byte opcode);

	/** Push 16bit register rr into the stack */
	ulong PUSH_rr(byte opcode);

	/** Pop 2 bytes from the stack and load them into 16bit register rr */
	ulong POP_rr(byte opcode);

	// =====================================
	// 8bit arithmetic/logical instructions
	// =====================================

	/** A = A + r */
	ulong ADD_A_r(byte opcode);

	/** A = A + n */
	ulong ADD_A_n(byte opcode);

	/** A = A + (HL) */
	ulong ADD_A_0xHL(byte opcode);

	/** A = A + r + cf */
	ulong ADC_A_r(byte opcode);

	/** A = A + n + cf */
	ulong ADC_A_n(byte opcode);

	/** A = A + (HL) + cf */
	ulong ADC_A_0xHL(byte opcode);

	/**
	* A = A - r
	* In all resources I've read it's "SUB r". The A is omitted. I've put it for consistency with the ADD instructions
	*/
	ulong SUB_A_r(byte opcode);

	/**
	* A = A - n
	* In all resources I've read it's "SUB n". The A is omitted. I've put it for consistency with the ADD instructions
	*/
	ulong SUB_A_n(byte opcode);

	/**
	* A = A - (HL)
	* In all resources I've read it's "SUB (HL)". The A is omitted. I've put it for consistency with the ADD instructions
	*/
	ulong SUB_A_0xHL(byte opcode);

	/** A = A - r - cf */
	ulong SBC_A_r(byte opcode);

	/** A = A - n - cf */
	ulong SBC_A_n(byte opcode);

	/** A = A - (HL) - cf */
	ulong SBC_A_0xHL(byte opcode);

	/** A = A & r */
	ulong AND_r(byte opcode);

	/** A = A & n */
	ulong AND_n(byte opcode);

	/** A = A & (HL) */
	ulong AND_0xHL(byte opcode);

	/** A = A ^ r */
	ulong XOR_r(byte opcode);

	/** A = A ^ n */
	ulong XOR_n(byte opcode);

	/** A = A ^ (HL) */
	ulong XOR_0xHL(byte opcode);

	/** A = A | r */
	ulong OR_r(byte opcode);

	/** A = A | n */
	ulong OR_n(byte opcode);

	/** A = A | (HL) */
	ulong OR_0xHL(byte opcode);

	/** Compare A - r */
	ulong CP_r(byte opcode);

	/** Compare A - n */
	ulong CP_n(byte opcode);

	/** Compare A - (HL) */
	ulong CP_0xHL(byte opcode);

	/** r = r + 1 */
	ulong INC_r(byte opcode);

	/** (HL) = (HL) + 1 */
	ulong INC_0xHL(byte opcode);

	/** r = r - 1 */
	ulong DEC_r(byte opcode);

	/** (HL) = (HL) - 1 */
	ulong DEC_0xHL(byte opcode);

	/** This instruction conditionally adjusts the accumulator for BCD (binary coded decimal) addition and subtraction operations */
	ulong DAA(byte opcode);

	/** A = A XOR 0xFF (all 0's become 1's, and all 1's become 0's) */
	ulong CPL(byte opcode);

	// =====================================
	// 16bit arithmetic/logical instructions
	// =====================================

	/** HL = HL + rr */
	ulong ADD_HL_rr(byte opcode);

	/** rr = rr + 1 */
	ulong INC_rr(byte opcode);

	/** rr = rr - 1 */
	ulong DEC_rr(byte opcode);

	/** SP = SP +- dd */
	ulong ADD_SP_dd(byte opcode);

	/** HL = SP +- dd */
	ulong LD_HL_SPdd(byte opcode);

	// =============================
	// Rotate and shift instructions
	// =============================

	/** Rotate A left */
	ulong RLCA(byte opcode);

	/** Rotate A left through carry */
	ulong RLA(byte opcode);

	/** Rotate A right */
	ulong RRCA(byte opcode);

	/** Rotate A right through carry */
	ulong RRA(byte opcode);

	/** Rotate r left */
	ulong RLC_r(byte opcode);

	/** Rotate (HL) left */
	ulong RLC_0xHL(byte opcode);

	/** Rotate r left through carry */
	ulong RL_r(byte opcode);

	/** Rotate (HL) left through carry */
	ulong RL_0xHL(byte opcode);

	/** Rotate r right */
	ulong RRC_r(byte opcode);

	/** Rotate (HL) right */
	ulong RRC_0xHL(byte opcode);

	/** ROtate r right through carry */
	ulong RR_r(byte opcode);

	/** Rotate (HL) right through carry */
	ulong RR_0xHL(byte opcode);

	/** Shift r left arithmetic (b0 = 0) */
	ulong SLA_r(byte opcode);

	/** Shift (HL) left arithmetic (b0 = 0) */
	ulong SLA_0xHL(byte opcode);

	/** Shift r right arithmetic (b7 = b7) */
	ulong SRA_r(byte opcode);

	/** Shift (HL) right arithmetic (b7 = b7) */
	ulong SRA_0xHL(byte opcode);

	/** Shift r right logical (b7 = 0) */
	ulong SRL_r(byte opcode);

	/** Shift (HL) logical (b7 = 0) */
	ulong SRL_0xHL(byte opcode);

	/** Swap the low/high nibbles of r */
	ulong SWAP_r(byte opcode);

	/** Swap the low/high nibbles of (HL) */
	ulong SWAP_0xHL(byte opcode);

	// =======================
	// Single bit instructions
	// =======================

	/** Test bit n in r */
	ulong BIT_n_r(byte opcode);

	/** Test bit n in (HL) */
	ulong BIT_n_0xHL(byte opcode);

	/** Set bit n in r */
	ulong SET_n_r(byte opcode);

	/** Set bit n in (HL) */
	ulong SET_n_0xHL(byte opcode);

	/** Clear bit n in r */
	ulong RES_n_r(byte opcode);

	/** Clear bit n in (HL) */
	ulong RES_n_0xHL(byte opcode);

	// ==================
	// Control instructions
	// ==================

	/** Complement carry flag (cf = cf XOR 1) */
	ulong CCF(byte opcode);

	/** Set carry flag (cf = 1) */
	ulong SCF(byte opcode);

	/** No operation */
	ulong NOP(byte opcode);

	/** Halt until interrupt occurs */
	ulong HALT(byte opcode);

	/** Stop */
	ulong STOP(byte opcode);

	/** Disable interrupts (IME = 0) */
	ulong DI(byte opcode);

	/** Enable interrupts (IME = 1) */
	ulong EI(byte opcode);
};
