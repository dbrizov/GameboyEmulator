#pragma once

class LCD
{
public:
	LCD();

	void Init();
	void Deinit();

	void CreateWindow(int width, int height);
	void DestroyWindow();

private:
	bool m_initialized;
	int m_width;
	int m_height;
	int m_pixelSize;
	SDL_Window* m_window;
	SDL_Surface* m_surface;

	bool IsPowerOfTwo(int number);
};
