## Command

Special commands are not real IRC events. They are called from channel messages
with a specific syntax using a delimiter and the plugin name.

For instance, with default *irccd* parameters, saying on a channel **!ask foo**
will call the special command of the plugin named ask, so from the file
*ask.lua*.

Note that the command character is not fixed to '!', it's just the default and
can be set by the parameter **command-char** in a **[server]** section, but
for more information see the *irccd.conf* manual.

### Function

	function onCommand(server, channel, who, message)

### Parameters

* server, the current server.
* channel, the channel where the message comes from.
* who, who invoked the command.
* message, the real message, without the "!<name>" part.

<!--- vim: set syntax=mkd: -->
