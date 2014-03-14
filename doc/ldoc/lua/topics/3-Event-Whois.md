## Whois

This event is triggered when irccd get information from a user.

### Function

	function onWhois(server, info)

### Parameters

<ul>
<li>server, the current server.</li>
<li>info, a table with the following information:</li>
  <ul>
  <li>nickname, the user nickname.</li>
  <li>user, the user name.</li>
  <li>host, the hostname.</li>
  <li>realname, the real name used.</li>
  <li>channels, an optional sequences of channels joined.</li>
  </ul>
</ul>
