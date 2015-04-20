IRC Client Daemon CHANGES
=========================

irccd current
----------------------

- Switch from Lua to JavaScript,
- Add new irccd.util.convert,
- Add onQueryCommand new callback,
- Added more fields in Date.prototype.calendar,
- Add option auto-rejoin to server,
- Add uid and gid options,
- Add rules system,
- Add logger.debug,
- Create brand new documentation with Pandoc,
- Add new irccd.timer API,
- UDP listeners have been removed,
- The history plugin has now better messages,
- Irccd can now connect to IPv6 servers.

irccd 1.1.5 2015-02-14
----------------------

- Fix Mac OS X build,
- Fix null constructed strings,
- Fix general.foreground option not working,
- Windows: Lua and OpenSSL are built as DLL and copied to installation.

irccd 1.1.4 2014-03-28
----------------------

- Fix Visual C++ redistributable installation.

irccd 1.1.3 2014-03-22
----------------------

- Fix default internet socket address,
- Remove listener disconnection errors,
- Updated Windows C++ 2013 redistributable.

irccd 1.1.2 2014-02-26
----------------------

- Fix the example in roulette documentation,
- Fix command parsing for onCommand event.

irccd 1.1.1 2014-02-15
----------------------

- Fix fs.mkdir that didn't return an error,
- Add missing optional mode parameter in fs.mkdir documentation,
- Also add irccd.VERSION_PATCH.

irccd 1.1.0 2014-01-30
----------------------

- Added support for UDP sockets,
- Added a plugin for authentication,
- Windows irccd's home is now the irccd.exe parent directory,
- Added new socket API for Lua,
- Added new thread API for Lua,
- Added support for server reconnection,
- Added support for text formatting with colors and attributes,
- Added support for onMe (CTCP Action) event,
- Added new way to load plugin by paths,
- Server:whois and server:names generate a new events instead of callback,
- Support of connecting and disconnecting at runtime,
- Plugin has more information, getHome() and getName() are deprecated,
- Split irccd.util into irccd.fs and irccd.system,
- Added support for LuaJIT.

irccd 1.0.2 2013-11-01
----------------------

- Errata, onMe event is not implemented.

irccd 1.0.1 2013-09-17
----------------------

- Fixed build without Lua,
- Improved documentation a lot,
- Improved NSIS installer,
- Fixed basename() issue.

irccd 1.0.0 2013-09-13
----------------------

- Initial release.
