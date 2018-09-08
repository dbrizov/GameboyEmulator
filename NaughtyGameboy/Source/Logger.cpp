#include "Logger.h"

bool Logger::m_isEnabled = true;

inline bool Logger::IsEnabled()
{
	return m_isEnabled;
}

inline void Logger::SetEnabled(bool enabled)
{
	m_isEnabled = enabled;
}
