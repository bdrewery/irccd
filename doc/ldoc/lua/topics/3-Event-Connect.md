## Connect

This callback is called when the *irccd* instance successfully connect
to a server.

### Function

	function onConnect(server)

### Parameters

* server, the current server.

### Note

Don't write a plugin to auto join channels, the option **channels**
in the *irccd.conf* will automatically do it for you.

<!--- vim: set syntax=mkd: -->
