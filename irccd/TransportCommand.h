#ifndef _IRCCD_TRANSPORT_COMMAND_H_
#define _IRCCD_TRANSPORT_COMMAND_H_

#include <memory>
#include <string>

namespace irccd {

class Irccd;
class TransportClientAbstract;

class TransportCommand {
protected:
	std::shared_ptr<TransportClientAbstract> m_client;

public:
	inline TransportCommand(std::shared_ptr<TransportClientAbstract> client)
		: m_client(client)
	{
	}

	virtual ~TransportCommand() = default;

	virtual void exec(Irccd &) = 0;

	/**
	 * Provide a ident string for unit tests.
	 *
	 * Derived classes should just concat their name plus all fields
	 * separated by ':'.
	 *
	 * @return the ident
	 */
	virtual std::string ident() const = 0;
};

} // !irccd

#endif // !_IRCCD_TRANSPORT_COMMAND_H_
