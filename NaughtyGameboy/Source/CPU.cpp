#include "CPU.h"

CPU::CPU()
{
	
	// In binary ##dddsss, where ddd is the DST register, and sss is the SRC register
	// -------
	// B = 000
	// C = 001
	// D = 010
	// E = 011
	// H = 100
	// L = 101
	// F = 110 - (?) unused
	// A = 111
	_byteRegisterMap[0x00] = reinterpret_cast<byte*>(&_BC) + 1; // Because the Z80 CPU is low-endian, a 16-bit address 0x[B][C] in the memory is [C][B]. The low byte comes first
	_byteRegisterMap[0x01] = reinterpret_cast<byte*>(&_BC);
	_byteRegisterMap[0x02] = reinterpret_cast<byte*>(&_DE) + 1;
	_byteRegisterMap[0x03] = reinterpret_cast<byte*>(&_DE);
	_byteRegisterMap[0x04] = reinterpret_cast<byte*>(&_HL) + 1;
	_byteRegisterMap[0x05] = reinterpret_cast<byte*>(&_HL);
	_byteRegisterMap[0x06] = reinterpret_cast<byte*>(&_AF); // Should not be used (F)
	_byteRegisterMap[0x07] = reinterpret_cast<byte*>(&_AF) + 1;

	
	// In binary ##ss####, where ss is a 16-bit register
	// -------
	// 00 = BC
	// 01 = DE
	// 10 = HL
	// 11 = SP
	_ushortRegisterMap[0x00] = &_BC;
	_ushortRegisterMap[0x01] = &_DE;
	_ushortRegisterMap[0x02] = &_HL;
	_ushortRegisterMap[0x03] = &_SP;
}

CPU::~CPU()
{
}

ulong CPU::NOP(const byte& opCode)
{
	return 4;
}
