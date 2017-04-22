#ifndef __GETOPT_H__
/**
* DISCLAIMER
* This file is part of the mingw-w64 runtime package.
*
* The mingw-w64 runtime package and its code is distributed in the hope that it
* will be useful but WITHOUT ANY WARRANTY.  ALL WARRANTIES, EXPRESSED OR
* IMPLIED ARE HEREBY DISCLAIMED.  This includes but is not limited to
* warranties of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
/*
* Copyright (c) 2002 Todd C. Miller <Todd.Miller@courtesan.com>
*
* Permission to use, copy, modify, and distribute this software for any
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
*
* Sponsored in part by the Defense Advanced Research Projects
* Agency (DARPA) and Air Force Research Laboratory, Air Force
* Materiel Command, USAF, under agreement number F39502-99-1-0512.
*/
/*-
* Copyright (c) 2000 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Dieter Baron and Thomas Klausner.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

//#pragma warning(disable:4996)

#define __GETOPT_H__

/* All the headers include this file. */
#include <crtdefs.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	REPLACE_GETOPT		/* use this getopt as the system getopt(3) */

#ifdef REPLACE_GETOPT
//	int	opterr = 1;		/* if error message should be printed */
//	int	optind = 1;		/* index into parent argv vector */
//	int	optopt = '?';		/* character checked for validity */
//#undef	optreset		/* see getopt.h */
//#define	optreset		__mingw_optreset
//	int	optreset;		/* reset getopt */
//	char    *optarg;		/* argument associated with option */
#endif

	//extern int optind;		/* index of first non-option in argv      */
	//extern int optopt;		/* single option character, as parsed     */
	//extern int opterr;		/* flag to enable built-in diagnostics... */
	//				/* (user may set to zero, to suppress)    */
	//
	//extern char *optarg;		/* pointer to argument of current option  */

#define PRINT_ERROR	((data->opterr) && (*options != ':'))

#define FLAG_PERMUTE	0x01	/* permute non-options to the end of argv */
#define FLAG_ALLARGS	0x02	/* treat non-options as args to option "-1" */
#define FLAG_LONGONLY	0x04	/* operate as getopt_long_only */

	/* return values */
#define	BADCH		(int)'?'
#define	BADARG		((*options == ':') ? (int)':' : (int)'?')
#define	INORDER 	(int)1

#ifndef __CYGWIN__
#define __progname __argv[0]
#else
	extern char __declspec(dllimport) *__progname;
#endif

	char * __argp_short_program_name();

#ifdef __CYGWIN__
	static char EMSG[] = "";
#else
#define	EMSG		""
#endif


#ifdef REPLACE_GETOPT
	/*
	* getopt --
	*	Parse argc/argv argument vector.
	*
	* [eventually this will replace the BSD getopt]
	*/
	extern int getopt(int nargc, char * const *nargv, const char *options);

	struct _getopt_data
	{
		int opterr;
		int optind;
		int optopt;
		int optreset;
		char *optarg;
	};

	extern int getopt_r(int nargc, char * const *nargv, const char *options, struct _getopt_data *r);

#define _GETOPT_DATA_INITIALIZER {1,1,'?'}

#endif /* REPLACE_GETOPT */

	//extern int getopt(int nargc, char * const *nargv, const char *options);

#ifdef _BSD_SOURCE
	/*
	* BSD adds the non-standard `optreset' feature, for reinitialisation
	* of `getopt' parsing.  We support this feature, for applications which
	* proclaim their BSD heritage, before including this header; however,
	* to maintain portability, developers are advised to avoid it.
	*/
//# define optreset  __mingw_optreset
	//extern int optreset;
#endif
#ifdef __cplusplus
}
#endif
/*
* POSIX requires the `getopt' API to be specified in `unistd.h';
* thus, `unistd.h' includes this header.  However, we do not want
* to expose the `getopt_long' or `getopt_long_only' APIs, when
* included in this manner.  Thus, close the standard __GETOPT_H__
* declarations block, and open an additional __GETOPT_LONG_H__
* specific block, only when *not* __UNISTD_H_SOURCED__, in which
* to declare the extended API.
*/
#endif /* !defined(__GETOPT_H__) */

#if !defined(__UNISTD_H_SOURCED__) && !defined(__GETOPT_LONG_H__)
#define __GETOPT_LONG_H__

#ifdef __cplusplus
extern "C" {
#endif

	struct option		/* specification for a long form option...	*/
	{
		const char *name;		/* option name, without leading hyphens */
		int         has_arg;		/* does it take an argument?		*/
		int        *flag;		/* where to save its status, or NULL	*/
		int         val;		/* its associated status value		*/
	};

	enum    		/* permitted values for its `has_arg' field...	*/
	{
		no_argument = 0,      	/* option never takes an argument	*/
		required_argument,		/* option always requires an argument	*/
		optional_argument		/* option may take an argument		*/
	};

	extern int getopt_long(int nargc, char * const *nargv, const char *options,
	    const struct option *long_options, int *idx);
	extern int getopt_long_only(int nargc, char * const *nargv, const char *options,
	    const struct option *long_options, int *idx);
	extern int _getopt_long_r(int nargc, char * const *nargv, const char *options,
		const struct option *long_options, int *idx, struct _getopt_data *r);
	extern int _getopt_long_only_r(int nargc, char * const *nargv, const char *options,
		const struct option *long_options, int *idx, struct _getopt_data *r);

	//extern int getopt_long(int nargc, char * const *nargv, const char *options,
	//    const struct option *long_options, int *idx);
	//extern int getopt_long_only(int nargc, char * const *nargv, const char *options,
	//    const struct option *long_options, int *idx);
	/*
	* Previous MinGW implementation had...
	*/
#ifndef HAVE_DECL_GETOPT
	/*
	* ...for the long form API only; keep this for compatibility.
	*/
# define HAVE_DECL_GETOPT	1
#endif

#ifdef __cplusplus
}
#endif


#endif /* !defined(__UNISTD_H_SOURCED__) && !defined(__GETOPT_LONG_H__) */