// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

// this file contains mostly source code, derived from Apache Portable Runtime

/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * apr_date.c: date parsing utility routines
 *     These routines are (hopefully) platform independent.
 *
 * 27 Oct 1996  Roy Fielding
 *     Extracted (with many modifications) from mod_proxy.c and
 *     tested with over 50,000 randomly chosen valid date strings
 *     and several hundred variations of invalid date strings.
 *
 */

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

#include "SPCore.h"
#include "SPTime.h"

NS_SP_BEGIN

static constexpr uint64_t SP_USEC_PER_SEC(1000000);

using sp_time_t = uint32_t;

struct sp_time_exp_t {
	/** microseconds past tm_sec */
	int32_t tm_usec;
	/** (0-61) seconds past tm_min */
	int32_t tm_sec;
	/** (0-59) minutes past tm_hour */
	int32_t tm_min;
	/** (0-23) hours past midnight */
	int32_t tm_hour;
	/** (1-31) day of the month */
	int32_t tm_mday;
	/** (0-11) month of the year */
	int32_t tm_mon;
	/** year since 1900 */
	int32_t tm_year;
	/** (0-6) days since Sunday */
	int32_t tm_wday;
	/** (0-365) days since January 1 */
	int32_t tm_yday;
	/** daylight saving time */
	int32_t tm_isdst;
	/** seconds east of UTC */
	int32_t tm_gmtoff;

	sp_time_exp_t() {
		tm_usec = 0;
		tm_sec = 0;
		tm_min = 0;
		tm_hour = 0;
		tm_mday = 0;
		tm_mon = 0;
		tm_year = 0;
		tm_wday = 0;
		tm_yday = 0;
		tm_isdst = 0;
		tm_gmtoff = 0;
	}

	sp_time_exp_t(Time t, int32_t offset, bool use_localtime) {
		struct tm tm;
		time_t tt = (t.toSeconds()) + offset;
		tm_usec = t.toMicroseconds() % SP_USEC_PER_SEC;

		if (use_localtime)
			localtime_r(&tt, &tm);
		else
			gmtime_r(&tt, &tm);

		tm_sec = tm.tm_sec;
		tm_min = tm.tm_min;
		tm_hour = tm.tm_hour;
		tm_mday = tm.tm_mday;
		tm_mon = tm.tm_mon;
		tm_year = tm.tm_year;
		tm_wday = tm.tm_wday;
		tm_yday = tm.tm_yday;
		tm_isdst = tm.tm_isdst;
		tm_gmtoff = tm.tm_gmtoff;
	}

	// apr_time_exp_tz
	sp_time_exp_t(Time t, int32_t offs) : sp_time_exp_t(t, offs, false) {
	    tm_gmtoff = offs;
	}

	// apr_time_exp_gmt
	sp_time_exp_t(Time t) : sp_time_exp_t(t, 0, 0) {
		tm_gmtoff = 0;
	}

	// apr_time_exp_lt
	sp_time_exp_t(Time t, bool use_localtime) : sp_time_exp_t(t, 0, use_localtime) { }

	Time get() const {
		sp_time_t year = tm_year;
		sp_time_t days;
		static const int dayoffset[12] = { 306, 337, 0, 31, 61, 92, 122, 153, 184, 214, 245, 275 };

		/* shift new year to 1st March in order to make leap year calc easy */

		if (tm_mon < 2)
			year--;

		/* Find number of days since 1st March 1900 (in the Gregorian calendar). */

		days = year * 365 + year / 4 - year / 100 + (year / 100 + 3) / 4;
		days += dayoffset[tm_mon] + tm_mday - 1;
		days -= 25508; /* 1 jan 1970 is 25508 days since 1 mar 1900 */
		days = ((days * 24 + tm_hour) * 60 + tm_min) * 60 + tm_sec;

		return Time::microseconds(days * SP_USEC_PER_SEC + tm_usec);
	}

	Time gmt_get() const {
		return Time::microseconds(get().toMicroseconds() - tm_gmtoff * SP_USEC_PER_SEC);
	}

	Time ltz_get() const {
		time_t t = time(NULL);
		struct tm lt = {0};
		localtime_r(&t, &lt);

		return Time::microseconds(get().toMicroseconds() - lt.tm_gmtoff * SP_USEC_PER_SEC);
	}
};

/*
 * Compare a string to a mask
 * Mask characters (arbitrary maximum is 256 characters, just in case):
 *   @ - uppercase letter
 *   $ - lowercase letter
 *   & - hex digit
 *   # - digit
 *   ~ - digit or space
 *   * - swallow remaining characters
 *  <x> - exact match for any other character
 */
static int sp_date_checkmask(const char *data, const char *mask) {
	int i;
	char d;

	for (i = 0; i < 256; i++) {
		d = data[i];
		switch (mask[i]) {
		case '\0':
			return (d == '\0');

		case '*':
			return 1;

		case '@':
			if (!chars::isupper(d))
				return 0;
			break;
		case '$':
			if (!chars::islower(d))
				return 0;
			break;
		case '#':
			if (!chars::isdigit(d))
				return 0;
			break;
		case '&':
			if (!chars::isxdigit(d))
				return 0;
			break;
		case '~':
			if ((d != ' ') && !chars::isdigit(d))
				return 0;
			break;
		default:
			if (mask[i] != d)
				return 0;
			break;
		}
	}
	return 0; /* We only get here if mask is corrupted (exceeds 256) */
}

/*
 * Parses an HTTP date in one of three standard forms:
 *
 *     Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
 *     Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
 *     Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format
 *
 * and returns the apr_time_t number of microseconds since 1 Jan 1970 GMT,
 * or APR_DATE_BAD if this would be out of range or if the date is invalid.
 *
 * The restricted HTTP syntax is
 *
 *     HTTP-date    = rfc1123-date | rfc850-date | asctime-date
 *
 *     rfc1123-date = wkday "," SP date1 SP time SP "GMT"
 *     rfc850-date  = weekday "," SP date2 SP time SP "GMT"
 *     asctime-date = wkday SP date3 SP time SP 4DIGIT
 *
 *     date1        = 2DIGIT SP month SP 4DIGIT
 *                    ; day month year (e.g., 02 Jun 1982)
 *     date2        = 2DIGIT "-" month "-" 2DIGIT
 *                    ; day-month-year (e.g., 02-Jun-82)
 *     date3        = month SP ( 2DIGIT | ( SP 1DIGIT ))
 *                    ; month day (e.g., Jun  2)
 *
 *     time         = 2DIGIT ":" 2DIGIT ":" 2DIGIT
 *                    ; 00:00:00 - 23:59:59
 *
 *     wkday        = "Mon" | "Tue" | "Wed"
 *                  | "Thu" | "Fri" | "Sat" | "Sun"
 *
 *     weekday      = "Monday" | "Tuesday" | "Wednesday"
 *                  | "Thursday" | "Friday" | "Saturday" | "Sunday"
 *
 *     month        = "Jan" | "Feb" | "Mar" | "Apr"
 *                  | "May" | "Jun" | "Jul" | "Aug"
 *                  | "Sep" | "Oct" | "Nov" | "Dec"
 *
 * However, for the sake of robustness (and Netscapeness), we ignore the
 * weekday and anything after the time field (including the timezone).
 *
 * This routine is intended to be very fast; 10x faster than using sscanf.
 *
 * Originally from Andrew Daviel <andrew@vancouver-webpages.com>, 29 Jul 96
 * but many changes since then.
 *
 */

static const int s_months[12] = {
		('J' << 16) | ('a' << 8) | 'n', ('F' << 16) | ('e' << 8) | 'b',
		('M' << 16) | ('a' << 8) | 'r', ('A' << 16) | ('p' << 8) | 'r',
		('M' << 16) | ('a' << 8) | 'y', ('J' << 16) | ('u' << 8) | 'n',
		('J' << 16) | ('u' << 8) | 'l', ('A' << 16) | ('u' << 8) | 'g',
		('S' << 16) | ('e' << 8) | 'p', ('O' << 16) | ('c' << 8) | 't',
		('N' << 16) | ('o' << 8) | 'v', ('D' << 16) | ('e' << 8) | 'c'
};

Time Time::fromCompileTime(const char *date, const char *time) {
	sp_time_exp_t ds;
	ds.tm_year = ((date[7] - '0') * 10 + (date[8] - '0') - 19) * 100;
	if (ds.tm_year < 0) {
		return Time();
	}

	ds.tm_year += ((date[9] - '0') * 10) + (date[10] - '0');
	ds.tm_mday = ((date[4] != ' ') ? ((date[4] - '0') * 10) : 0) + (date[5] - '0');

	int mint = (date[0] << 16) | (date[1] << 8) | date[2];
	int mon = 0;
	for (; mon < 12; mon++) {
		if (mint == s_months[mon])
			break;
	}

	if (mon == 12)
		return Time();

	if (ds.tm_mday <= 0 || ds.tm_mday > 31)
		return Time();

	if ((ds.tm_mday == 31) && (mon == 3 || mon == 5 || mon == 8 || mon == 10))
		return Time();

	if ((mon == 1)
			&& ((ds.tm_mday > 29)
					|| ((ds.tm_mday == 29)
							&& ((ds.tm_year & 3) || (((ds.tm_year % 100) == 0) && (((ds.tm_year % 400) != 100))))
					)
			))
		return Time();

	ds.tm_mon = mon;
	ds.tm_hour = ((time[0] - '0') * 10) + (time[1] - '0');
	ds.tm_min = ((time[3] - '0') * 10) + (time[4] - '0');
	ds.tm_sec = ((time[6] - '0') * 10) + (time[7] - '0');

	if ((ds.tm_hour > 23) || (ds.tm_min > 59) || (ds.tm_sec > 61))
		return Time();

	ds.tm_usec = 0;
	ds.tm_gmtoff = 0;
	return ds.ltz_get();
}

Time Time::fromHttp(const StringView &r) {
	auto date = r.data();
	sp_time_exp_t ds;
	int mint, mon;
	const char *monstr, *timstr;
	if (!date)
		return Time();

	while (*date && chars::isspace(*date)) /* Find first non-whitespace char */
		++date;

	if (*date == '\0')
		return Time();

	if ((date = strchr(date, ' ')) == NULL) /* Find space after weekday */
		return Time();

	++date; /* Now pointing to first char after space, which should be */

	/* start of the actual date information for all 4 formats. */

	if (sp_date_checkmask(date, "## @$$ #### ##:##:## *")) {
		/* RFC 1123 format with two days */
		ds.tm_year = ((date[7] - '0') * 10 + (date[8] - '0') - 19) * 100;
		if (ds.tm_year < 0)
			return Time();

		ds.tm_year += ((date[9] - '0') * 10) + (date[10] - '0');

		ds.tm_mday = ((date[0] - '0') * 10) + (date[1] - '0');

		monstr = date + 3;
		timstr = date + 12;
	} else if (sp_date_checkmask(date, "##-@$$-## ##:##:## *")) {
		/* RFC 850 format */
		ds.tm_year = ((date[7] - '0') * 10) + (date[8] - '0');
		if (ds.tm_year < 70)
			ds.tm_year += 100;

		ds.tm_mday = ((date[0] - '0') * 10) + (date[1] - '0');

		monstr = date + 3;
		timstr = date + 10;
	} else if (sp_date_checkmask(date, "@$$ ~# ##:##:## ####*")) {
		/* asctime format */
		ds.tm_year = ((date[16] - '0') * 10 + (date[17] - '0') - 19) * 100;
		if (ds.tm_year < 0)
			return Time();

		ds.tm_year += ((date[18] - '0') * 10) + (date[19] - '0');

		if (date[4] == ' ')
			ds.tm_mday = 0;
		else
			ds.tm_mday = (date[4] - '0') * 10;

		ds.tm_mday += (date[5] - '0');

		monstr = date;
		timstr = date + 7;
	} else if (sp_date_checkmask(date, "# @$$ #### ##:##:## *")) {
		/* RFC 1123 format with one day */
		ds.tm_year = ((date[6] - '0') * 10 + (date[7] - '0') - 19) * 100;
		if (ds.tm_year < 0)
			return Time();

		ds.tm_year += ((date[8] - '0') * 10) + (date[9] - '0');

		ds.tm_mday = (date[0] - '0');

		monstr = date + 2;
		timstr = date + 11;
	} else
		return Time();

	if (ds.tm_mday <= 0 || ds.tm_mday > 31)
		return Time();

	ds.tm_hour = ((timstr[0] - '0') * 10) + (timstr[1] - '0');
	ds.tm_min = ((timstr[3] - '0') * 10) + (timstr[4] - '0');
	ds.tm_sec = ((timstr[6] - '0') * 10) + (timstr[7] - '0');

	if ((ds.tm_hour > 23) || (ds.tm_min > 59) || (ds.tm_sec > 61))
		return Time();

	mint = (monstr[0] << 16) | (monstr[1] << 8) | monstr[2];
	for (mon = 0; mon < 12; mon++)
		if (mint == s_months[mon])
			break;

	if (mon == 12)
		return Time();

	if ((ds.tm_mday == 31) && (mon == 3 || mon == 5 || mon == 8 || mon == 10))
		return Time();

	/* February gets special check for leapyear */
	if ((mon == 1)
			&& ((ds.tm_mday > 29)
					|| ((ds.tm_mday == 29)
							&& ((ds.tm_year & 3) || (((ds.tm_year % 100) == 0) && (((ds.tm_year % 400) != 100))))
					)
			))
		return Time();

	ds.tm_mon = mon;

	/* ap_mplode_time uses tm_usec and tm_gmtoff fields, but they haven't
	 * been set yet.
	 * It should be safe to just zero out these values.
	 * tm_usec is the number of microseconds into the second.  HTTP only
	 * cares about second granularity.
	 * tm_gmtoff is the number of seconds off of GMT the time is.  By
	 * definition all times going through this function are in GMT, so this
	 * is zero.
	 */
	ds.tm_usec = 0;
	ds.tm_gmtoff = 0;

	return ds.get();
}

/*
 * Parses a string resembling an RFC 822 date.  This is meant to be
 * leinent in its parsing of dates.  Hence, this will parse a wider
 * range of dates than apr_date_parse_http.
 *
 * The prominent mailer (or poster, if mailer is unknown) that has
 * been seen in the wild is included for the unknown formats.
 *
 *     Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
 *     Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
 *     Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format
 *     Sun, 6 Nov 1994 08:49:37 GMT   ; RFC 822, updated by RFC 1123
 *     Sun, 06 Nov 94 08:49:37 GMT    ; RFC 822
 *     Sun, 6 Nov 94 08:49:37 GMT     ; RFC 822
 *     Sun, 06 Nov 94 08:49 GMT       ; Unknown [drtr@ast.cam.ac.uk]
 *     Sun, 6 Nov 94 08:49 GMT        ; Unknown [drtr@ast.cam.ac.uk]
 *     Sun, 06 Nov 94 8:49:37 GMT     ; Unknown [Elm 70.85]
 *     Sun, 6 Nov 94 8:49:37 GMT      ; Unknown [Elm 70.85]
 *     Mon,  7 Jan 2002 07:21:22 GMT  ; Unknown [Postfix]
 *     Sun, 06-Nov-1994 08:49:37 GMT  ; RFC 850 with four digit years
 *
 */

#define TIMEPARSE(ds,hr10,hr1,min10,min1,sec10,sec1)        \
    {                                                       \
        ds.tm_hour = ((hr10 - '0') * 10) + (hr1 - '0');     \
        ds.tm_min = ((min10 - '0') * 10) + (min1 - '0');    \
        ds.tm_sec = ((sec10 - '0') * 10) + (sec1 - '0');    \
    }
#define TIMEPARSE_STD(ds,timstr)                            \
    {                                                       \
        TIMEPARSE(ds, timstr[0],timstr[1],                  \
                      timstr[3],timstr[4],                  \
                      timstr[6],timstr[7]);                 \
    }

static Time subParseTime(sp_time_exp_t &ds, const char *monstr, const char *timstr, const char *gmtstr) {
	int mint, mon;

	if (ds.tm_mday <= 0 || ds.tm_mday > 31)
		return Time();

	if ((ds.tm_hour > 23) || (ds.tm_min > 59) || (ds.tm_sec > 61))
		return Time();

	mint = (monstr[0] << 16) | (monstr[1] << 8) | monstr[2];
	for (mon = 0; mon < 12; mon++)
		if (mint == s_months[mon])
			break;

	if (mon == 12)
		return Time();

	if ((ds.tm_mday == 31) && (mon == 3 || mon == 5 || mon == 8 || mon == 10))
		return Time();

    /* February gets special check for leapyear */

	if ((mon == 1)
			&& ((ds.tm_mday > 29)
					|| ((ds.tm_mday == 29)
							&& ((ds.tm_year & 3) || (((ds.tm_year % 100) == 0) && (((ds.tm_year % 400) != 100)))))
			))
		return Time();

	ds.tm_mon = mon;

	/* tm_gmtoff is the number of seconds off of GMT the time is.
	 *
	 * We only currently support: [+-]ZZZZ where Z is the offset in
	 * hours from GMT.
	 *
	 * If there is any confusion, tm_gmtoff will remain 0.
	 */
	ds.tm_gmtoff = 0;

	/* Do we have a timezone ? */
	if (gmtstr) {
		int offset;
		switch (*gmtstr) {
		case '-':
			offset = atoi(gmtstr + 1);
			ds.tm_gmtoff -= (offset / 100) * 60 * 60;
			ds.tm_gmtoff -= (offset % 100) * 60;
			break;
		case '+':
			offset = atoi(gmtstr + 1);
			ds.tm_gmtoff += (offset / 100) * 60 * 60;
			ds.tm_gmtoff += (offset % 100) * 60;
			break;
		}
	}

	/* apr_time_exp_get uses tm_usec field, but it hasn't been set yet.
	 * It should be safe to just zero out this value.
	 * tm_usec is the number of microseconds into the second.  HTTP only
	 * cares about second granularity.
	 */
	ds.tm_usec = 0;

	if (gmtstr) {
		return ds.gmt_get();
	} else {
		return ds.ltz_get();
	}
}

Time Time::fromRfc(const StringView &r) {
	auto date = r.data();
	sp_time_exp_t ds;
	const char *monstr, *timstr, *gmtstr;

	if (!date) {
		return Time();
	}

	 if (sp_date_checkmask(date, "@$$ @$$ ## ##:##:## *")) {
		auto extra_str = StringView(r.data() + 20, r.size() - 20);
		extra_str.skipUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		if (extra_str.is<StringView::CharGroup<CharGroupId::WhiteSpace>>()) {
			++ extra_str;
		}

		auto ydate = extra_str.data();
		ds.tm_year = ((ydate[0] - '0') * 10 + (ydate[1] - '0') - 19) * 100;

		if (ds.tm_year < 0) { return Time(); }

		ds.tm_year += ((ydate[2] - '0') * 10) + (ydate[3] - '0');
		ds.tm_mday = ((date[8] - '0') * 10) + (date[9] - '0');

		monstr = date + 4;
		timstr = date + 11;

		TIMEPARSE_STD(ds, timstr);

		gmtstr = nullptr;

		return subParseTime(ds, monstr, timstr, gmtstr);
	}

	/* Not all dates have text days at the beginning. */
	if (!chars::isdigit(date[0])) {
		while (*date && chars::isspace(*date)) /* Find first non-whitespace char */
			++date;

		if (*date == '\0')
			return Time();

		if ((date = strchr(date, ' ')) == NULL) /* Find space after weekday */
			return Time();

		++date; /* Now pointing to first char after space, which should be */
	}

	/* start of the actual date information for all 11 formats. */
	if (sp_date_checkmask(date, "## @$$ #### ##:##:## *")) { /* RFC 1123 format */
		ds.tm_year = ((date[7] - '0') * 10 + (date[8] - '0') - 19) * 100;

		if (ds.tm_year < 0)
			return Time();

		ds.tm_year += ((date[9] - '0') * 10) + (date[10] - '0');

		ds.tm_mday = ((date[0] - '0') * 10) + (date[1] - '0');

		monstr = date + 3;
		timstr = date + 12;
		gmtstr = date + 21;

		TIMEPARSE_STD(ds, timstr);
	} else if (sp_date_checkmask(date, "##-@$$-## ##:##:## *")) {/* RFC 850 format  */
		ds.tm_year = ((date[7] - '0') * 10) + (date[8] - '0');

		if (ds.tm_year < 70)
			ds.tm_year += 100;

		ds.tm_mday = ((date[0] - '0') * 10) + (date[1] - '0');

		monstr = date + 3;
		timstr = date + 10;
		gmtstr = date + 19;

		TIMEPARSE_STD(ds, timstr);
	} else if (sp_date_checkmask(date, "@$$ ~# ##:##:## ####*")) {
		/* asctime format */
		ds.tm_year = ((date[16] - '0') * 10 + (date[17] - '0') - 19) * 100;
		if (ds.tm_year < 0)
			return Time();

		ds.tm_year += ((date[18] - '0') * 10) + (date[19] - '0');

		if (date[4] == ' ')
			ds.tm_mday = 0;
		else
			ds.tm_mday = (date[4] - '0') * 10;

		ds.tm_mday += (date[5] - '0');

		monstr = date;
		timstr = date + 7;
		gmtstr = NULL;

		TIMEPARSE_STD(ds, timstr);
	} else if (sp_date_checkmask(date, "# @$$ #### ##:##:## *")) {
		/* RFC 1123 format*/
		ds.tm_year = ((date[6] - '0') * 10 + (date[7] - '0') - 19) * 100;

		if (ds.tm_year < 0)
			return Time();

		ds.tm_year += ((date[8] - '0') * 10) + (date[9] - '0');
		ds.tm_mday = (date[0] - '0');

		monstr = date + 2;
		timstr = date + 11;
		gmtstr = date + 20;

		TIMEPARSE_STD(ds, timstr);
	} else if (sp_date_checkmask(date, "## @$$ ## ##:##:## *")) {
		/* This is the old RFC 1123 date format - many many years ago, people
		 * used two-digit years.  Oh, how foolish.
		 *
		 * Two-digit day, two-digit year version. */
		ds.tm_year = ((date[7] - '0') * 10) + (date[8] - '0');

		if (ds.tm_year < 70)
			ds.tm_year += 100;

		ds.tm_mday = ((date[0] - '0') * 10) + (date[1] - '0');

		monstr = date + 3;
		timstr = date + 10;
		gmtstr = date + 19;

		TIMEPARSE_STD(ds, timstr);
	} else if (sp_date_checkmask(date, " # @$$ ## ##:##:## *")) {
		/* This is the old RFC 1123 date format - many many years ago, people
		 * used two-digit years.  Oh, how foolish.
		 *
		 * Space + one-digit day, two-digit year version.*/
		ds.tm_year = ((date[7] - '0') * 10) + (date[8] - '0');

		if (ds.tm_year < 70)
			ds.tm_year += 100;

		ds.tm_mday = (date[1] - '0');

		monstr = date + 3;
		timstr = date + 10;
		gmtstr = date + 19;

		TIMEPARSE_STD(ds, timstr);
	} else if (sp_date_checkmask(date, "# @$$ ## ##:##:## *")) {
		/* This is the old RFC 1123 date format - many many years ago, people
		 * used two-digit years.  Oh, how foolish.
		 *
		 * One-digit day, two-digit year version. */
		ds.tm_year = ((date[6] - '0') * 10) + (date[7] - '0');

		if (ds.tm_year < 70)
			ds.tm_year += 100;

		ds.tm_mday = (date[0] - '0');

		monstr = date + 2;
		timstr = date + 9;
		gmtstr = date + 18;

		TIMEPARSE_STD(ds, timstr);
	} else if (sp_date_checkmask(date, "## @$$ ## ##:## *")) {
		/* Loser format.  This is quite bogus.  */
		ds.tm_year = ((date[7] - '0') * 10) + (date[8] - '0');

		if (ds.tm_year < 70)
			ds.tm_year += 100;

		ds.tm_mday = ((date[0] - '0') * 10) + (date[1] - '0');

		monstr = date + 3;
		timstr = date + 10;
		gmtstr = NULL;

		TIMEPARSE(ds, timstr[0], timstr[1], timstr[3], timstr[4], '0', '0');
	} else if (sp_date_checkmask(date, "# @$$ ## ##:## *")) {
		/* Loser format.  This is quite bogus.  */
		ds.tm_year = ((date[6] - '0') * 10) + (date[7] - '0');

		if (ds.tm_year < 70)
			ds.tm_year += 100;

		ds.tm_mday = (date[0] - '0');

		monstr = date + 2;
		timstr = date + 9;
		gmtstr = NULL;

		TIMEPARSE(ds, timstr[0], timstr[1], timstr[3], timstr[4], '0', '0');
	} else if (sp_date_checkmask(date, "## @$$ ## #:##:## *")) {
		/* Loser format.  This is quite bogus.  */
		ds.tm_year = ((date[7] - '0') * 10) + (date[8] - '0');

		if (ds.tm_year < 70)
			ds.tm_year += 100;

		ds.tm_mday = ((date[0] - '0') * 10) + (date[1] - '0');

		monstr = date + 3;
		timstr = date + 9;
		gmtstr = date + 18;

		TIMEPARSE(ds, '0', timstr[1], timstr[3], timstr[4], timstr[6], timstr[7]);
	} else if (sp_date_checkmask(date, "# @$$ ## #:##:## *")) {
		/* Loser format.  This is quite bogus.  */
		ds.tm_year = ((date[6] - '0') * 10) + (date[7] - '0');

		if (ds.tm_year < 70)
			ds.tm_year += 100;

		ds.tm_mday = (date[0] - '0');

		monstr = date + 2;
		timstr = date + 8;
		gmtstr = date + 17;

		TIMEPARSE(ds, '0', timstr[1], timstr[3], timstr[4], timstr[6], timstr[7]);
	} else if (sp_date_checkmask(date, " # @$$ #### ##:##:## *")) {
		/* RFC 1123 format with a space instead of a leading zero. */
		ds.tm_year = ((date[7] - '0') * 10 + (date[8] - '0') - 19) * 100;

		if (ds.tm_year < 0)
			return Time();

		ds.tm_year += ((date[9] - '0') * 10) + (date[10] - '0');

		ds.tm_mday = (date[1] - '0');

		monstr = date + 3;
		timstr = date + 12;
		gmtstr = date + 21;

		TIMEPARSE_STD(ds, timstr);
	} else if (sp_date_checkmask(date, "##-@$$-#### ##:##:## *")) {
		/* RFC 1123 with dashes instead of spaces between date/month/year
		 * This also looks like RFC 850 with four digit years.
		 */
		ds.tm_year = ((date[7] - '0') * 10 + (date[8] - '0') - 19) * 100;
		if (ds.tm_year < 0)
			return Time();

		ds.tm_year += ((date[9] - '0') * 10) + (date[10] - '0');

		ds.tm_mday = ((date[0] - '0') * 10) + (date[1] - '0');

		monstr = date + 3;
		timstr = date + 12;
		gmtstr = date + 21;

		TIMEPARSE_STD(ds, timstr);
	} else {
		return Time();
	}

	return subParseTime(ds, monstr, timstr, gmtstr);
}

static const char sp_month_snames[12][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
static const char sp_day_snames[7][4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

void Time::encodeRfc822(char *date_str) {
	sp_time_exp_t xt(*this);
	const char *s;
	int real_year;

	/* example: "Sat, 08 Jan 2000 18:31:41 GMT" */
	/*           12345678901234567890123456789  */

	s = &sp_day_snames[xt.tm_wday][0];
	*date_str++ = *s++;
	*date_str++ = *s++;
	*date_str++ = *s++;
	*date_str++ = ',';
	*date_str++ = ' ';
	*date_str++ = xt.tm_mday / 10 + '0';
	*date_str++ = xt.tm_mday % 10 + '0';
	*date_str++ = ' ';
	s = &sp_month_snames[xt.tm_mon][0];
	*date_str++ = *s++;
	*date_str++ = *s++;
	*date_str++ = *s++;
	*date_str++ = ' ';
	real_year = 1900 + xt.tm_year;
	/* This routine isn't y10k ready. */
	*date_str++ = real_year / 1000 + '0';
	*date_str++ = real_year % 1000 / 100 + '0';
	*date_str++ = real_year % 100 / 10 + '0';
	*date_str++ = real_year % 10 + '0';
	*date_str++ = ' ';
	*date_str++ = xt.tm_hour / 10 + '0';
	*date_str++ = xt.tm_hour % 10 + '0';
	*date_str++ = ':';
	*date_str++ = xt.tm_min / 10 + '0';
	*date_str++ = xt.tm_min % 10 + '0';
	*date_str++ = ':';
	*date_str++ = xt.tm_sec / 10 + '0';
	*date_str++ = xt.tm_sec % 10 + '0';
	*date_str++ = ' ';
	*date_str++ = 'G';
	*date_str++ = 'M';
	*date_str++ = 'T';
	*date_str++ = 0;
}

void Time::encodeCTime(char *date_str) {
	sp_time_exp_t xt(*this, true);
	const char *s;
	int real_year;

	/* example: "Wed Jun 30 21:49:08 1993" */
	/*           123456789012345678901234  */

	s = &sp_day_snames[xt.tm_wday][0];
	*date_str++ = *s++;
	*date_str++ = *s++;
	*date_str++ = *s++;
	*date_str++ = ' ';
	s = &sp_month_snames[xt.tm_mon][0];
	*date_str++ = *s++;
	*date_str++ = *s++;
	*date_str++ = *s++;
	*date_str++ = ' ';
	*date_str++ = xt.tm_mday / 10 + '0';
	*date_str++ = xt.tm_mday % 10 + '0';
	*date_str++ = ' ';
	*date_str++ = xt.tm_hour / 10 + '0';
	*date_str++ = xt.tm_hour % 10 + '0';
	*date_str++ = ':';
	*date_str++ = xt.tm_min / 10 + '0';
	*date_str++ = xt.tm_min % 10 + '0';
	*date_str++ = ':';
	*date_str++ = xt.tm_sec / 10 + '0';
	*date_str++ = xt.tm_sec % 10 + '0';
	*date_str++ = ' ';
	real_year = 1900 + xt.tm_year;
	*date_str++ = real_year / 1000 + '0';
	*date_str++ = real_year % 1000 / 100 + '0';
	*date_str++ = real_year % 100 / 10 + '0';
	*date_str++ = real_year % 10 + '0';
	*date_str++ = 0;
}


NS_SP_END
