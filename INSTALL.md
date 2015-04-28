IRC Client Daemon INSTALL
=========================

This guide will help you to install irccd on your computer. For a better guide
with more details see:

  http://projects.malikania.fr/irccd/doc/guides/user-guide.html

Requirements
============

- C++14 compiler, you need a compiler that supports C++14.

- [CMake](http://www.cmake.org). Version 3.0.0 recommended. This tool is
  required to build irccd.

Optional:

- [OpenSSL](http://openssl.org), Used for SSL connections to IRC servers,
  recommended.

- [Pandoc](http://johnmacfarlane.net/pandoc). Used for documentation process.

- [Doxygen](http://www.stack.nl/~dimitri/doxygen). For the documentation about
  irccd internals.

- LaTeX, used to generate guides as PDF, you can still keep HTML documentation
  instead.

Basic installation
==================

This is the quick way to install irccd.

````
tar xvzf irccd-x.y.tar.gz
cd irccd-x.y
mkdir _build_
cd _build_
cmake .. -DCMAKE_BUILD_TYPE=Release
make
sudo make install
````

Do not forget to adapt x.y with the current version.

Tweaking installation
=====================

If you want to disable or change some component, you need to adjust one of
the following CMake variable.

Disabling JavaScript
--------------------

If you want to build irccd without Lua, use this CMake argument:

````
cmake .. -DWITH_JS=Off
````

Disabling all documentation
---------------------------

You can disable all documentation if you want:

````
cmake .. -DWITH_DOCS=Off
````

See next sections if you only want to disable specific documentation parts.

Disabling JavaScript documentation
----------------------------------

If you don't want JavaScript API documentation, use this:

````
cmake .. -DWITH_DOCS_JS=Off
````

**Note**: automatically disabled if pandoc is not found.

Disabling user guides
---------------------

You can disable PDF or HTML user guides generation.

Disable PDF:

````
cmake .. -DWITH_DOCS_GUIDES_PDF=Off
````

**Note**: automatically disabled if pandoc or LaTeX are not found.

Disable HTML:

````
cmake .. -DWITH_DOCS_GUIDES_HTML=Off
````

**Note**: automatically disabled if pandoc is not found.

Disabling manuals
-----------------

This disable manual pages, it's already the default on Windows.

$ cmake .. -WITH_MAN=Off

Directories
===========

It's also possible to set different directories for irccd components.

Adjusting the installation directory
------------------------------------

By default, irccd will usually use /usr/local as the prefix for
installation. To change this, use the following:

````
cmake .. -DCMAKE_INSTALL_PREFIX=/new/directory
````

Manual directories
------------------

By default, irccd use ${CMAKE_INSTALL_PREFIX}/share/man for manual pages.
Some systems use different one.

For example, on FreeBSD the typical use would be:

````
cmake .. -DMANDIR=/usr/local/man
````
