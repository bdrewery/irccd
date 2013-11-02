## Reload

This function is called when *irccd* instance reload a plugin. Thus,
there are **no** IRC events that call this function.

<br />
This function does nothing in the irccd internals, it just calls a function
that you can use to reload some data. It does not delete anything.

<br />
If you want to fully unload a plugin, use irccdctl unload then irccdctl load.

### Function

	function onReload()

<!--- vim: set syntax=mkd: -->
