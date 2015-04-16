#include <cassert>

#include "ServerCommand.h"

ServerCommand::ServerCommand(std::function<bool ()> command)
	: m_command(std::move(command))
{
	assert(command);
}

bool ServerCommand::call()
{
	m_command(*this);
}