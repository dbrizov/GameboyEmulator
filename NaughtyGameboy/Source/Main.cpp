#include <iostream>
#include <SDL.h>
#include "PCH.h"
#include "CPU.h"
#include "LCD.h"
#include "Logger.h"

const ulong CyclesPerFrame = 70224;
const int ScreenWidth = 160 * 2;
const int ScreenHeight = 144 * 2;

int main(int argc, char* argv[])
{
	LCD lcd = LCD();
	lcd.Init();
	lcd.CreateWindow(ScreenWidth, ScreenHeight);

	CPU cpu = CPU();

	ulong cycles = 0;

	SDL_Event sdlEvent;
	bool isRunning = true;
	while (isRunning)
	{
		SDL_WaitEvent(&sdlEvent);
		if (sdlEvent.type == SDL_QUIT)
		{
			isRunning = false;
		}

		while (cycles < CyclesPerFrame)
		{
			cycles += cpu.Step();
		}

		cycles -= CyclesPerFrame;
	}

	lcd.DestroyWindow();
	lcd.Deinit();

	return 0;
}
