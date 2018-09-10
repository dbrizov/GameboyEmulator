#include "CPU.h"
#include "Logger.h"
#include "BitUtil.h"

const byte CPU::ZeroFlag = 7;
const byte CPU::SubtractFlag = 6;
const byte CPU::HalfCarryFlag = 5;
const byte CPU::CarryFlag = 4;

CPU::CPU() :
	m_cycles(0),
	m_AF(0x0000),
	m_BC(0x0000),
	m_DE(0x0000),
	m_HL(0x0000),
	m_SP(0x0000),
	m_PC(0x0000)
{	
	// In binary ##dddsss, where ddd is the DST register, and sss is the SRC register
	// -------
	// B = 000
	// C = 001
	// D = 010
	// E = 011
	// H = 100
	// L = 101
	// F = 110 - unused
	// A = 111
	m_byteRegisterMap[0x00] = reinterpret_cast<byte*>(&m_BC) + 1; // Because the Z80 CPU is low-endian, a 16bit address 0x[B][C] in the memory is [C][B]. The low byte comes first
	m_byteRegisterMap[0x01] = reinterpret_cast<byte*>(&m_BC);
	m_byteRegisterMap[0x02] = reinterpret_cast<byte*>(&m_DE) + 1;
	m_byteRegisterMap[0x03] = reinterpret_cast<byte*>(&m_DE);
	m_byteRegisterMap[0x04] = reinterpret_cast<byte*>(&m_HL) + 1;
	m_byteRegisterMap[0x05] = reinterpret_cast<byte*>(&m_HL);
	m_byteRegisterMap[0x06] = reinterpret_cast<byte*>(&m_AF); // Should not be used (F)
	m_byteRegisterMap[0x07] = reinterpret_cast<byte*>(&m_AF) + 1;
	
	// In binary ##rr####, where rr is a 16bit register
	// -------
	// 00 = BC
	// 01 = DE
	// 10 = HL
	// 11 = SP
	m_ushortRegisterMap[0x00] = &m_BC;
	m_ushortRegisterMap[0x01] = &m_DE;
	m_ushortRegisterMap[0x02] = &m_HL;
	m_ushortRegisterMap[0x03] = &m_SP;

	m_MMU = std::make_unique<MMU>();

	InitInstructionMap();
}

ulong CPU::Step()
{
	ulong cycles = 0;

	ushort address = m_PC;
	byte opcode = ReadBytePCI();
	InstructionFunction instruction = nullptr;

	if (opcode == 0xCB)
	{
		opcode = ReadBytePCI();
		instruction = m_instructionMapCB[opcode];
	}
	else
	{
		instruction = m_instructionMap[opcode];
	}

	if (instruction != nullptr)
	{
		cycles = (this->*instruction)(opcode);
	}
	else
	{
		Logger::LogError("OpCode 0x%02X at address 0x%04X could not be interpreted.", opcode, address);
	}

	return cycles;
}

byte CPU::ReadBytePCI()
{
	byte value = m_MMU->ReadByte(m_PC);
	m_PC++;

	return value;
}

ushort CPU::ReadUShortPCI()
{
	ushort value = m_MMU->ReadUShort(m_PC);
	m_PC += 2;

	return value;
}

void CPU::InitInstructionMap()
{
	for (int i = 0; i < ARRAY_SIZE(m_instructionMap); i++)
	{
		m_instructionMap[i] = nullptr;
	}

	for (int i = 0; i < ARRAY_SIZE(m_instructionMapCB); i++)
	{
		m_instructionMapCB[i] = nullptr;
	}

	m_instructionMap[0x00] = &CPU::NOP;
}

byte* CPU::GetByteRegister_Src(byte opcode)
{
	return m_byteRegisterMap[opcode & 0x07];
}

byte* CPU::GetByteRegister_Dst(byte opcode)
{
	return m_byteRegisterMap[(opcode >> 3) & 0x07];
}

ushort* CPU::GetUShortRegister(byte opcode)
{
	return m_ushortRegisterMap[(opcode >> 4) & 0x03];
}

void CPU::PushByteToStack(byte value)
{
	// The stack is in range FF80-FFFE where FFFE is the bottom of the stack, and FF80 is the maximum top of the stack
	// So in order to push something to the stack we need to decrement the stack pointer first
	m_SP--;
	m_MMU->WriteByte(m_SP, value);
}

void CPU::PushUShortToStack(ushort value)
{
	// The stack is in range FF80-FFFE where FFFE is the bottom of the stack, and FF80 is the maximum top of the stack
	// So in order to push something to the stack we need to decrement the stack pointer first
	m_SP -= 2;
	m_MMU->WriteUShort(m_SP, value);
}

byte CPU::PopByteFromStack()
{
	// The stack is in range FF80-FFFE where FFFE is the bottom of the stack, and FF80 is the maximum top of the stack
	// So in order to pop something from the stack we need to increase the stack pointer after we read the data from it
	byte value = m_MMU->ReadByte(m_SP);
	m_SP++;

	return value;
}

ushort CPU::PopUShortFromStack()
{
	// We pop the low byte first, and the high byte second, becasuse in memory the low byte comes first (the CPU is low-endian)
	ushort value = m_MMU->ReadUShort(m_SP);
	m_SP += 2;
	
	return value;
}

byte CPU::GetFlag(byte flag)
{
	byte F = GetLowByte(m_AF);
	return GET_BIT(F, flag);
}

void CPU::SetFlag(byte flag)
{
	byte F = GetLowByte(m_AF);
	F = SET_BIT(F, flag);
	SetLowByte(&m_AF, F);
}

void CPU::ClearFlag(byte flag)
{
	byte F = GetLowByte(m_AF);
	F = CLEAR_BIT(F, flag);
	SetLowByte(&m_AF, F);
}

bool CPU::IsFlagSet(byte flag)
{
	byte F = GetLowByte(m_AF);
	return IS_BIT_SET(F, flag);
}

byte CPU::AddBytes(byte b1, byte b2)
{
	byte result = b1 + b2;

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	(((b1 & 0x0F) + (b2 & 0x0F)) > 0x0F) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	((int)(b1 + b2) > 0xFF) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return result;
}

byte CPU::AddBytes(byte b1, byte b2, byte b3)
{
	byte result = b1 + b2 + b3;

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	(((b1 & 0x0F) + (b2 & 0x0F) + (b3 & 0x0F)) > 0x0F) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	((int)(b1 + b2 + b3) > 0xFF) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return result;
}

byte CPU::SubtractBytes(byte b1, byte b2)
{
	byte result = b1 - b2;

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	SetFlag(SubtractFlag);
	((int)((b1 & 0x0F) - (b2 & 0x0F)) < 0x00) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	((int)(b1 - b2) < 0x00) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return result;
}

byte CPU::SubtractBytes(byte b1, byte b2, byte b3)
{
	byte result = b1 - b2 - b3;

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	SetFlag(SubtractFlag);
	((int)((b1 & 0x0F) - (b2 & 0x0F) - (b3 & 0x0F)) < 0x00) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	((int)(b1 - b2 - b3) < 0x00) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return result;
}

ulong CPU::LD_r_n(byte opcode)
{
	byte n = ReadBytePCI();
	byte* r = GetByteRegister_Dst(opcode);
	*r = n;

	return 8;
}

ulong CPU::LD_r_R(byte opcode)
{
	byte* R = GetByteRegister_Src(opcode);
	byte* r = GetByteRegister_Dst(opcode);
	*r = *R;

	return 4;
}

ulong CPU::LD_r_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte* r = GetByteRegister_Dst(opcode);
	*r = value;

	return 8;
}

ulong CPU::LD_0xHL_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	m_MMU->WriteByte(m_HL, *r);

	return 8;
}

ulong CPU::LD_0xHL_n(byte opcode)
{
	byte n = ReadBytePCI();
	m_MMU->WriteByte(m_HL, n);

	return 12;
}

ulong CPU::LD_A_0xBC(byte opcode)
{
	byte value = m_MMU->ReadByte(m_BC);
	SetHighByte(&m_AF, value);

	return 8;
}

ulong CPU::LD_A_0xDE(byte opcode)
{
	byte value = m_MMU->ReadByte(m_DE);
	SetHighByte(&m_AF, value);

	return 8;
}

ulong CPU::LD_A_0xnn(byte opcode)
{
	ushort nn = ReadUShortPCI();
	byte value = m_MMU->ReadByte(nn);
	SetHighByte(&m_AF, value);

	return 16;
}

ulong CPU::LD_0xBC_A(byte opcode)
{
	byte A = GetHighByte(m_AF);
	m_MMU->WriteByte(m_BC, A);

	return 8;
}

ulong CPU::LD_0xDE_A(byte opcode)
{
	byte A = GetHighByte(m_AF);
	m_MMU->WriteByte(m_DE, A);

	return 8;
}

ulong CPU::LD_0xnn_A(byte opcode)
{
	byte A = GetHighByte(m_AF);
	ushort nn = ReadUShortPCI();
	m_MMU->WriteByte(nn, A);

	return 16;
}

ulong CPU::LD_A_0xFF00n(byte opcode)
{
	byte n = ReadBytePCI();
	byte value = m_MMU->ReadByte(0xFF00 + n);
	SetHighByte(&m_AF, value);

	return 12;
}

ulong CPU::LD_0xFF00n_A(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	m_MMU->WriteByte(0xFF00 + n, A);

	return 12;
}

ulong CPU::LD_A_0xFF00C(byte opcode)
{
	byte C = GetLowByte(m_BC);
	byte value = m_MMU->ReadByte(0xFF00 + C);
	SetHighByte(&m_AF, value);

	return 8;
}

ulong CPU::LD_0xFF00C_A(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte C = GetLowByte(m_BC);
	m_MMU->WriteByte(0xFF00 + C, A);

	return 8;
}

ulong CPU::LDI_0xHL_A(byte opcode)
{
	byte A = GetHighByte(m_AF);
	m_MMU->WriteByte(m_HL, A);
	m_HL++;

	return 8;
}

ulong CPU::LDI_A_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	SetHighByte(&m_AF, value);
	m_HL++;

	return 8;
}

ulong CPU::LDD_0xHL_A(byte opcode)
{
	byte A = GetHighByte(m_AF);
	m_MMU->WriteByte(m_HL, A);
	m_HL--;

	return 8;
}

ulong CPU::LDD_A_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	SetHighByte(&m_AF, value);
	m_HL--;

	return 8;
}

ulong CPU::LD_rr_nn(byte opcode)
{
	ushort nn = ReadUShortPCI();
	ushort* rr = GetUShortRegister(opcode);
	*rr = nn;

	return 12;
}

ulong CPU::LD_SP_HL(byte opcode)
{
	m_SP = m_HL;

	return 8;
}

ulong CPU::PUSH_rr(byte opcode)
{
	ushort* rr = GetUShortRegister(opcode);
	PushUShortToStack(*rr);

	return 16;
}

ulong CPU::POP_rr(byte opcode)
{
	ushort* rr = GetUShortRegister(opcode);
	ushort value = PopUShortFromStack();
	*rr = value;

	return 12;
}

ulong CPU::ADD_A_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte result = AddBytes(A, *r);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::ADD_A_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte result = AddBytes(A, n);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::ADD_A_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte result = AddBytes(A, value);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::ADC_A_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte cf = GetFlag(CarryFlag);
	byte result = AddBytes(A, *r, cf);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::ADC_A_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte cf = GetFlag(CarryFlag);
	byte result = AddBytes(A, n, cf);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::ADC_A_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte cf = GetFlag(CarryFlag);
	byte result = AddBytes(A, value, cf);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::SUB_A_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte result = SubtractBytes(A, *r);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::SUB_A_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte result = SubtractBytes(A, n);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::SUB_A_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte result = SubtractBytes(A, value);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::SBC_A_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte cf = GetFlag(CarryFlag);
	byte result = SubtractBytes(A, *r, cf);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::SBC_A_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte cf = GetFlag(CarryFlag);
	byte result = SubtractBytes(A, n, cf);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::SBC_A_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte cf = GetFlag(CarryFlag);
	byte result = SubtractBytes(A, value, cf);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::AND_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte result = A & *r;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	SetFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 4;
}

ulong CPU::AND_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte result = A & n;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	SetFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::AND_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte result = A & value;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	SetFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::XOR_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte result = A ^ *r;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 4;
}

ulong CPU::XOR_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte result = A ^ n;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::XOR_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte result = A ^ value;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::OR_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte result = A | *r;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 4;
}

ulong CPU::OR_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte result = A | n;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::OR_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte result = A | value;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::NOP(byte opcode)
{
	return 4;
}
