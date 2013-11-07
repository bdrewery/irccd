## Invite

This callback is called when someone is inviting you.

### Function

	function onInvite(server, channel, who)

### Parameters

* server, the current server.
* channel, on which channel you are invited to.
* who, who invited you.

Note, don't write a plugin to auto join channels on invites, the option
**join-invite** in the *irccd.conf* will automatically do it for you.

<!--- vim: set syntax=mkd: -->
