#include "CPU.h"
#include "Logger.h"
#include "BitUtil.h"

const byte CPU::ZeroFlag = 7;
const byte CPU::SubtractFlag = 6;
const byte CPU::HalfCarryFlag = 5;
const byte CPU::CarryFlag = 4;

const byte CPU::ZeroFlagMask = 1 << 7;
const byte CPU::SubtractFlagMask = 1 << 6;
const byte CPU::HalfCarryFlagMask = 1 << 5;
const byte CPU::CarryFlagMask = 1 << 4;
const byte CPU::AllFlagsMask = 0xF0;

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
	// TODO Need to check this. There are some special cases with the PUSH and POP instruction that the AF register is used. However I think this is a Z80 thing only
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
	// The stack is in range FF80-FFFE where FFFE is the bottom of the stack, and FF80 is the maximum top of the stack
	// So in order to pop something from the stack we need to increase the stack pointer after we read the data from it
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

byte CPU::AddBytes_Two(byte b1, byte b2, byte affectedFlags /*= AllFlagsMask*/)
{
	byte result = b1 + b2;

	if (IS_BIT_SET(affectedFlags, ZeroFlag))
	{
		(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	if (IS_BIT_SET(affectedFlags, SubtractFlag))
	{
		ClearFlag(SubtractFlag);
	}

	if (IS_BIT_SET(affectedFlags, HalfCarryFlag))
	{
		(((b1 & 0x0F) + (b2 & 0x0F)) > 0x0F) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	}

	if (IS_BIT_SET(affectedFlags, CarryFlag))
	{
		((int)(b1 + b2) > 0xFF) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);
	}

	return result;
}

byte CPU::AddBytes_Three(byte b1, byte b2, byte b3, byte affectedFlags /*= AllFlagsMask*/)
{
	byte result = b1 + b2 + b3;

	if (IS_BIT_SET(affectedFlags, ZeroFlag))
	{
		(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	if (IS_BIT_SET(affectedFlags, SubtractFlag))
	{
		ClearFlag(SubtractFlag);
	}

	if (IS_BIT_SET(affectedFlags, HalfCarryFlag))
	{
		(((b1 & 0x0F) + (b2 & 0x0F) + (b3 & 0x0F)) > 0x0F) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	}

	if (IS_BIT_SET(affectedFlags, CarryFlag))
	{
		((int)(b1 + b2 + b3) > 0xFF) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);
	}

	return result;
}

ushort CPU::AddUShorts_Two(ushort s1, ushort s2, byte affectedFlags /*= AllFlagsMask*/)
{
	ushort result = s1 + s2;

	if (IS_BIT_SET(affectedFlags, ZeroFlag))
	{
		(result == 0x0000) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	if (IS_BIT_SET(affectedFlags, SubtractFlag))
	{
		ClearFlag(SubtractFlag);
	}

	if (IS_BIT_SET(affectedFlags, HalfCarryFlag))
	{
		(((s1 & 0x0F00) + (s2 & 0x0F00)) > 0x0F00) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	}

	if (IS_BIT_SET(affectedFlags, CarryFlag))
	{
		((int)(s1 + s2) > 0xFFFF) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);
	}

	return result;
}

byte CPU::SubtractBytes_Two(byte b1, byte b2, byte affectedFlags /*= AllFlagsMask*/)
{
	byte result = b1 - b2;

	if (IS_BIT_SET(affectedFlags, ZeroFlag))
	{
		(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	if (IS_BIT_SET(affectedFlags, SubtractFlag))
	{
		SetFlag(SubtractFlag);
	}

	if (IS_BIT_SET(affectedFlags, HalfCarryFlag))
	{
		((int)((b1 & 0x0F) - (b2 & 0x0F)) < 0x00) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	}

	if (IS_BIT_SET(affectedFlags, CarryFlag))
	{
		((int)(b1 - b2) < 0x00) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);
	}

	return result;
}

byte CPU::SubtractBytes_Three(byte b1, byte b2, byte b3, byte affectedFlags /*= AllFlagsMask*/)
{
	byte result = b1 - b2 - b3;

	if (IS_BIT_SET(affectedFlags, ZeroFlag))
	{
		(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	if (IS_BIT_SET(affectedFlags, SubtractFlag))
	{
		SetFlag(SubtractFlag);
	}

	if (IS_BIT_SET(affectedFlags, HalfCarryFlag))
	{
		((int)((b1 & 0x0F) - (b2 & 0x0F) - (b3 & 0x0F)) < 0x00) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	}

	if (IS_BIT_SET(affectedFlags, CarryFlag))
	{
		((int)(b1 - b2 - b3) < 0x00) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);
	}

	return result;
}

ushort CPU::SubtractUShorts_Two(ushort s1, ushort s2, byte affectedFlags /*= AllFlagsMask*/)
{
	ushort result = s1 - s2;

	if (IS_BIT_SET(affectedFlags, ZeroFlag))
	{
		(result == 0x0000) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	if (IS_BIT_SET(affectedFlags, SubtractFlag))
	{
		SetFlag(SubtractFlag);
	}

	if (IS_BIT_SET(affectedFlags, HalfCarryFlag))
	{
		((int)((s1 & 0x0F00) - (s2 & 0x0F00)) < 0x0000) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	}

	if (IS_BIT_SET(affectedFlags, CarryFlag))
	{
		((int)(s1 - s2) < 0x0000) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);
	}

	return result;
}

byte CPU::CompareBytes(byte b1, byte b2)
{
	byte flags = 0x00;

	if (b1 == b2)
	{
		flags = SET_BIT(flags, ZeroFlag);
	}

	flags = SET_BIT(flags, SubtractFlag);

	if ((int)((b1 & 0x0F) - (b2 - 0x0F)) < 0x00)
	{
		flags = SET_BIT(flags, HalfCarryFlag);
	}

	if ((int)(b1 - b2) < 0x00)
	{
		flags = SET_BIT(flags, CarryFlag);
	}

	return flags;
}

byte CPU::RotateLeft(byte b, bool clearZeroFlag /*= false*/)
{
	byte newCF = GET_BIT(b, 7);
	b = b << 1;
	b = (newCF == 1) ? SET_BIT(b, 0) : CLEAR_BIT(b, 0);

	if (clearZeroFlag)
	{
		ClearFlag(ZeroFlag);
	}
	else
	{
		(b == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(newCF == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return b;
}

byte CPU::RotateLeftThroughCarry(byte b, bool clearZeroFlag /*= false*/)
{
	byte oldCF = GetFlag(CarryFlag);
	byte newCF = GET_BIT(b, 7);
	b = b << 1;
	b = (oldCF == 1) ? SET_BIT(b, 0) : CLEAR_BIT(b, 0);

	if (clearZeroFlag)
	{
		ClearFlag(ZeroFlag);
	}
	else
	{
		(b == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(newCF == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return b;
}

byte CPU::RotateRight(byte b, bool clearZeroFlag /*= false*/)
{
	byte newCF = GET_BIT(b, 0);
	b = b >> 1;
	b = (newCF == 1) ? SET_BIT(b, 7) : CLEAR_BIT(b, 7);

	if (clearZeroFlag)
	{
		ClearFlag(ZeroFlag);
	}
	else
	{
		(b == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(newCF == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return b;
}

byte CPU::RotateRightThroughCarry(byte b, bool clearZeroFlag /*= false*/)
{
	byte oldCF = GetFlag(CarryFlag);
	byte newCF = GET_BIT(b, 0);
	b = b >> 1;
	b = (oldCF == 1) ? SET_BIT(b, 7) : CLEAR_BIT(b, 7);

	if (clearZeroFlag)
	{
		ClearFlag(ZeroFlag);
	}
	else
	{
		(b == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(newCF == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return b;
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
	byte result = AddBytes_Two(A, *r);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::ADD_A_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte result = AddBytes_Two(A, n);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::ADD_A_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte result = AddBytes_Two(A, value);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::ADC_A_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte cf = GetFlag(CarryFlag);
	byte result = AddBytes_Three(A, *r, cf);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::ADC_A_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte cf = GetFlag(CarryFlag);
	byte result = AddBytes_Three(A, n, cf);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::ADC_A_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte cf = GetFlag(CarryFlag);
	byte result = AddBytes_Three(A, value, cf);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::SUB_A_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte result = SubtractBytes_Two(A, *r);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::SUB_A_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte result = SubtractBytes_Two(A, n);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::SUB_A_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte result = SubtractBytes_Two(A, value);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::SBC_A_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte cf = GetFlag(CarryFlag);
	byte result = SubtractBytes_Three(A, *r, cf);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::SBC_A_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte cf = GetFlag(CarryFlag);
	byte result = SubtractBytes_Three(A, n, cf);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::SBC_A_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte cf = GetFlag(CarryFlag);
	byte result = SubtractBytes_Three(A, value, cf);
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

ulong CPU::CP_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte flags = CompareBytes(A, *r);
	SetLowByte(&m_AF, flags);

	return 4;
}

ulong CPU::CP_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte flags = CompareBytes(A, n);
	SetLowByte(&m_AF, flags);

	return 8;
}

ulong CPU::CP_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte flags = CompareBytes(A, value);
	SetLowByte(&m_AF, flags);

	return 8;
}

ulong CPU::INC_r(byte opcode)
{
	byte* r = GetByteRegister_Dst(opcode);
	byte result = AddBytes_Two(*r, 1, /*affectedFlags =*/ ZeroFlagMask | SubtractFlagMask | HalfCarryFlagMask);
	*r = result;

	return 4;
}

ulong CPU::INC_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte result = AddBytes_Two(value, 1, /*affectedFlags =*/ ZeroFlagMask | SubtractFlagMask | HalfCarryFlagMask);
	m_MMU->WriteByte(m_HL, result);

	return 12;
}

ulong CPU::DEC_r(byte opcode)
{
	byte* r = GetByteRegister_Dst(opcode);
	byte result = SubtractBytes_Two(*r, 1, /*affectedFlags =*/ ZeroFlagMask | SubtractFlagMask | HalfCarryFlagMask);
	*r = result;

	return 4;
}

ulong CPU::DEC_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte result = SubtractBytes_Two(value, 1, /*affectedFlags =*/ ZeroFlagMask | SubtractFlagMask | HalfCarryFlagMask);
	m_MMU->WriteByte(m_HL, result);

	return 12;
}

/*
This instruction conditionally adjusts the accumulator for BCD (binary coded decimal) addition
and subtraction operations. For addition(ADD, ADC, INC) or subtraction
(SUB, SBC, DEC, NEC), the following table indicates the operation performed :

N   C  Value of      H  Value of     Hex no   C flag after
	   high nibble      low nibble   added    execution

0   0      0-9       0     0-9       00       0
0   0      0-8       0     A-F       06       0
0   0      0-9       1     0-3       06       0
0   0      A-F       0     0-9       60       1
0   0      9-F       0     A-F       66       1
0   0      A-F       1     0-3       66       1
0   1      0-2       0     0-9       60       1
0   1      0-2       0     A-F       66       1
0   1      0-3       1     0-3       66       1
1   0      0-9       0     0-9       00       0
1   0      0-8       1     6-F       FA       0
1   1      7-F       0     0-9       A0       1
1   1      6-F       1     6-F       9A       1

Flags:
	Z : Set if ACC is Zero after operation, clear otherwise.
	N : Unaffected.
	H : Clear
	C : See instruction.

Example:
	If an addition operation is performed between 15 (BCD) and 27 (BCD), simple decimal
	arithmetic gives this result:

	  15
	+ 27
	----
	  42

	But when the binary representations are added in the Accumulator according to
	standard binary arithmetic:

	  0001 0101  15
	+ 0010 0111  27
	---------------
	  0011 1100  3C

	The sum is ambiguous. The DAA instruction adjusts this result so that correct
	BCD representation is obtained:

	  0011 1100  3C result
	+ 0000 0110  06 + error
	---------------
	  0100 0010  42 Correct BCD!
*/
ulong CPU::DAA(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte err = 0x00; // error
	byte c = GetFlag(CarryFlag);
	byte h = GetFlag(HalfCarryFlag);

	if (!IsFlagSet(SubtractFlag))
	{
		if (c || (A >= 0x9A))
		{
			err += 0x60;
		}

		if (h || ((A & 0x0F) >= 0x0A))
		{
			err += 0x06;
		}
	}
	else
	{
		if (!c && h)
		{
			err += 0xFA;
		}
		else if (c && !h)
		{
			err += 0xA0;
		}
		else if (c && h)
		{
			err += 0x9A;
		}
	}

	byte result = A + err;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(HalfCarryFlag);

	if ((A + err) > 0xFF)
	{
		SetFlag(CarryFlag);
	}

	return 4;
}

ulong CPU::CPL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte result = A ^ 0xFF;
	SetHighByte(&m_AF, result);

	SetFlag(SubtractFlag);
	SetFlag(HalfCarryFlag);

	return 4;
}

ulong CPU::ADD_HL_rr(byte opcode)
{
	ushort* rr = GetUShortRegister(opcode);
	ushort result = AddUShorts_Two(m_HL, *rr, /*affectedFlags =*/ SubtractFlagMask | HalfCarryFlagMask | CarryFlagMask);
	m_HL = result;

	return 8;
}

ulong CPU::INC_rr(byte opcode)
{
	ushort* rr = GetUShortRegister(opcode);
	ushort result = AddUShorts_Two(*rr, 1, /*affectedFlags =*/ 0x00);
	*rr = result;

	return 8;
}

ulong CPU::DEC_rr(byte opcode)
{
	ushort* rr = GetUShortRegister(opcode);
	ushort result = SubtractUShorts_Two(*rr, 1, /*affectedFlags =*/ 0x00);
	*rr = result;

	return 8;
}

ulong CPU::ADD_SP_dd(byte opcode)
{
	sbyte dd = (sbyte)ReadBytePCI();
	ushort result = (m_SP + dd);

	ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	((result & 0x0F) < (m_SP & 0x0F)) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	((result & 0xFF) < (m_SP & 0xFF)) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	m_SP = result;

	return 16;
}

ulong CPU::LD_HL_SPdd(byte opcode)
{
	sbyte dd = (sbyte)ReadBytePCI();
	ushort result = (m_SP + dd);

	ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	((result & 0x0F) < (m_SP & 0x0F)) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	((result & 0xFF) < (m_SP & 0xFF)) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return 12;
}

ulong CPU::RLCA(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte result = RotateLeft(A, /*clearZeroFlag =*/ true);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::RLA(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte result = RotateLeftThroughCarry(A, /*clearZeroFlag =*/ true);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::RRCA(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte result = RotateRight(A, /*clearZeroFlag =*/ true);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::RRA(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte result = RotateRightThroughCarry(A, /*clearZeroFlag =*/ true);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::RLC_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	byte result = RotateLeft(*r);
	*r = result;

	return 8;
}

ulong CPU::RLC_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte result = RotateLeft(value);
	m_MMU->WriteByte(m_HL, result);

	return 16;
}

ulong CPU::RL_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	byte result = RotateLeftThroughCarry(*r);
	*r = result;

	return 8;
}

ulong CPU::RL_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte result = RotateLeftThroughCarry(value);
	m_MMU->WriteByte(m_HL, result);

	return 16;
}

ulong CPU::RRC_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	byte result = RotateRight(*r);
	*r = result;

	return 8;
}

ulong CPU::RRC_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte result = RotateRight(value);
	m_MMU->WriteByte(m_HL, result);

	return 16;
}

ulong CPU::RR_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	byte result = RotateRightThroughCarry(*r);
	*r = result;

	return 8;
}

ulong CPU::RR_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte result = RotateRightThroughCarry(value);
	m_MMU->WriteByte(m_HL, result);

	return 16;
}

ulong CPU::SLA_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	byte cf = GET_BIT(*r, 7);
	*r = (*r << 1);

	(*r == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(cf == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::SLA_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte cf = GET_BIT(value, 7);
	byte result = (value << 1);
	m_MMU->WriteByte(m_HL, result);

	(result == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(cf == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return 16;
}

ulong CPU::SRA_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	byte cf = GET_BIT(*r, 0);
	*r = (*r >> 1) | (*r & 0x80);

	(*r == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(cf == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::SRA_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte cf = GET_BIT(value, 0);
	byte result = (value >> 1) | (value & 0x80);
	m_MMU->WriteByte(m_HL, result);

	(result == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(cf == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return 16;
}

ulong CPU::SRL_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	byte cf = GET_BIT(*r, 0);
	*r = (*r >> 1);

	(*r == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(cf == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::SRL_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte cf = GET_BIT(value, 0);
	byte result = (value >> 1);
	m_MMU->WriteByte(m_HL, result);

	(result == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(cf == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return 16;
}

ulong CPU::SWAP_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	byte low = (*r & 0x0F);
	byte high = (*r & 0xF0);
	*r = (low << 4) | (high >> 4);

	(*r == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::SWAP_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte low = (value & 0x0F);
	byte high = (value & 0xF0);
	byte result = (low << 4) | (high >> 4);
	m_MMU->WriteByte(m_HL, result);

	(result == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 16;
}

ulong CPU::NOP(byte opcode)
{
	return 4;
}
