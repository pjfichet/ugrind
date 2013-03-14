/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*	from OpenSolaris "regexp.c	1.11	92/07/21 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * From Heirloom vGrind "regexp.c	1.3 (gritter) 10/22/05"
 */

/*
 * Portions Copyright (c) 2013 Pierre-Jean Fichet, Amiens, France
 *
 * $Id: regexp.c,v 0.3 2013/03/14 10:40:47 pj Exp pj $
 */
	  /* from UCB 5.1 (Berkeley) 6/5/85 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

typedef int	boolean;
#define TRUE	1
#define FALSE	0
#define NIL	0

extern boolean	l_onecase;	/* true if upper and lower equivalent */
extern char	*l_idchars;	/* set of characters legal in identifiers
				   in addition to letters and digits */

typedef struct {
	int test;	// TRUE if match
	char * beg; // begin of match
	char * end; // end of match 
} ptrmatch;


static void expconv(void);

#define isidchr(c)	\
		(isalnum(c) || ((c) != NIL && strchr(l_idchars, (c)) != NIL))
#define makelower(c)	(isupper((c)) ? tolower((c)) : (c))

/*  STRNCMP -	like strncmp except that we convert the
 *	 	first string to lower case before comparing
 *		if l_onecase is set.
 */

int
STRNCMP(register char *s1, register char *s2, register int len)
{
	if (l_onecase) {
	    do
		if (*s2 - makelower(*s1))
			return (*s2 - makelower(*s1));
		else {
			s2++;
			s1++;
		}
	    while (--len);
	} else {
	    do
		if (*s2 - *s1)
			return (*s2 - *s1);
		else {
			s2++;
			s1++;
		}
	    while (--len);
	}
	return(0);
}

/*	The following routine converts an irregular expression to
 *	internal format.
 *
 *	Either meta symbols (\a \d or \p) or character strings or
 *	operations ( alternation or parenthesizing ) can be
 *	specified.  Each starts with a descriptor byte.  The descriptor
 *	byte has STR set for strings, META set for meta symbols
 *	and OPER set for operations.
 *	The descriptor byte can also have the OPT bit set if the object
 *	defined is optional.  Also ALT can be set to indicate an alternation.
 *
 *	For metasymbols the byte following the descriptor byte identities
 *	the meta symbol (containing an ascii 'a', 'd', 'p', '|', or '(').  For
 *	strings the byte after the descriptor is a character count for
 *	the string:
 *
 *		meta symbols := descriptor
 *				symbol
 *
 *		strings :=	descriptor
 *				character count
 *				the string
 *
 *		operations :=	descriptor
 *				symbol
 *				character count
 */

/*
 *  handy macros for accessing parts of match blocks
 */
#define MSYM(A) (*(A+1))	/* symbol in a meta symbol block */
#define MNEXT(A) (A+2)		/* character following a metasymbol block */

#define OSYM(A) (*(A+1))	/* symbol in an operation block */
#define OCNT(A) (*(A+2))	/* character count */
#define ONEXT(A) (A+3)		/* next character after the operation */
#define OPTR(A) (A+*(A+2))	/* place pointed to by the operator */

#define SCNT(A) (*(A+1))	/* byte count of a string */
#define SSTR(A) (A+2)		/* address of the string */
#define SNEXT(A) (A+2+*(A+1))	/* character following the string */

/*
 *  bit flags in the descriptor 
 */
#define OPT 1
#define STR 2
#define META 4
#define ALT 8
#define OPER 16

static char *ure;	/* pointer current position in unconverted exp */
static char *ccre;	/* pointer to current position in converted exp*/

char *
convexp(
    char *re		/* unconverted irregular expression */
)
{
    register char *cre;		/* pointer to converted regular expression */

    /* allocate room for the converted expression */
    if (re == NIL)
	return (NIL);
    if (*re == '\0')
	return (NIL);
    cre = malloc (4 * strlen(re) + 3);
    ccre = cre;
    ure = re;

    /* start the conversion with a \a */
    *cre = META | OPT;
    MSYM(cre) = 'a';
    ccre = MNEXT(cre);

    /* start the conversion (its recursive) */
    expconv ();
    *ccre = 0;
    return (cre);
}

static void
expconv(void)
{
    register char *cs;		/* pointer to current symbol in converted exp */
    register char c;		/* character being processed */
    register char *acs;		/* pinter to last alternate */
    register int temp;

    /* let the conversion begin */
    acs = NIL;
    cs = NIL;
    while (*ure != NIL) {
	switch (c = *ure++) {

	case '\\':
	    switch (c = *ure++) {

	    /* escaped characters are just characters */
	    default:
		if (cs == NIL || (*cs & STR) == 0) {
		    cs = ccre;
		    *cs = STR;
		    SCNT(cs) = 1;
		    ccre += 2;
		} else 
		    SCNT(cs)++;
		*ccre++ = c;
		break;

	    /* normal(?) metacharacters */
	    case 'a':
	    case 'd':
	    case 'e':
	    case 'p':
		if (acs != NIL && acs != cs) {
		    do {
			temp = OCNT(acs);
			OCNT(acs) = ccre - acs; 
			acs -= temp;
		    } while (temp != 0);
		    acs = NIL;
		}
		cs = ccre;
		*cs = META;
		MSYM(cs) = c;
		ccre = MNEXT(cs);
		break;
	    }
	    break;
	    
	/* just put the symbol in */
	case '^':
	case '$':
	    if (acs != NIL && acs != cs) {
		do {
		    temp = OCNT(acs);
		    OCNT(acs) = ccre - acs;
		    acs -= temp;
		} while (temp != 0);
		acs = NIL;
	    }
	    cs = ccre;
	    *cs = META;
	    MSYM(cs) = c;
	    ccre = MNEXT(cs);
	    break;

	/* mark the last match sequence as optional */
	case '?':
	    if (cs)
	    	*cs = *cs | OPT;
	    break;

	/* recurse and define a subexpression */
	case '(':
	    if (acs != NIL && acs != cs) {
		do {
		    temp = OCNT(acs);
		    OCNT(acs) = ccre - acs;
		    acs -= temp;
		} while (temp != 0);
		acs = NIL;
	    }
	    cs = ccre;
	    *cs = OPER;
	    OSYM(cs) = '(';
	    ccre = ONEXT(cs);
	    expconv ();
	    OCNT(cs) = ccre - cs;		/* offset to next symbol */
	    break;

	/* return from a recursion */
	case ')':
	    if (acs != NIL) {
		do {
		    temp = OCNT(acs);
		    OCNT(acs) = ccre - acs;
		    acs -= temp;
		} while (temp != 0);
		acs = NIL;
	    }
	    cs = ccre;
	    *cs = META;
	    MSYM(cs) = c;
	    ccre = MNEXT(cs);
	    return;

	/* mark the last match sequence as having an alternate */
	/* the third byte will contain an offset to jump over the */
	/* alternate match in case the first did not fail */
	case '|':
	    if (acs != NIL && acs != cs)
		OCNT(ccre) = ccre - acs;	/* make a back pointer */
	    else
		OCNT(ccre) = 0;
	    *cs |= ALT;
	    cs = ccre;
	    *cs = OPER;
	    OSYM(cs) = '|';
	    ccre = ONEXT(cs);
	    acs = cs;	/* remember that the pointer is to be filles */
	    break;

	/* if its not a metasymbol just build a scharacter string */
	default:
	    if (cs == NIL || (*cs & STR) == 0) {
		cs = ccre;
		*cs = STR;
		SCNT(cs) = 1;
		ccre = SSTR(cs);
	    } else
		SCNT(cs)++;
	    *ccre++ = c;
	    break;
	}
    }
    if (acs != NIL) {
	do {
	    temp = OCNT(acs);
	    OCNT(acs) = ccre - acs;
	    acs -= temp;
	} while (temp != 0);
	acs = NIL;
    }
    return;
}
/* end of convertre */


/*
 *	The following routine recognises an irregular expresion
 *	with the following special characters:
 *
 *		\?	-	means last match was optional
 *		\a	-	matches any number of characters
 *		\d	-	matches any number of spaces and tabs
 *		\p	-	matches any number of alphanumeric
 *				characters. The
 *				characters matched will be copied into
 *				the area pointed to by 'name'.
 *		\|	-	alternation
 *		\( \)	-	grouping used mostly for alternation and
 *				optionality
 *
 *	The irregular expression must be translated to internal form
 *	prior to calling this routine
 *
 *	The value returned is the pointer to the first non \a 
 *	character matched.
 */

extern boolean escaped;		/* true if we are currently escaped */
extern char *Start;		/* start of string */

#include <stdio.h>

ptrmatch
expmatch (
	register ptrmatch s,	/* struct to check for a match in */
    register char *re,		/* a converted irregular expression */
    register char *mstring	/* where to put whatever matches a \p */
)
{
    register char *cs;		/* the current symbol */
	ptrmatch ptrnil, ptr, s1;	/* temporary struct of pointers */
    boolean matched;		/* a temporary boolean */
	char *p;
	int i=0;

    /* initial conditions */
	ptr.test = FALSE;
	ptrnil.test=NIL;
	ptrnil.beg=NIL;
	ptrnil.end=NIL;
    if (re == NIL)
	return (ptrnil);
    cs = re;
    matched = FALSE;

    /* loop till expression string is exhausted (or at least pretty tired) */
    while (*cs) {
	switch (*cs & (OPER | STR | META)) {

	/* try to match a string */
	case STR:
		matched = !STRNCMP (s.end, SSTR(cs), SCNT(cs));
	    if (matched) {

		/* hoorah it matches */
		s.end += SCNT(cs);
		cs = SNEXT(cs);
	    } else if (*cs & ALT) {

		/* alternation, skip to next expression */
		cs = SNEXT(cs);
	    } else if (*cs & OPT) {

		/* the match is optional */
		cs = SNEXT(cs);
		matched = 1;		/* indicate a successful match */
	    } else {

		/* no match, error return */
		return (ptrnil);
	    }
	    break;

	/* an operator, do something fancy */
	case OPER:
	    switch (OSYM(cs)) {

	    /* this is an alternation */
	    case '|':
		if (matched)

		    /* last thing in the alternation was a match, skip ahead */
		    cs = OPTR(cs);
		else

		    /* no match, keep trying */
		    cs = ONEXT(cs);
		break;

	    /* this is a grouping, recurse */
	    case '(':
		ptr = expmatch (s, ONEXT(cs), mstring);
		if (ptr.test != NIL) {

		    /* the subexpression matched */
		    matched = 1;
		    s = ptr;
		} else if (*cs & ALT) {

		    /* alternation, skip to next expression */
		    matched = 0;
		} else if (*cs & OPT) {

		    /* the match is optional */
		    matched = 1;	/* indicate a successful match */
		} else {

		    /* no match, error return */
		    return (ptrnil);
		}
		cs = OPTR(cs);
		break;
	    }
	    break;

	/* try to match a metasymbol */
	case META:
	    switch (MSYM(cs)) {

	    /* try to match anything and remember what was matched */
	    case 'p':
		/*
		 *  This is really the same as trying the match the
		 *  remaining parts of the expression to any subset
		 *  of the string.
		 */
		/*	Match the next expression, if that match, record
		 *  the string between this match and the previous one,
		 *  and test if the string only contain alphanumeric.
		 */
		s1 = s;
		do {
		    ptr = expmatch (s1, MNEXT(cs), mstring);
		    if (ptr.test != NIL && s1.end != s.end) {

			/* we have a match, remember the match */
			strncpy (mstring, s.end, s1.end - s.end);
			mstring[s1.end - s.end] = '\0';
			return (ptr);
		    } else if (ptr.test != NIL && (*cs & OPT)) {

			/* it was aoptional so no match is ok */
			return (ptr);
		    } else if (ptr.test != NIL) {

			/* not optional and we still matched */

				/* in case MNEXT was null, record chars: */
				i=0;
				while ( isidchr(s1.end[i]) ) {
					mstring[i]=s1.end[i];
					*s.end++;
					i++;
				}
				mstring[i]='\0';
				if (i > 0){
					s.test = 1;
					return s;
				} else
				return (ptrnil);
			}
			
		    if (!isidchr(*s1.end))
			return (ptrnil);
		    if (*s1.end == '\\')
			escaped = escaped ? FALSE : TRUE;
		    else
			escaped = FALSE;
		/* increment while not match: */
		} while (*s1.end++);
		return (ptrnil);

	    /* try to match anything */
	    case 'a':
		/*
		 *  This is really the same as trying the match the
		 *  remaining parts of the expression to any subset
		 *  of the string.
		 */
		s1 = s;
		do {
			s1.beg = s1.end;

		    ptr = expmatch (s1, MNEXT(cs), mstring);
		    if (ptr.test != NIL && s1.end != s.end) {

			/* we have a match */
			return (ptr);
		    } else if (ptr.test != NIL && (*cs & OPT)) {

			/* it was aoptional so no match is ok */
			return (ptr);
		    } else if (ptr.test != NIL) {

			/* not optional and we still matched */
			return (ptrnil);
		    }
		    if (*s1.end == '\\')
			escaped = escaped ? FALSE : TRUE;
		    else
			escaped = FALSE;
		} while (*s1.end++);
		return (ptrnil);

	    /* fail if we are currently escaped */
	    case 'e':
		if (escaped)
		    return(ptrnil);
		cs = MNEXT(cs); 
		break;

	    /* match any number of tabs and spaces */
	    case 'd':
		ptr = s;
		while (*s.end == ' ' || *s.end == '\t')
		    s.end++;
		if (s.end != ptr.end || s.end == Start) {

		    /* match, be happy */
		    matched = 1;
		    cs = MNEXT(cs); 
		} else if (*s.end == '\n' || *s.end == '\0') {

		    /* match, be happy */
		    matched = 1;
		    cs = MNEXT(cs); 
		} else if (*cs & ALT) {

		    /* try the next part */
		    matched = 0;
		    cs = MNEXT(cs);
		} else if (*cs & OPT) {

		    /* doesn't matter */
		    matched = 1;
		    cs = MNEXT(cs);
		} else

		    /* no match, error return */
		    return (ptrnil);
		break;

	    /* check for end of line */
	    case '$':
		if (*s.end == '\0' || *s.end == '\n') {

		    /* match, be happy */
		    s.end++;
		    matched = 1;
		    cs = MNEXT(cs);
		} else if (*cs & ALT) {

		    /* try the next part */
		    matched = 0;
		    cs = MNEXT(cs);
		} else if (*cs & OPT) {

		    /* doesn't matter */
		    matched = 1;
		    cs = MNEXT(cs);
		} else

		    /* no match, error return */
		    return (ptrnil);
		break;

	    /* check for start of line */
	    case '^':
		if (s.end == Start) {

		    /* match, be happy */
		    matched = 1;
		    cs = MNEXT(cs);
		} else if (*cs & ALT) {

		    /* try the next part */
		    matched = 0;
		    cs = MNEXT(cs);
		} else if (*cs & OPT) {

		    /* doesn't matter */
		    matched = 1;
		    cs = MNEXT(cs);
		} else

		    /* no match, error return */
		    return (ptrnil);
		break;

	    /* end of a subexpression, return success */
	    case ')':
		s.test = 1;
		return (s);
	    }
	    break;
	}
    }
	s.test = 1;
    return (s);
}
