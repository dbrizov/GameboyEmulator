#pragma once

// Declaration
class Logger
{
private:
	static bool _isEnabled;

private:
	Logger();

public:
	static bool IsEnabled();
	static void SetEnabled(bool enabled);

	template<typename TArg>
	static void Log(TArg arg);

	template<typename... TArgs>
	static void LogFormat(const char* format, TArgs... args);
};

// Implementation
bool Logger::_isEnabled = true;

inline bool Logger::IsEnabled()
{
	return _isEnabled;
}

inline void Logger::SetEnabled(bool enabled)
{
	_isEnabled = enabled;
}

template<typename TArg>
inline void Logger::Log(TArg arg)
{
	if (!_isEnabled)
	{
		return;
	}

	std::cout << arg << std::endl;
}

template<typename... TArgs>
inline void Logger::LogFormat(const char* format, TArgs... args)
{
	if (!_isEnabled)
	{
		return;
	}

	static char buffer[512];
	sprintf_s(buffer, format, args...);
	std::cout << buffer << std::endl;
}
