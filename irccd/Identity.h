#ifndef _IRCCD_IDENTITY_H_
#define _IRCCD_IDENTITY_H_

#include <string>

namespace irccd {

class Identity {
private:
	std::string m_name;		//!< identity name
	std::string m_nickname;		//!< nickname to show
	std::string m_username;		//!< username to use for connection
	std::string m_realname;		//!< the full real name
	std::string m_ctcpversion;	//!< the CTCP version to define

public:
	Identity(std::string name = "irccd",
		 std::string nickname = "irccd",
		 std::string username = "irccd",
		 std::string realname = "IRC Client Daemon",
		 std::string ctcpversion = "IRC Client Daemon")
		: m_name(std::move(name))
		, m_nickname(std::move(nickname))
		, m_username(std::move(username))
		, m_realname(std::move(realname))
		, m_ctcpversion(std::move(ctcpversion))
	{
	}

	inline const std::string &name() const noexcept
	{
		return m_name;
	}

	inline const std::string &nickname() const noexcept
	{
		return m_nickname;
	}

	inline void setNickname(std::string nickname) noexcept
	{
		m_nickname = std::move(nickname);
	}

	inline const std::string &username() const noexcept
	{
		return m_username;
	}

	inline const std::string &realname() const noexcept
	{
		return m_realname;
	}

	inline const std::string &ctcpversion() const noexcept
	{
		return m_ctcpversion;
	}
};

} // !irccd

#endif // !_IRCCD_IDENTITY_H_
