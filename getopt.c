/***************************************************************************\
**                                                                         **
**   Function name: getopt()                                               **
**   Author:        Henry Spencer, UofT                                    **
**   Coding date:   84/04/28                                               **
**                                                                         **
**   Description:                                                          **
**                                                                         **
**   Parses argv[] for arguments.                                          **
**   Works with Whitesmith's C compiler.                                   **
**                                                                         **
**   Inputs   - The number of arguments                                    **
**            - The base address of the array of arguments                 **
**            - A string listing the valid options (':' indicates an       **
**              argument to the preceding option is required, a ';'        **
**              indicates an argument to the preceding option is optional) **
**                                                                         **
**   Outputs  - Returns the next option character,                         **
**              '?' for non '-' arguments                                  **
**              or ':' when there is no more arguments.                    **
**                                                                         **
**   Side Effects + The argument to an option is pointed to by 'optarg'    **
**                                                                         **
*****************************************************************************
**                                                                         **
**   REVISION HISTORY:                                                     **
**                                                                         **
**     DATE           NAME                        DESCRIPTION              **
**   YY/MM/DD  ------------------   ------------------------------------   **
**   88/10/20  Janick Bergeron      Returns '?' on unamed arguments        **
**                                  returns '!' on unknown options         **
**                                  and 'EOF' only when exhausted.         **
**   88/11/18  Janick Bergeron      Return ':' when no more arguments      **
**   89/08/11  Janick Bergeron      Optional optarg when ';' in optstring  **
**   99/05/22  Peter Verhas         optarg is argument to be thread safe   **
**   99/08/28  Peter Verhas         optind becomes argument                **
**   02/11/28  Nigel Hathaway       MacOS variation                        **
**                                                                         **
\***************************************************************************/
#include <stdio.h>
#ifdef VMS
#define index  strchr
#endif
#ifdef WIN32
#define index  strchr
#endif
#ifdef __MACOS__
#define index  strchr
#endif

/*FUNCTION*/
char getoptt(int argc, 
            char **argv,
            char *optstring,
            char **optarg,
            int *poptind
  ){
#define optind (*poptind)
	register int c;
	register char *place;
	extern char *index();
	static char *scan = NULL;

	*optarg = NULL;

	if (scan == NULL || *scan == '\0') {

		if (optind == 0)
			optind++;
		if (optind >= argc)
			return ':';

		*optarg = place = argv[optind++];
		if (place[0] != '-' || place[1] == '\0')
			return '?';
		if (place[1] == '-' && place[2] == '\0')
			return '?';
		scan = place + 1;
	}

	c = *scan++;
	place = index(optstring, c);
	if (place == NULL || c == ':' || c == ';') {
		scan = NULL;
		return '!';
	}
	if (*++place == ':') {

		if (*scan != '\0') {

			*optarg = scan;
			scan = NULL;

		}
		else {

			if (optind >= argc) {
				return '!';
			}
			*optarg = argv[optind];
			optind++;
		}
	}
	else if (*place == ';') {

		if (*scan != '\0') {

			*optarg = scan;
			scan = NULL;

		}
		else {

			if (optind >= argc || *argv[optind] == '-')
				*optarg = NULL;
			else {
				*optarg = argv[optind];
				optind++;
			}
		}
	}
	return c;
}
