/*
 * JsServer.cpp -- server management for irccd JS API
 *
 * Copyright (c) 2013, 2014, 2015 David Demelier <markand@malikania.fr>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sstream>
#include <unordered_map>

#include "Server.h"
#include "Js.h"

namespace irccd {

namespace {

/*
 * Method: Server.cnotice(channel, message)
 * --------------------------------------------------------
 *
 * Send a channel notice.
 *
 * Arguments:
 *   - channel, the channel
 *   - message, the message
 */
duk_ret_t Server_prototype_cnotice(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<std::shared_ptr<Server>>(ctx, [&] (std::shared_ptr<Server> &s) {
		s->cnotice(duk_require_string(ctx, 0), duk_require_string(ctx, 1));
	});
	dukx_assert_equals(ctx);

	return 0;
}

/*
 * Method: Server.invite(target, channel)
 * --------------------------------------------------------
 *
 * Invite someone to a channel.
 *
 * Arguments:
 *   - target, the target to invite
 *   - channel, the channel
 */
duk_ret_t Server_prototype_invite(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<std::shared_ptr<Server>>(ctx, [&] (std::shared_ptr<Server> &s) {
		s->invite(duk_require_string(ctx, 0), duk_require_string(ctx, 1));
	});
	dukx_assert_equals(ctx);

	return 0;
}

/*
 * Method: Server.join(channel, password = undefined)
 * --------------------------------------------------------
 *
 * Join a channel with an optional password.
 *
 * Arguments:
 *   - channel, the channel to join
 *   - password, the password or undefined to not use
 */
duk_ret_t Server_prototype_join(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<std::shared_ptr<Server>>(ctx, [&] (std::shared_ptr<Server> &s) {
		const char *channel = duk_require_string(ctx, 0);
		const char *password = "";

		if (duk_get_top(ctx) == 2) {
			password = duk_require_string(ctx, 1);
		}

		s->join(channel, password);
	});
	dukx_assert_equals(ctx);

	return 0;
}

/*
 * Method: Server.kick(target, channel, reason = undefined)
 * --------------------------------------------------------
 *
 * Kick someone from a channel.
 *
 * Arguments:
 *   - target, the target to kick
 *   - channel, the channel
 *   - reason, the optional reason or undefined to not set
 */
duk_ret_t Server_prototype_kick(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<std::shared_ptr<Server>>(ctx, [&] (std::shared_ptr<Server> &s) {
		const char *target = duk_require_string(ctx, 0);
		const char *channel = duk_require_string(ctx, 1);
		const char *reason = "";

		if (duk_get_top(ctx) == 3) {
			reason = duk_require_string(ctx, 2);
		}

		s->kick(target, channel, reason);
	});
	dukx_assert_equals(ctx);

	return 0;
}

/*
 * Method: Server.me(target, message)
 * --------------------------------------------------------
 *
 * Send a CTCP Action.
 *
 * Arguments:
 *   - target, the target or a channel
 *   - message, the message
 */
duk_ret_t Server_prototype_me(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<std::shared_ptr<Server>>(ctx, [&] (std::shared_ptr<Server> &s) {
		s->me(duk_require_string(ctx, 0), duk_require_string(ctx, 1));
	});
	dukx_assert_equals(ctx);

	return 0;
}

/*
 * Method: Server.message(target, message)
 * --------------------------------------------------------
 *
 * Send a message.
 *
 * Arguments:
 *   - target, the target or a channel
 *   - message, the message
 */
duk_ret_t Server_prototype_message(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<std::shared_ptr<Server>>(ctx, [&] (std::shared_ptr<Server> &s) {
		s->message(duk_require_string(ctx, 0), duk_require_string(ctx, 1));
	});
	dukx_assert_equals(ctx);

	return 0;
}

/*
 * Method: Server.mode(channel, mode)
 * --------------------------------------------------------
 *
 * Change a channel mode.
 *
 * Arguments:
 *   - channel, the channel
 *   - mode, the mode
 */
duk_ret_t Server_prototype_mode(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<std::shared_ptr<Server>>(ctx, [&] (std::shared_ptr<Server> &s) {
		s->mode(duk_require_string(ctx, 0), duk_require_string(ctx, 1));
	});
	dukx_assert_equals(ctx);

	return 0;
}

/*
 * Method: Server.names(channel)
 * --------------------------------------------------------
 *
 * Get the list of names from a channel.
 *
 * Arguments:
 *   - channel, the channel
 */
duk_ret_t Server_prototype_names(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<std::shared_ptr<Server>>(ctx, [&] (std::shared_ptr<Server> &s) {
		s->names(duk_require_string(ctx, 0));
	});
	dukx_assert_equals(ctx);

	return 0;
}

/*
 * Method: Server.nick(nickname)
 * --------------------------------------------------------
 *
 * Change the nickname.
 *
 * Arguments:
 *   - nickname, the nickname
 */
duk_ret_t Server_prototype_nick(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<std::shared_ptr<Server>>(ctx, [&] (std::shared_ptr<Server> &s) {
		s->nick(duk_require_string(ctx, 0));
	});
	dukx_assert_equals(ctx);

	return 0;
}

/*
 * Method: Server.notice(target, message)
 * --------------------------------------------------------
 *
 * Send a private notice.
 *
 * Arguments:
 *   - target, the target
 *   - message, the notice message
 */
duk_ret_t Server_prototype_notice(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<std::shared_ptr<Server>>(ctx, [&] (std::shared_ptr<Server> &s) {
		s->notice(duk_require_string(ctx, 0), duk_require_string(ctx, 1));
	});
	dukx_assert_equals(ctx);

	return 0;
}

/*
 * Method: Server.part(channel, reason = undefined)
 * --------------------------------------------------------
 *
 * Leave a channel.
 *
 * Arguments:
 *   - channel, the channel to leave
 *   - reason, the optional reason, keep undefined for portability
 */
duk_ret_t Server_prototype_part(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<std::shared_ptr<Server>>(ctx, [&] (std::shared_ptr<Server> &s) {
		const char *channel = duk_require_string(ctx, 0);
		const char *reason = "";

		if (duk_get_top(ctx) == 2) {
			reason = duk_require_string(ctx, 1);
		}

		s->part(channel, reason);
	});
	dukx_assert_equals(ctx);

	return 0;
}

/*
 * Method: Server.send(raw)
 * --------------------------------------------------------
 *
 * Send a raw message to the IRC server.
 *
 * Arguments:
 *   - raw, the raw message (without terminators)
 */
duk_ret_t Server_prototype_send(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<std::shared_ptr<Server>>(ctx, [&] (std::shared_ptr<Server> &s) {
		s->send(duk_require_string(ctx, 0));
	});
	dukx_assert_equals(ctx);

	return 0;
}

/*
 * Method: Server.topic(channel, topic)
 * --------------------------------------------------------
 *
 * Change a channel topic.
 *
 * Arguments:
 *   - channel, the channel
 *   - topic, the new topic
 */
duk_ret_t Server_prototype_topic(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<std::shared_ptr<Server>>(ctx, [&] (std::shared_ptr<Server> &s) {
		s->topic(duk_require_string(ctx, 0), duk_require_string(ctx, 1));
	});
	dukx_assert_equals(ctx);

	return 0;
}

/*
 * Method: Server.umode(mode)
 * --------------------------------------------------------
 *
 * Change your mode.
 *
 * Arguments:
 *   - mode, the new mode
 */
duk_ret_t Server_prototype_umode(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<std::shared_ptr<Server>>(ctx, [&] (std::shared_ptr<Server> &s) {
		s->umode(duk_require_string(ctx, 0));
	});
	dukx_assert_equals(ctx);

	return 0;
}

/*
 * Method: Server.whois(target)
 * --------------------------------------------------------
 *
 * Get whois information.
 *
 * Arguments:
 *   - target, the target
 */
duk_ret_t Server_prototype_whois(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<std::shared_ptr<Server>>(ctx, [&] (std::shared_ptr<Server> &s) {
		s->whois(duk_require_string(ctx, 0));
	});
	dukx_assert_equals(ctx);

	return 0;
}

/*
 * Method: toString()
 * --------------------------------------------------------
 *
 * Convert the object to string, convenience for adding the object
 * as property key.
 *
 * Returns:
 *   - the server name (unique)
 */
duk_ret_t Server_prototype_toString(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	dukx_with_this<std::shared_ptr<Server>>(ctx, [&] (std::shared_ptr<Server> &s) {
		duk_push_string(ctx, s->info().name.c_str());
	});
	dukx_assert_end(ctx, 1);

	return 1;
}

const duk_function_list_entry serverMethods[] = {
	/* Server methods */
	{ "cnotice",	Server_prototype_cnotice,	2		},
	{ "invite",	Server_prototype_invite,	2		},
	{ "join",	Server_prototype_join,		DUK_VARARGS	},
	{ "kick",	Server_prototype_kick,		DUK_VARARGS	},
	{ "me",		Server_prototype_me,		2		},
	{ "message",	Server_prototype_message,	2		},
	{ "mode",	Server_prototype_mode,		2		},
	{ "names",	Server_prototype_names,		1		},
	{ "nick",	Server_prototype_nick,		1		},
	{ "notice",	Server_prototype_notice,	2		},
	{ "part",	Server_prototype_part,		DUK_VARARGS	},
	{ "send",	Server_prototype_send,		1		},
	{ "topic",	Server_prototype_topic,		2		},
	{ "umode",	Server_prototype_umode,		1		},
	{ "whois",	Server_prototype_whois,		1		},

	/* Special */
	{ "toString",	Server_prototype_toString,	0		},
	{ nullptr,	nullptr,			0		}
};

} // !namespace

duk_ret_t dukopen_server(duk_context *ctx) noexcept
{
	duk_push_object(ctx);

	return 1;
}

void dukpreload_server(duk_context *ctx) noexcept
{
	dukx_assert_begin(ctx);
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "\xff" "irccd-proto");
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, serverMethods);
	duk_put_prop_string(ctx, -2, "Server");
	duk_pop_2(ctx);
	dukx_assert_equals(ctx);
}

} // !irccd
