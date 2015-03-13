#ifndef _IRCCD_TRANSPORT_COMMAND_INVITE_H_
#define _IRCCD_TRANSPORT_COMMAND_INVITE_H_

#include "TransportCommand.h"

namespace irccd {

namespace transport {

class Invite : public TransportCommand {
private:
	std::string m_server;
	std::string m_target;
	std::string m_channel;

public:
	Invite(std::shared_ptr<TransportClientAbstract> client,
	       std::string server,
	       std::string target,
	       std::string channel);

	void exec(Irccd &) override;

	std::string ident() const override;
};

} // !transport

} // !irccd

#endif // !_IRCCD_TRANSPORT_COMMAND_INVITE_H_
