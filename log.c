/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2011-2012 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2014 Matthias Heidbrink <m-sigrok@heidbrink.biz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "libsigrok.h"

/** @cond PRIVATE */
#define NO_LOG_WRAPPERS
/** @endcond */
#include "libsigrok-internal.h"

/**
 * @file
 *
 * Controlling the libsigrok message logging functionality.
 */

/**
 * @defgroup grp_logging Logging
 *
 * Controlling the libsigrok message logging functionality.
 *
 * @{
 */

/* Currently selected libsigrok loglevel. Default: SR_LOG_WARN. */
static int cur_loglevel = SR_LOG_WARN; /* Show errors+warnings per default. */

/* Currently selected libsigrok log options. Default: SR_LOG_NOOPTS. */
static int cur_logopts = SR_LOG_NOOPTS; /* Don't show date and time per default. */

/* Function prototype. */
static int sr_logv(void *cb_data, int loglevel, const char *format,
		   va_list args);

/* Pointer to the currently selected log callback. Default: sr_logv(). */
static sr_log_callback sr_log_cb = sr_logv;

/*
 * Pointer to private data that can be passed to the log callback.
 * This can be used (for example) by C++ GUIs to pass a "this" pointer.
 */
static void *sr_log_cb_data = NULL;

/* Log domain (a short string that is used as prefix for all messages). */
/** @cond PRIVATE */
#define LOGDOMAIN_MAXLEN 30
#define LOGDOMAIN_DEFAULT "sr: "
/** @endcond */
static char sr_log_domain[LOGDOMAIN_MAXLEN + 1] = LOGDOMAIN_DEFAULT;

/**
 * Set the libsigrok loglevel.
 *
 * This influences the amount of log messages (debug messages, error messages,
 * and so on) libsigrok will output. Using SR_LOG_NONE disables all messages.
 *
 * Note that this function itself will also output log messages. After the
 * loglevel has changed, it will output a debug message with SR_LOG_DBG for
 * example. Whether this message is shown depends on the (new) loglevel.
 *
 * @param[in] loglevel The loglevel to set (SR_LOG_NONE, SR_LOG_ERR, SR_LOG_WARN,
 *                 SR_LOG_INFO, SR_LOG_DBG or SR_LOG_SPEW plus optional
 *                 members of SR_LOG_DATETIME_MASK).
 *
 * @retval SR_OK Success.
 * @retval SR_ERR_ARG Invalid loglevel.
 *
 * @since 0.1.0
 */
SR_API int sr_log_loglevel_set(int loglevel)
{
	if (loglevel < SR_LOG_NONE || loglevel > SR_LOG_SPEW) {
		sr_err("Invalid loglevel %d.", loglevel);
		return SR_ERR_ARG;
	}

	cur_loglevel = loglevel;

	sr_dbg("libsigrok loglevel set to %d.", loglevel);

	return SR_OK;
}

/**
 * Get the libsigrok loglevel.
 *
 * @return The currently configured libsigrok loglevel.
 *
 * @since 0.1.0
 */
SR_API int sr_log_loglevel_get(void)
{
	return cur_loglevel;
}

/**
 * Set the libsigrok log options.
 *
 * This influences the format of log messages libsigrok will output, e.g. date
 * and time stamps. Using SR_LOG_NOOPTS disables non-elementary elements.
 *
 * Note that the accuracy and granularity of dates and timestamps, especially
 * ms and us timestamps, is influenced by the computer's hardware and operating
 * system.
 *
 * Also note that this function itself will also output log messages. After the
 * log options have changed, it will output a debug message with SR_LOG_DBG for
 * example. Whether this message is shown depends on the (new) loglevel.
 *
 * @param[in] logopts The log options to set (SR_LOG_NOOPTS or a combination of
 *                 SR_LOG_DATE, SR_LOG_TIME, SR_LOG_TIME_MS, SR_LOG_TIME_US,
 *                 SR_LOG_UTC).
 *
 * @retval SR_OK Success.
 * @retval SR_ERR_ARG Invalid log options.
 *
 * @since 0.3.1
 */
SR_API int sr_log_logopts_set(int logopts)
{
	if ((logopts < SR_LOG_NOOPTS) ||
	    (logopts > (SR_LOG_DATE | SR_LOG_TIME | SR_LOG_TIME_MS | SR_LOG_TIME_US | SR_LOG_UTC)))
	{
		sr_err("Invalid log options %d.", logopts);
		return SR_ERR_ARG;
	}

	cur_logopts = logopts;

	sr_dbg("libsigrok log options set to %d.", cur_logopts);

	return SR_OK;
}

/**
 * Get the libsigrok log options.
 *
 * @return The currently configured libsigrok log options.
 *
 * @since 0.3.1
 */
SR_API int sr_log_logopts_get(void)
{
	return cur_logopts;
}


/**
 * Set the libsigrok logdomain string.
 *
 * @param[in] logdomain The string to use as logdomain for libsigrok log
 *                  messages from now on. Must not be NULL. The maximum
 *                  length of the string is 30 characters (this does not
 *                  include the trailing NUL-byte). Longer strings are
 *                  silently truncated.
 *                  In order to not use a logdomain, pass an empty string.
 *                  The function makes its own copy of the input string, i.e.
 *                  the caller does not need to keep it around.
 *
 * @retval SR_OK Success.
 * @retval SR_ERR_ARG Invalid logdomain.
 *
 * @since 0.1.0
 */
SR_API int sr_log_logdomain_set(const char *logdomain)
{
	if (!logdomain) {
		sr_err("log: %s: logdomain was NULL", __func__);
		return SR_ERR_ARG;
	}

	/* TODO: Error handling. */
	snprintf((char *)&sr_log_domain, LOGDOMAIN_MAXLEN, "%s", logdomain);

	sr_dbg("Log domain set to '%s'.", (const char *)&sr_log_domain);

	return SR_OK;
}

/**
 * Get the currently configured libsigrok logdomain.
 *
 * @return A copy of the currently configured libsigrok logdomain
 *         string. The caller is responsible for g_free()ing the string when
 *         it is no longer needed.
 *
 * @since 0.1.0
 */
SR_API char *sr_log_logdomain_get(void)
{
	return g_strdup((const char *)&sr_log_domain);
}

/**
 * Set the libsigrok log callback to the specified function.
 *
 * @param cb Function pointer to the log callback function to use.
 *           Must not be NULL.
 * @param cb_data Pointer to private data to be passed on. This can be used by
 *                the caller to pass arbitrary data to the log functions. This
 *                pointer is only stored or passed on by libsigrok, and is
 *                never used or interpreted in any way. The pointer is allowed
 *                to be NULL if the caller doesn't need/want to pass any data.
 *
 * @retval SR_OK Success.
 * @retval SR_ERR_ARG Invalid arguments.
 *
 * @since 0.3.0
 */
SR_API int sr_log_callback_set(sr_log_callback cb, void *cb_data)
{
	if (!cb) {
		sr_err("log: %s: cb was NULL", __func__);
		return SR_ERR_ARG;
	}

	/* Note: 'cb_data' is allowed to be NULL. */

	sr_log_cb = cb;
	sr_log_cb_data = cb_data;

	return SR_OK;
}

/**
 * Set the libsigrok log callback to the default built-in one.
 *
 * Additionally, the internal 'sr_log_cb_data' pointer is set to NULL.
 *
 * @retval SR_OK Success.
 * @retval other Negative error code.
 *
 * @since 0.1.0
 */
SR_API int sr_log_callback_set_default(void)
{
	/*
	 * Note: No log output in this function, as it should safely work
	 * even if the currently set log callback is buggy/broken.
	 */
	sr_log_cb = sr_logv;
	sr_log_cb_data = NULL;

	return SR_OK;
}

static int sr_logv(void *cb_data, int loglevel, const char *format, va_list args)
{
	int ret;

	/* This specific log callback doesn't need the void pointer data. */
	(void)cb_data;

	/* Only output messages of at least the selected loglevel(s). */
	if (loglevel > cur_loglevel)
		return SR_OK; /* TODO? */

	ret = 0;

	/* Log date/time timestamp */
	if (cur_logopts & (SR_LOG_DATE | SR_LOG_TIME | SR_LOG_TIME_MS | SR_LOG_TIME_US)) {
		gint64 dt_us = g_get_real_time();
		time_t dt_timet = (time_t) (dt_us / 1000000);
		struct tm dt_tm;
		gchar buf[30];
#ifdef G_OS_WIN32
		if (cur_logopts & SR_LOG_UTC) {
			if (gmtime_s(&dt_tm, &dt_timet) != 0)
				ret += fprintf(stderr, "gmtime_s() failed. ");
			/* Do not return on error to prevent being unable to output error msgs at all! */
		}
		else if (localtime_s(&dt_tm, &dt_timet) != 0)
			ret += fprintf(stderr, "localtime_s() failed. ");
#else
		if (cur_logopts & SR_LOG_UTC) {
			if (!gmtime_r(&dt_timet, &dt_tm))
				ret += fprintf(stderr, "gmtime_r() failed: %d %s ", errno, strerror(errno));
		}
		else if (!localtime_r(&dt_timet, &dt_tm))
			ret += fprintf(stderr, "localtime_r() failed: %d %s ", errno, strerror(errno));
#endif
		if (cur_logopts & SR_LOG_DATE) {
			strftime(buf, sizeof(buf), "%Y%m%d ", &dt_tm);
			ret += fprintf(stderr, "%s", buf);
		}
		if (cur_logopts & (SR_LOG_TIME | SR_LOG_TIME_MS |SR_LOG_TIME_US)) {
			strftime(buf, sizeof(buf), "%H%M%S", &dt_tm);
			ret += fprintf(stderr, "%s", buf);
			if (cur_logopts & SR_LOG_TIME_US)
				ret += fprintf(stderr, ",%06u ", (guint) (dt_us % 1000000));
			else if (cur_logopts & SR_LOG_TIME_MS)
				ret += fprintf(stderr, ",%03u ", (guint) ((dt_us / 1000) % 1000));
			else /* Resolution seconds only */
				ret += fprintf(stderr, " ");
		}
	}

	/* Log domain and message */
	if (sr_log_domain[0] != '\0')
		fprintf(stderr, "%s", sr_log_domain);
	ret += vfprintf(stderr, format, args);
	fprintf(stderr, "\n");

	return ret;
}

/** @private */
SR_PRIV int sr_log(int loglevel, const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = sr_log_cb(sr_log_cb_data, loglevel, format, args);
	va_end(args);

	return ret;
}

/** @private */
SR_PRIV int sr_spew(const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = sr_log_cb(sr_log_cb_data, SR_LOG_SPEW, format, args);
	va_end(args);

	return ret;
}

/** @private */
SR_PRIV int sr_dbg(const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = sr_log_cb(sr_log_cb_data, SR_LOG_DBG, format, args);
	va_end(args);

	return ret;
}

/** @private */
SR_PRIV int sr_info(const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = sr_log_cb(sr_log_cb_data, SR_LOG_INFO, format, args);
	va_end(args);

	return ret;
}

/** @private */
SR_PRIV int sr_warn(const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = sr_log_cb(sr_log_cb_data, SR_LOG_WARN, format, args);
	va_end(args);

	return ret;
}

/** @private */
SR_PRIV int sr_err(const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = sr_log_cb(sr_log_cb_data, SR_LOG_ERR, format, args);
	va_end(args);

	return ret;
}

/** @} */
