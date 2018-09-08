#pragma once

#include <iostream>
#include "stdio.h"

class Logger
{
private:
	static bool m_isEnabled;

private:
	Logger();

public:
	static bool IsEnabled();
	static void SetEnabled(bool enabled);

	template<typename TArg>
	static void Log(TArg arg);

	template<typename... TArgs>
	static void Log(const char* format, TArgs... args);

	template<typename... TArgs>
	static void LogError(const char* format, TArgs... args);
};

template<typename TArg>
void Logger::Log(TArg arg)
{
	if (!m_isEnabled)
	{
		return;
	}

	std::cout << arg << std::endl;
}

template<typename... TArgs>
void Logger::Log(const char* format, TArgs... args)
{
	if (!m_isEnabled)
	{
		return;
	}

	static char buffer[512];
	sprintf_s(buffer, format, args...);
	std::cout << buffer << std::endl;
}

template<typename... TArgs>
void Logger::LogError(const char* format, TArgs... args)
{
	if (!m_isEnabled)
	{
		return;
	}

	std::cout << "ERROR: ";
	Log(format, args...);
}

