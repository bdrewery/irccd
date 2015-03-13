#include "Invite.h"

namespace irccd {

namespace transport {

Invite::Invite(std::shared_ptr<TransportClientAbstract> client,
	       std::string server,
	       std::string target,
	       std::string channel)
	: TransportCommand(std::move(client))
	, m_server(std::move(server))
	, m_target(std::move(target))
	, m_channel(std::move(channel))
{
}

void Invite::exec(Irccd &)
{
}

std::string Invite::ident() const
{
	return "invite:" + m_server + ":" + m_target + ":" + m_channel;
}

} // !transport

} // !irccd
