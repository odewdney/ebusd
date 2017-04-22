/*
 * ebusd - daemon for communication with eBUS heating systems.
 * Copyright (C) 2015-2017 John Baier <ebusd@ebusd.eu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lib/utils/clock.h"
#ifdef __MACH__
#  include <mach/clock.h>
#  include <mach/mach.h>
#endif
#ifdef _WIN32
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#endif

namespace ebusd {

#ifdef __MACH__
static bool clockInitialized = false;
static clock_serv_t clockServ;
#endif

#ifdef _WIN32
int asprintf(char **strp, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int r = vasprintf(strp, fmt, ap);
	va_end(ap);
	return r;
}

int vasprintf(char **ret, const char *format, va_list ap)
{
	// _vscprintf tells you how big the buffer needs to be
	int len = _vscprintf(format, ap);
	if (len == -1) {
		return -1;
	}
	size_t size = (size_t)len + 1;
	char *str = (char*)malloc(size);
	if (!str) {
		return -1;
	}
	// _vsprintf_s is the "secure" version of vsprintf
	int r = _vsnprintf_s(str, len + 1, _TRUNCATE, format, ap);
	if (r == -1) {
		free(str);
		return -1;
	}
	*ret = str;
	return r;
}

int     opterr = 1,             /* if error message should be printed */
optind = 1,             /* index into parent argv vector */
optopt,                 /* character checked for validity */
optreset;               /* reset getopt */
char    *optarg;                /* argument associated with option */

#define BADCH   (int)'?'
#define BADARG  (int)':'
#define EMSG    ""

/*
* getopt --
*      Parse argc/argv argument vector.
*/
int
getopt(int nargc, char * const nargv[], const char *ostr)
{
	static char *place = EMSG;              /* option letter processing */
	const char *oli;                        /* option letter list index */

	if (optreset || !*place) {              /* update scanning pointer */
		optreset = 0;
		if (optind >= nargc || *(place = nargv[optind]) != '-') {
			place = EMSG;
			return (-1);
		}
		if (place[1] && *++place == '-') {      /* found "--" */
			++optind;
			place = EMSG;
			return (-1);
		}
	}                                       /* option letter okay? */
	if ((optopt = (int)*place++) == (int)':' ||
		!(oli = strchr(ostr, optopt))) {
		/*
		* if the user didn't specify '-' as an option,
		* assume it means -1.
		*/
		if (optopt == (int)'-')
			return (-1);
		if (!*place)
			++optind;
		if (opterr && *ostr != ':')
			(void)printf("illegal option -- %c\n", optopt);
		return (BADCH);
	}
	if (*++oli != ':') {                    /* don't need argument */
		optarg = NULL;
		if (!*place)
			++optind;
	}
	else {                                  /* need an argument */
		if (*place)                     /* no white space */
			optarg = place;
		else if (nargc <= ++optind) {   /* no arg */
			place = EMSG;
			if (*ostr == ':')
				return (BADARG);
			if (opterr)
				(void)printf("option requires an argument -- %c\n", optopt);
			return (BADCH);
		}
		else                            /* white space */
			optarg = nargv[optind];
		place = EMSG;
		++optind;
	}
	return (optopt);                        /* dump back option letter */
}

char *suboptarg;

int getsubopt(char **optionp, char * const *tokens,char **valuep)
{
	register int cnt;
	register char *p;

	suboptarg = *valuep = NULL;

	if (!optionp || !*optionp)
		return(-1);

	/* skip leading white-space, commas */
	for (p = *optionp; *p && (*p == ',' || *p == ' ' || *p == '\t'); ++p);

	if (!*p) {
		*optionp = p;
		return(-1);
	}

	/* save the start of the token, and skip the rest of the token. */
	for (suboptarg = p;
		*++p && *p != ',' && *p != '=' && *p != ' ' && *p != '\t';);

	if (*p) {
		/*
		* If there's an equals sign, set the value pointer, and
		* skip over the value part of the token.  Terminate the
		* token.
		*/
		if (*p == '=') {
			*p = '\0';
			for (*valuep = ++p;
				*p && *p != ',' && *p != ' ' && *p != '\t'; ++p);
			if (*p)
				*p++ = '\0';
		}
		else
			*p++ = '\0';
		/* Skip any whitespace or commas after this token. */
		for (; *p && (*p == ',' || *p == ' ' || *p == '\t'); ++p);
	}

	/* set optionp for next round. */
	*optionp = p;

	for (cnt = 0; *tokens; ++tokens, ++cnt)
	if (!strcmp(suboptarg, *tokens))
		return(cnt);
	return(-1);
}


#else
void clockGettime(struct timespec* t) {
#ifdef __MACH__
  if (!clockInitialized) {
    clockInitialized = true;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &clockServ);
  }
  mach_timespec_t mts;
  clock_get_time(clockServ, &mts);
  t->tv_sec = mts.tv_sec;
  t->tv_nsec = mts.tv_nsec;
#else
  clock_gettime(CLOCK_REALTIME, t);
#endif
}

#endif
}  // namespace ebusd
