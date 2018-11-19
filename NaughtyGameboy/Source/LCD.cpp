#include <SDL.h>
#include "LCD.h"
#include "Logger.h"

LCD::LCD() :
	m_initialized(false),
	m_width(0),
	m_height(0),
	m_window(nullptr),
	m_surface(nullptr)
{
}

void LCD::Init()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		Logger::LogError("SDL initialization failed! SDL_Error: %s\n", SDL_GetError());
	}
	else
	{
		m_initialized = true;
	}
}

void LCD::Deinit()
{
	if (m_initialized)
	{
		m_initialized = false;
		SDL_Quit();
	}
}

void LCD::CreateWindow(int width, int height)
{
	int horizontalPixelSize = width / 160;
	int verticalPixelSize = height / 144;
	if (horizontalPixelSize != verticalPixelSize || !IsPowerOfTwo(horizontalPixelSize) || !IsPowerOfTwo(verticalPixelSize))
	{
		Logger::LogError("Invalid screen m_width(%d) and m_height(%d)", width, height);
		return;
	}

	m_width = width;
	m_height = height;
	m_pixelSize = horizontalPixelSize;

	m_window = SDL_CreateWindow("NaughtyGameboy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_width, m_height, SDL_WINDOW_SHOWN);
	if (m_window == nullptr)
	{
		Logger::LogError("Window could not be created! SDL_Error: %s", SDL_GetError());
	}
	else
	{
		m_surface = SDL_GetWindowSurface(m_window);
		SDL_FillRect(m_surface, nullptr, SDL_MapRGB(m_surface->format, 0x00, 0x00, 0x00));
		SDL_UpdateWindowSurface(m_window);
	}
}

void LCD::DestroyWindow()
{
	if (m_window != nullptr)
	{
		SDL_DestroyWindow(m_window);
		m_window = nullptr;
		m_surface = nullptr;
	}
}

bool LCD::IsPowerOfTwo(int number)
{
	bool isPowerOfTwo = (number != 0) && ((number & (number - 1)) == 0);
	return isPowerOfTwo;
}
