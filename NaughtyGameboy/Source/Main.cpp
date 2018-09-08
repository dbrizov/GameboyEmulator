#include <iostream>
#include "PCH.h"
#include "CPU.h"
#include "Logger.h"

const ulong CyclesPerFrame = 70224;

int main()
{
	CPU cpu = CPU();

	ulong cycles = 0;

	bool isRunning = true;
	while (isRunning)
	{
		while (cycles < CyclesPerFrame)
		{
			cycles += cpu.Step();
		}

		cycles -= CyclesPerFrame;
	}

	std::cin.get();
}
