#ifndef _IRCCD_TIMER_EVENT_H_
#define _IRCCD_TIMER_EVENT_H_

#include <cstdint>
#include <memory>

#include "Plugin.h"
#include "Timer.h"

namespace irccd {

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
	TimerEvent(std::shared_ptr<Plugin> plugin,
		   std::shared_ptr<Timer> timer,
		   TimerEventType type = TimerEventType::Signal) noexcept;

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

	void call() noexcept;
};

} // !irccd

#endif // !_IRCCD_TIMER_EVENT_H_
