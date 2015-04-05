#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <streambuf>

#include "Logger.h"

namespace irccd {

LoggerBase::LoggerBase(std::streambuf *buf)
	: std::ostream(buf)
{
}

LoggerConsole::LoggerConsole(const std::ostream &stream)
	: LoggerBase(stream.rdbuf())
{
}

LoggerFile::LoggerFile(const std::string &path)
	: LoggerBase(&m_buffer)
{
	if (m_buffer.open(path, std::ios_base::out) == nullptr) {
		throw std::strerror(errno);
	}
}

LoggerSilent::LoggerSilent()
	: LoggerBase(&m_buffer)
{
}

std::unique_ptr<LoggerBase> Logger::m_output{new LoggerConsole};
std::unique_ptr<LoggerBase> Logger::m_error{new LoggerConsole(std::cerr)};
bool Logger::m_verbose{false};

namespace {

/* Silent output when verbose / debug are disabled */
LoggerSilent silent;

} // !namespace

void Logger::setVerbose(bool mode) noexcept
{
	m_verbose = mode;
}

std::ostream &Logger::info() noexcept
{
	return (m_verbose) ? *m_output : silent;
}

std::ostream &Logger::warning() noexcept
{
	return *m_error;
}

std::ostream &Logger::error() noexcept
{
	return *m_error;
}

std::ostream &Logger::debug() noexcept
{
#if defined(NDEBUG)
	return silent;
#else
	return *m_output;
#endif
}

} // !irccd