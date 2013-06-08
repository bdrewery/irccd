## Synopsis

The *irccd* daemon works like a waiting operator. It waits until someone
connects, speak or something else happen. It's event based.

There are a lot of events that you can catch withing your plugin, of course
there are no prerequisite of implementing all IRC events, you may want to
catch only connections or messages.

## The server object

Almost all of the callbacks have a server object, you may guess what it is.
In fact, *irccd* as you may already know, is multiple server based so each
plugins are called for each server defined because currently plugins are
loaded globally and not per a server basis.

So if you want to answer to someone who asked you to compute a very complicated
calculus, you need this server to send back a response and that's why server
is passed almost all of these events.

## Note about yourself

By default, almost every callback will also be called if **you** made an
action, like joining, saying something, got kicked and so on.

## The list of supported events

Currently, the following callback are supported:

* @{3.101-Action.md}
* @{3.102-Command.md}
* @{3.103-Connect.md}
* @{3.104-Channel-notice.md}
* @{3.105-Invite.md}
* @{3.106-Join.md}
* @{3.107-Kick.md}
* @{3.108-Message.md}
* @{3.109-Mode.md}
* @{3.110-Nick.md}
* @{3.111-Notice.md}
* @{3.112-Part.md}
* @{3.113-Query.md}
* @{3.114-Reload.md}
* @{3.115-Topic.md}
* @{3.116-User-mode.md}

<!--- vim: set syntax=mkd: -->
