#ifndef _IRCCD_TIMER_EVENT_H_
#define _IRCCD_TIMER_EVENT_H_

#include <cstdint>
#include <memory>

namespace irccd {

class Plugin;
class Timer;

enum class TimerEventType {
	Signal,
	End
};

class TimerEvent {
private:
	std::shared_ptr<Plugin> m_plugin;
	std::shared_ptr<Timer> m_timer;
	TimerEventType m_type;

public:
	inline TimerEvent(std::shared_ptr<Plugin> plugin, std::shared_ptr<Timer> timer, TimerEventType type = TimerEventType::Signal) noexcept
		: m_plugin(plugin)
		, m_timer(timer)
		, m_type(TimerEventType::Signal)
	{
	}

	inline const std::shared_ptr<Plugin> &plugin() const noexcept
	{
		return m_plugin;
	}

	inline const std::shared_ptr<Timer> &timer() const noexcept
	{
		return m_timer;
	}

	inline TimerEventType type() const noexcept
	{
		return m_type;
	}
};

} // !irccd

#endif // !_IRCCD_TIMER_EVENT_H_