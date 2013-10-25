## Basic

The Irccd Lua API let you extend irccd using Lua plugins. It works with a event
driven programmin basis. Thus, if someone connects to a channel, you can
catch this event within your Lua plugin and know which person has joined
the channel and do what you want.

### Adding Lua plugins

Usually, you would like to create a new Lua plugin within the standard
default path, which is usually */usr/local/share/irccd/plugins*.

However, it's is possible to add modules in your home directory, so you don't
require root rights to add them. Usually it is in your XDG config home,
*${HOME}/.config/irccd/plugins*.

### Overriding an existing plugin

Some people may want to tweak an existing plugin, you may edit directly
in the default plugin path but this will be overriden by a new release
installation, so the better way is to copy it into your local plugin
directory and edit it.

Lua plugins are searched by default in user local directory and if not
found, in the standard irccd plugins path said above.

<br />

Next @{2-Plugin_creation.md}

<!--- vim: set syntax=mkd: -->
