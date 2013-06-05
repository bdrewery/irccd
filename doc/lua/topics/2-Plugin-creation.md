## First plugin, a repeater

Now, we will write a simple plugin that repeat every sentences said on a
channel, it's quite easy but you should not use it on a real channel or
you'll probably get kicked.

### Requirements

First of all, you need to be a little familiar with the Lua
[syntax](http://www.lua.org/manual/5.2/ "syntax").

Please also note that we are running Lua *5.2* and older versions
are not supported.

### Create your plugin

We will create our new plugin under the home user plugin path. If you're running
on Unix systems it is usually *${XDG\_CONFIG\_HOME}/.irccd/plugins*.

So let start by creating a plugin named **repeater.lua**. So, on my system the
file will live as */home/markand/.config/irccd/plugins/repeater.lua*.

### Register the callback

Remember, plugins are made through the event driven mechanism, so we must
define a function that will be called when a user said something on the channel.

The function defined on channel message is called **onMessage**. It has the
following signature:

	function onMessage(server, channel, nickname, message)

The parameters are defined as follow:

* server, on which server the message happened
* channel, on which channel
* nickname, who emit the message
* message, and the message content

### Send the response

Now that we have the message, the channel and the server, we can send the
copy of the message. For this, you must take care of the **server** parameter.

Server is one of defined in the *irccd.conf* file, there are several methods
available for the object, they are defined in the @{irccd.server} documentation.

The one we are interested in is @{irccd.server.Server:say}. This function takes
two parameters, the target which can be a nickname or a channel, and the
message.

So the only thing to do is the following:

	function onMessage(server, channel, nickname, message)
		server:say(channel, message)
	end

That's it! You've just made a brand new plugin, of course it's not a very
powerful one but at least you understood the way it works. With the powerful
API provided you will be able to create a bunch of plugins that can fits
your needs, such as a content provider, a moderator, a calculator and
so on.

To see all server functions, see @{irccd.server}.
To get a list of all IRC events, see @{3-Events.md}

<!--- vim: set syntax=mkd: -->
