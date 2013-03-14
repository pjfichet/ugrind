/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*	from OpenSolaris "vfontedpr.c	1.17	93/06/03 SMI" 	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * From Heirloom vGrind "vfontedpr.c	1.4 (gritter) 10/22/05"
 */

/*
 * Portions Copyright (c) 2013 Pierre-Jean Fichet, Amiens, France
 *
 * $Id: grind.c,v 0.6 2013/03/14 17:41:13 pj Exp pj $
 */

#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#ifdef	EUC
#include <wchar.h>
#endif

#if defined (__GLIBC__) && defined (_IO_getc_unlocked)
#undef	getc
#define	getc(f)	_IO_getc_unlocked(f)
#endif

#define boolean int
#define TRUE 1
#define FALSE 0
#define NIL 0
#define STANDARD 0
#define ALTERNATE 1

/*
 * Vfontedpr.
 *
 * Dave Presotto 1/12/81 (adapted from an earlier version by Bill Joy)
 *
 */

#define STRLEN 10		/* length of strings introducing things */
#define PNAMELEN 40		/* length of a function/procedure name */
#define PSMAX 20		/* size of procedure name stacking */

/* regular expression routines */

/* regexp.c */
/*ptrmatch;			pointers to begin and end of a match */
typedef struct {
	int test;	// 1 if match succeeded
	char * beg; // begin of match
	char * end; // end of match 
} ptrmatch;
/*char	*expmatch();		match a string to an expression */
ptrmatch expmatch(register ptrmatch, register char *, register char *);
/*char	*STRNCMP();		a different kind of strncmp */
int	STRNCMP(register char *, register char *, register int);
/*char	*convexp();		convert expression to internal form */
char	*convexp(char *);

/*
 *	The state variables
 */

static boolean	incomm;		/* in a comment of the primary type */
static boolean	instr;		/* in a string constant */
static boolean	inchr;		/* in a string constant */
static boolean	nokeyw = FALSE;	/* no keywords being flagged */
static boolean	doindex = FALSE;/* form an index */
static boolean twocol = FALSE;	/* in two-column mode */
static boolean	filter = TRUE;	/* act as a filter (like eqn) */
static boolean	pass = FALSE;	/* when acting as a filter, pass indicates
				 * whether we are currently processing
				 * input.
				 */
static boolean	prccont;	/* continue last procedure */
static int	comtype;	/* type of comment */
static int	margin;
static int	psptr;		/* the stack index of the current procedure */
static char	pstack[PSMAX][PNAMELEN+1];	/* the procedure name stack */
static int	plstack[PSMAX];	/* the procedure nesting level stack */
static int	blklevel;	/* current nesting level */
static int	prclevel;	/* nesting level at which procedure definitions
				   may be found, -1 if none currently valid
				   (meaningful only if l_prclevel is true) */
static char	*defsfile = LIBDIR "/grindefs";	/* name of language definitions file */
static char	pname[BUFSIZ+1];

/*
 *	The language specific globals
 */

static char	*language = "c";/* the language indicator */
static char	*l_keywds[BUFSIZ/2];	/* keyword table address */
static char	*l_varbeg;	/* regular expr for variable begin */
static char *l_varend;	/* delimiter for variable end */
static char	*l_prcbeg;	/* regular expr for procedure begin */
static char	*l_combeg;	/* string introducing a comment */
static char	*l_comend;	/* string ending a comment */
static char	*l_acmbeg;	/* string introducing a comment */
static char	*l_acmend;	/* string ending a comment */
static char	*l_blkbeg;	/* string begining of a block */
static char	*l_blkend;	/* string ending a block */
static char	*l_strbeg;	/* delimiter for string constant */
static char	*l_strend;	/* delimiter for string constant */
static char	*l_chrbeg;	/* delimiter for character constant */
static char	*l_chrend;	/* delimiter for character constant */
static char	*l_prcenable;	/* re indicating that procedure definitions
				   can be found in the next lexical level --
				   kludge for lisp-like languages that use
				   something like
					   (defun (proc ...)
					  	(proc ...)
					   )
				   to define procedures */
static char	l_escape;	/* character used to  escape characters */
static boolean	l_toplex;	/* procedures only defined at top lex level */
static boolean l_prclevel;	/* procedure definitions valid only within
				   the nesting level indicated by the px
				   (l_prcenable) capability */

/*
 *  for the benefit of die-hards who aren't convinced that tabs
 *  occur every eight columns
 */
static short tabsize = 8;

static int	mb_cur_max;

/*
 *  global variables also used by expmatch
 */
boolean	escaped;		/* if last character was an escape */
char	*Start;			/* start of the current string */
boolean	l_onecase;		/* upper and lower case are equivalent */
char	*l_idchars;		/* characters legal in identifiers in addition
				   to letters and digits (default "_") */

extern int	STRNCMP(register char *, register char *, register int);
extern char	*convexp(char *);
extern ptrmatch expmatch(register ptrmatch, register char *, register char *);
extern int	tgetent(char *, char *, char *);
extern int	tgetnum(char *);
extern int	tgetflag(char *);
extern char	*tgetstr(char *, char **);

static int getlang(void);
static void	putScp(char *);
static char	*putKcp(char *, char *, int);
static int	tabs(char *, char *);
static int	width(register char *, register char *);
static char	*putcp(register int);
static int	isproc(char *);
static int	iskw(register char *);
static char	*fgetline(char **, size_t *, size_t *, FILE *);

/*
 * The code below emits troff macros and directives that consume part of the
 * troff macro and register space.  See tmac.vgrind for an enumeration of
 * these macros and registers.
 */

int
main(int argc, char **argv)
{
    FILE *in;
    char *fname;
    struct stat stbuf;
    char *buf = NULL;
    size_t size = 0;
    char idbuf[256];	/* enough for all 8 bit chars */
    char strings[2 * BUFSIZ];
    char defs[2 * BUFSIZ];
    int needbp = 0;
    int i;
    char *cp;

    setlocale(LC_CTYPE, "");
    mb_cur_max = MB_CUR_MAX;

    buf = malloc(size = BUFSIZ);

    /*
     * Dump the name by which we were invoked.
     */
    argc--, argv++;

    /*
     * Process arguments.  For the sake of compatibility with older versions
     * of the program, the syntax accepted below is very idiosyncratic.  Some
     * options require space between the option and its argument; others
     * disallow it.  No options may be bundled together.
     *
     * Actually, there is one incompatibility.  Files and options formerly
     * could be arbitrarily intermixed, but this is no longer allowed.  (This
     * possiblity was never documented.)
     */
    while (argc > 0 && *argv[0] == '-') {
	switch (*(cp = argv[0] + 1)) {

	case '\0':				/* - */
	    /* Take input from stdin. */
	    /* 
	     * This option implies the end of the flag arguments.  Leave the
	     * "-" in place for the file processing code to see.
	     */
	    goto flagsdone;

	case '2':				/* -2 */
	    /* Enter two column mode. */
	    twocol = 1;
	    printf("'nr =2 1\n");
	    break;

	case 'd':				/* -d <defs-file> */
	    /* Specify the language description file. */
	    defsfile = argv[1];
	    argc--, argv++;
	    break;

	case 'h':				/* -h [header] */
	    /* Specify header string. */
	    if (argc == 1) {
		printf("'ds =H\n");
		break;
	    }
	    printf("'ds =H %s\n", argv[1]);
	    argc--, argv++;
	    break;

	case 'l':				/* -l<language> */
	    /* Specify the language. */
	    language = cp + 1;
	    break;

	case 'n':				/* -n */
	    /* Indicate no keywords. */
	    nokeyw = 1;
	    break;

	case 's':				/* -s<size> */
	    /* Specify the font size. */
	    i = 0;
	    cp++;
	    while (*cp)
		i = i * 10 + (*cp++ - '0');
	    printf("'nr vP %d\n", i);
	    break;

	case 't':				/* -t */
	    /* Specify a nondefault tab size. */
	    tabsize = 4;
	    break;

	case 'x':				/* -x */
	    /* Build an index. */
	    doindex = 1;
	    /* This option implies "-n" as well; turn it on. */
	    argv[0] = "-n";
	    continue;
	}

	/* Advance to next argument. */
	argc--, argv++;
    }

flagsdone:
    /*
     * If not in filter mode, emit a call to the vS macro,
	 * so that the macro file can uniformly assume that all
     * program input is bracketed with vS-vE pairs.
     */
    if (!filter){
		printf("'vS\n");
	}

	/* If no more args, postpend "-" to get input from stdin */
	if (argc == 0 ) {
	    argv[0] = "-";
		argc = 1;
	}

    if (doindex) {
	/*
	 * XXX:	Hard-wired spacing information.  This should probably turn
	 *	into the emission of a macro invocation, so that tmac.vgrind
	 *	can make up its own mind about what spacing is appropriate.
	 */
	if (twocol)
	    printf("'ta 2.5i 2.75i 4.0iR\n'in .25i\n");
	else
	    printf("'ta 4i 4.25i 5.5iR\n'in .5i\n");
    }

    while (argc > 0) {
	if (strcmp(argv[0], "-") == 0) {
	    /* Embed an instance of the original stdin. */
	    in = fdopen(fileno(stdin), "r");
	    fname = "";
	} else {
	    /* Open the file for input. */
	    if ((in = fopen(argv[0], "r")) == NULL) {
		perror(argv[0]);
		exit(1);
	    }
	    fname = argv[0];
	}
	argc--, argv++;

	/*
	 * Reinitialize for the current file.
	 */
	incomm = FALSE;
	instr = FALSE;
	inchr = FALSE;
	escaped = FALSE;
	blklevel = 0;
	prclevel = -1;
	for (psptr=0; psptr<PSMAX; psptr++) {
	    pstack[psptr][0] = '\0';
	    plstack[psptr] = 0;
	}
	psptr = -1;
	printf("'-F\n");
	if (!filter) {
	    char *cp;

	    printf(".ds =F %s\n", fname);
	    if (needbp) {
		needbp = 0;
		printf(".()\n");
		printf(".bp\n");
	    }
	    fstat(fileno(in), &stbuf);
	    cp = ctime(&stbuf.st_mtime);
	    cp[16] = '\0';
	    cp[24] = '\0';
	    printf(".ds =M %s %s\n", cp+4, cp+20);
	    printf("'wh 0 vH\n");
	    printf("'wh -1i vF\n");
	}
	if (needbp && filter) {
	    needbp = 0;
	    printf(".()\n");
	    printf(".bp\n");
	}

	/*
	 *	MAIN LOOP!!!
	 */
	while (fgetline(&buf, &size, NULL, in) != NULL) {
	    if (buf[0] == '\f') {
		printf(".bp\n");
	    }
	    if (buf[0] == '.') {
		printf("%s", buf);
		if (!strncmp (buf+1, "vS", 2)){
		    pass = TRUE;
			/* get language */
			char *dp;
			char *lp;
			char lang[20];
			lp = lang;
			dp=buf+3;
			while ( *dp++ != ' ' && *dp != '\t')
				;
			while ( *dp != ' ' && *dp != '\t' && *dp != '\n')
					*lp++=*dp++;
			*lp='\0';
			language=lang;
			if (!getlang())
				pass = FALSE;
		}
		if (!strncmp (buf+1, "vE", 2)) {
		    pass = FALSE;
			language= "c";
		}
		continue;
	    }
	    prccont = FALSE;
	    if (!filter || pass)
		putScp(buf);
	    else
		printf("%s", buf);
	    if (prccont && (psptr >= 0))
		printf("'FC %s\n", pstack[psptr]);
#ifdef DEBUG
	    printf ("com %o str %o chr %o ptr %d\n", incomm, instr, inchr, psptr);
#endif
	    margin = 0;
	}

	needbp = 1;
	fclose(in);
    }

    /* Close off the vS-vE pair. */
    if (!filter)
	printf("'vE\n");

    exit(0);
    /* NOTREACHED */
}

static int
getlang(void)
{
    int i;
    char *cp;
    size_t size = 0;
    char defs[2 * BUFSIZ];
    char strings[2 * BUFSIZ];
    char *buf = NULL;
    char idbuf[256];	/* enough for all 8 bit chars */

    buf = malloc(size = BUFSIZ);

    /*
     * Get the language definition from the defs file.
     */
    i = tgetent (defs, language, defsfile);
    if (i == 0) {
	fprintf (stderr, "no entry for language %s in %s\n", language, defsfile);
	//exit (0);
	return 0;
    } else  if (i < 0) {
	fprintf (stderr,  "cannot find grindefs file %s\n", defsfile);
	//exit (0);
	return 0;
    }
    cp = strings;
    if (tgetstr ("kw", &cp) == NIL)
		nokeyw = TRUE;
    else  {
	char **cpp;

	cpp = l_keywds;
	cp = strings;
	while (*cp) {
	    while (*cp == ' ' || *cp =='\t')
		*cp++ = '\0';
	    if (*cp)
		*cpp++ = cp;
	    while (*cp != ' ' && *cp  != '\t' && *cp)
		cp++;
	}
	*cpp = NIL;
    }
	cp = buf;
	l_varbeg = convexp (tgetstr ("vb", &cp));
	cp = buf;
	l_varend = convexp (tgetstr ("ve", &cp));
    cp = buf;
    l_prcbeg = convexp (tgetstr ("pb", &cp));
    cp = buf;
    l_combeg = convexp (tgetstr ("cb", &cp));
    cp = buf;
    l_comend = convexp (tgetstr ("ce", &cp));
    cp = buf;
    l_acmbeg = convexp (tgetstr ("ab", &cp));
    cp = buf;
    l_acmend = convexp (tgetstr ("ae", &cp));
    cp = buf;
    l_strbeg = convexp (tgetstr ("sb", &cp));
    cp = buf;
    l_strend = convexp (tgetstr ("se", &cp));
    cp = buf;
    l_blkbeg = convexp (tgetstr ("bb", &cp));
    cp = buf;
    l_blkend = convexp (tgetstr ("be", &cp));
    cp = buf;
    l_chrbeg = convexp (tgetstr ("lb", &cp));
    cp = buf;
    l_chrend = convexp (tgetstr ("le", &cp));
    cp = buf;
    l_prcenable = convexp (tgetstr ("px", &cp));
    cp = idbuf;
    l_idchars = tgetstr ("id", &cp);
    /* Set default, for compatibility with old version */
    if (l_idchars == NIL)
	l_idchars = "_";
    l_escape = '\\';
    l_onecase = tgetflag ("oc");
    l_toplex = tgetflag ("tl");
    l_prclevel = tgetflag ("pl");

	return 1;
}



#define isidchr(c) (isalnum(c) || ((c) != NIL && strchr(l_idchars, (c)) != NIL))

static void
putScp(char *os)
{
    register char *s = os;		/* pointer to unmatched string */
    char dummy[BUFSIZ];			/* dummy to be used by expmatch */
	char vname[BUFSIZ+1];		/* variable name */
    ptrmatch comptr;			/* end of a comment delimiter */
    ptrmatch acmptr;			/* end of a comment delimiter */
    ptrmatch strptr;			/* end of a string delimiter */
    ptrmatch chrptr;			/* end of a character const delimiter */
    ptrmatch blksptr;			/* end of a lexical block start */
    ptrmatch blkeptr;			/* end of a lexical block end */
	ptrmatch prcptr;			/* end of a procedure delimiter */
	ptrmatch varptr;			/* end of a variable delimiter */
	ptrmatch vaeptr;			/* end of a variable delimiter */
	register ptrmatch z;		/* struct with unmatched string */
	char *nl;					/* char to print after putKcp (\n) */
	int i;

	z.test = NIL;
	z.beg = s;
	z.end = s;

    Start = os;			/* remember the start for expmatch */
    escaped = FALSE;
    if (nokeyw || incomm || instr)
	goto skip;
    if (isproc(s)) {
	printf("'FN %s\n", pname);
	if (psptr < PSMAX-1) {
	    ++psptr;
	    strncpy (pstack[psptr], pname, PNAMELEN);
	    pstack[psptr][PNAMELEN] = '\0';
	    plstack[psptr] = blklevel;
	}
    }
    /*
     * if l_prclevel is set, check to see whether this lexical level
     * is one immediately below which procedure definitions are allowed.
     */
    if (l_prclevel && !incomm && !instr && !inchr) {
	if ((expmatch (z, l_prcenable, dummy)).test != NIL)
	    prclevel = blklevel + 1;
    }
skip:
    do {

	if (!incomm && !inchr) {
	/* check for string, comment, blockstart, etc */
	    blkeptr = expmatch (z, l_blkend, dummy);
		blksptr = expmatch (z, l_blkbeg, dummy);
	    comptr = expmatch (z, l_combeg, dummy);
	    acmptr = expmatch (z, l_acmbeg, dummy);
	    strptr = expmatch (z, l_strbeg, dummy);
	    chrptr = expmatch (z, l_chrbeg, dummy);
		prcptr = expmatch (z, l_prcbeg, dummy);
		varptr = expmatch (z, l_varbeg, dummy);
/*
**		if (comptr.test != NIL)
**			printf ("\n-->com.beg:%s\n-->com.end:%s\n", comptr.beg, comptr.end);
**		if (acmptr.test != NIL)
**			printf ("\n-->acm.beg:%s\n-->acm.end:%s\n", acmptr.beg, acmptr.end);
**		if (strptr.test != NIL)
**			printf ("\n-->str.beg:%s\n-->str.end:%s\n", strptr.beg, strptr.end);
**		if (chrptr.test != NIL)
**			printf ("\n-->chr.beg:%s\n-->chr.end:%s\n", chrptr.beg, chrptr.end);
**		if (prcptr.test != NIL)
**			printf ("\n-->prc.beg:%s\n-->prc.end:%s\n", prcptr.beg, prcptr.end);
*/
		/* start of a variable? */
	    if (varptr.test != NIL && !inchr)
		if ( (varptr.end < prcptr.end || prcptr.test == NIL)
		  && (varptr.end < strptr.end || strptr.test == NIL)
		  && (varptr.end < acmptr.end || acmptr.end == NIL)
		  && (varptr.end < chrptr.end || chrptr.end == NIL)
		  && (varptr.end < blksptr.end || blksptr.end == NIL)
		  && (varptr.end < blkeptr.end || blkeptr.end == NIL)
		  && (varptr.end < comptr.end || comptr.end == NIL)
		  && (varptr.end < acmptr.end || acmptr.end == NIL)) {
			nl = putKcp (z.end, varptr.beg-1, FALSE);
			printf ("\\*(+V%s", nl);
			z.end = varptr.beg;
			nl = putKcp (z.end, varptr.end-1, FALSE);
			printf ("\\*(-V%s", nl);
			z.end = varptr.end;
		    continue;
		}
	}

	if (!incomm && !instr && !inchr) {

		/* start of a procedure? */
	    if (prcptr.test != NIL)
		if ((prcptr.end < strptr.end || strptr.test == NIL)
		  && (prcptr.end < acmptr.end || acmptr.test == NIL)
		  && (prcptr.end < chrptr.end || chrptr.test == NIL)
		  && (prcptr.end < blksptr.end || blksptr.test == NIL)
		  && (prcptr.end < blkeptr.end || blkeptr.test == NIL)
		  && (prcptr.end < comptr.end || comptr.test == NIL)
		  && (prcptr.end < acmptr.end || acmptr.test == NIL)) {
			/* assume procedure begin a line */
			nl = putKcp (z.end, prcptr.beg-1, FALSE);
			printf ("\\*(+K%s", nl);
			z.end = prcptr.beg;
			nl = putKcp (z.end, prcptr.end-1, FALSE);
			printf ("\\*(-K%s", nl);
		    z.end = prcptr.end;
		    continue;
		}

	    /* start of a comment? */
	    if (comptr.test != NIL)
		if ((comptr.end < strptr.end || strptr.test == NIL)
		  && (comptr.end < acmptr.end || acmptr.test == NIL)
		  && (comptr.end < chrptr.end || chrptr.test == NIL)
		  && (comptr.end < blksptr.end || blksptr.test == NIL)
		  && (comptr.end < blkeptr.end || blkeptr.test == NIL)) {
			nl = putKcp (z.end, comptr.beg-1, FALSE); 
			printf ("\\*(+C%s", nl);
			z.end = comptr.beg;
		    nl = putKcp (z.end, comptr.end-1, FALSE);
			printf ("%s", nl);
			z.end = comptr.end;
		    incomm = TRUE;
		    comtype = STANDARD;
		    continue;
		}

	    /* start of a comment? */
	    if (acmptr.test != NIL)
		if ((acmptr.end < strptr.end || strptr.test == NIL)
		  && (acmptr.end < chrptr.end || chrptr.test == NIL)
		  && (acmptr.end < blksptr.end || blksptr.test == NIL)
		  && (acmptr.end < blkeptr.end || blkeptr.test == NIL)) {
			nl = putKcp (z.end, acmptr.beg-1, FALSE);
			printf ("\\*(+C%s", nl);
			z.end = acmptr.beg;
		    nl = putKcp (z.end, acmptr.end-1, FALSE);
			printf ("%s", nl);
		    z.end = acmptr.end;
		    incomm = TRUE;
		    comtype = ALTERNATE;
		    continue;
		}

	    /* start of a string? */
	    if (strptr.test != NIL)
		if ((strptr.end < chrptr.end || chrptr.test == NIL)
		  && (strptr.end < blksptr.end || blksptr.test == NIL)
		  && (strptr.end < blkeptr.end || blkeptr.test == NIL)) {
			nl = putKcp (z.end, strptr.beg-1, FALSE); 
			printf ("\\*(+S%s", nl);
			z.end = strptr.beg;
		    nl = putKcp (z.end, strptr.end-1, FALSE); 
			printf ("%s", nl);
		    z.end = strptr.end;
		    instr = TRUE;
		    continue;
		}

	    /* start of a character string? */
	    if (chrptr.test != NIL)
		if ((chrptr.end < blksptr.end || blksptr.test == NIL)
		  && (chrptr.end < blkeptr.end || blkeptr.test == NIL)) {
			nl = putKcp (z.end, chrptr.beg-1, FALSE);
			printf ("\\*(+S%s", nl);
			z.end = chrptr.beg;
		    nl = putKcp (z.end, chrptr.end-1, FALSE);
			printf ("%s", nl);
		    z.end = chrptr.end;
		    inchr = TRUE;
		    continue;
		}

	    /* end of a lexical block */
	    if (blkeptr.test != NIL) {
		if (blkeptr.end < blksptr.end || blksptr.test == NIL) {
		    /* reset prclevel if necessary */
		    if (l_prclevel && prclevel == blklevel)
			prclevel = -1;
			nl = putKcp (z.end, blkeptr.beg-1, FALSE);
			printf ("\\*(+K%s", nl);
			z.end = blkeptr.beg;
		    nl = putKcp (z.end, blkeptr.end-1, FALSE);
			printf ("\\*(-K%s", nl);
		    z.end = blkeptr.end;
		    blklevel--;
		    if (psptr >= 0 && plstack[psptr] >= blklevel) {

			/* end of current procedure */
			printf ("\n'-F\n");
			blklevel = plstack[psptr];

			/* see if we should print the last proc name */
			if (--psptr >= 0)
			    prccont = TRUE;
			else
			    psptr = -1;
		    }
		    continue;
		}
	    }

	    /* start of a lexical block */
	    if (blksptr.test != NIL) {
		nl = putKcp (z.end, blksptr.beg-1, FALSE); 
		printf ("\\*(+K%s", nl);
		z.end = blksptr.beg;
		nl = putKcp (z.end, blksptr.end-1, FALSE);
		printf ("\\*(-K%s", nl);
		z.end = blksptr.end;
		blklevel++;
		continue;
	    }

	/* check for end of comment */
	} else if (incomm) {
	    comptr = expmatch (z, l_comend, dummy);
	    acmptr = expmatch (z, l_acmend, dummy);
	    if (((comtype == STANDARD) && (comptr.test != NIL)) ||
	        ((comtype == ALTERNATE) && (acmptr.test != NIL))) {
		if (comtype == STANDARD) {
		    nl = putKcp (z.end, comptr.end-1, TRUE);
			printf ("\\*(-C%s", nl);
		    z.end = comptr.end;
		} else {
			nl = putKcp (z.end, acmptr.end-1, TRUE); 
			printf ("\\*(-C%s", nl);
			z.end = acmptr.end;
		}
		incomm = FALSE;
		continue;
	    } else {
		nl = putKcp (z.end, z.end + strlen(z.end) -1, TRUE);
		printf ("%s", nl);
		z.end = z.end + strlen(z.end);
		continue;
	    }

	/* check for end of string */
	} else if (instr) {
		strptr = expmatch (z, l_strend, dummy);
	    if (strptr.test != NIL) {
		nl = putKcp (z.end, strptr.end-1, TRUE);
		printf ("\\*(-S%s", nl);
		z.end = strptr.end;
		instr = FALSE;
		continue;
	    } else {
		nl = putKcp (z.end, z.end+strlen(z.end)-1, TRUE);
		printf ("%s", nl);
		z.end = z.end + strlen(z.end);
		continue;
	    }

	/* check for end of character string */
	} else if (inchr) {
		chrptr = expmatch (z, l_chrend, dummy);
	    if (chrptr.test != NIL) {
		nl = putKcp (z.end, chrptr.end-1, TRUE);
		printf ("\\*(-S%s", nl);
		z.end = chrptr.end;
		inchr = FALSE;
		continue;
	    } else {
		nl = putKcp (z.end, z.end+strlen(z.end)-1, TRUE);
		printf ("%s", nl);
		z.end = z.end + strlen(z.end);
		continue;
	    }
	}

	/* print out the line */
	printf ("\\&"); // escape blank lines
	nl = putKcp (z.end, z.end + strlen(z.end) -1, FALSE);
	printf ("%s", nl);
	z.end = z.end + strlen(z.end);
    } while (*z.end);
}

static char *
putKcp (
    char	*start,		/* start of string to write */
    char	*end,		/* end of string to write */
    boolean	force		/* true if we should force nokeyw */
)
{
    int i;
	char *nl = "";		/* newline or not printable */
    int xfld = 0;

    while (start <= end) {
	if (doindex) {
	    if (*start == ' ' || *start == '\t') {
		if (xfld == 0)	
		    printf("");
		printf("\t");
		xfld = 1;
		while (*start == ' ' || *start == '\t')
		    start++;
		continue;
	    }
	}

	/* take care of nice tab stops */
	if (*start == '\t') {
	    while (*start == '\t')
		start++;
	    i = tabs(Start, start) - margin / tabsize;
	    printf ("\\h'|%dn'",
		    i * (tabsize == 4 ? 5 : 10) + 1 - margin % tabsize);
	    continue;
	}

	if (!nokeyw && !force)
	    if (  (*start == '#'   ||  isidchr(*start)) 
	       && (start == Start || !isidchr(start[-1]))
	       ) {
		i = iskw(start);
		if (i > 0) {
		    printf("\\*(+K");
		    do 
			nl = putcp(*start++);
		    while (--i > 0);
		    printf("\\*(-K");
		    continue;
		}
	    }
	nl = putcp (*start++);
    }
	return nl;
}


static int
tabs(char *s, char *os)
{

    return (width(s, os) / tabsize);
}

static int
width(register char *s, register char *os)
{
	register int i = 0;
	unsigned char c;
	int n;

	while (s < os) {
		if (*s == '\t') {
			i = (i + tabsize) &~ (tabsize-1);
			s++;
			continue;
		}
		c = *(unsigned char *)s;
		if (c < ' ')
			i += 2, s++;
#ifdef	EUC
		else if (c >= 0200) {
			wchar_t	wc;
			if ((n = mbtowc(&wc, s, mb_cur_max)) > 0) {
				s += n;
				if ((n = wcwidth(wc)) > 0)
					i += n;
			} else
				s++;
		}
#endif	/* EUC */
		else
			i++, s++;
	}
	return (i);
}

static char *
putcp(register int c)
{

	switch(c) {

	case 0:
		break;

	case '\f':
		break;

	case '{':
		printf("\\*(+K{\\*(-K");
		break;

	case '}':
		printf("\\*(+K}\\*(-K");
		break;

	case '\\':
		printf("\\e");
		break;

	case '_':
		printf("\\*_");
		break;

	case '-':
		printf("\\*-");
		break;

		/*
		 * The following two cases deal with the accent characters.
		 * If they're part of a comment, we assume that they're part
		 * of running text and hand them to troff as regular quote
		 * characters.  Otherwise, we assume they're being used as
		 * special characters (e.g., string delimiters) and arrange
		 * for troff to render them as accents.  This is an imperfect
		 * heuristic that produces slightly better appearance than the
		 * former behavior of unconditionally rendering the characters
		 * as accents.  (See bug 1040343.)
		 */

	case '`':
		if (incomm)
			printf("`");
		else
			printf("\\`");
		break;

	case '\'':
		if (incomm)
			printf("'");
		else
			printf("\\'");
		break;

	case '.':
		printf("\\&.");
		break;

		/*
		 * The following two cases contain special hacking
		 * to make C-style comments line up.  The tests aren't
		 * really adequate; they lead to grotesqueries such
		 * as italicized multiplication and division operators.
		 * However, the obvious test (!incomm) doesn't work,
		 * because incomm isn't set until after we've put out
		 * the comment-begin characters.  The real problem is
		 * that expmatch() doesn't give us enough information.
		 */

	case '*':
		if (instr || inchr)
			printf("*");
		else
			printf("\\f2*\\fP");
		break;

	case '/':
		if (instr || inchr)
			printf("/");
		else
			printf("\\f2\\h'\\w' 'u-\\w'/'u'/\\fP");
		break;

	default:
		if (c < 040)
			putchar('^'), c |= '@';
	case '\t':
		putchar(c);
		break;

	case '\n':
		return "\n";
	}
	return ""; // not printed char
}

/*
 *	look for a process beginning on this line
 */
static boolean
isproc(char *s)
{
    ptrmatch z;			/* structure to test */
	z.beg = s;
	z.end = s;
	z.test = NIL;
    pname[0] = '\0';
    if (l_prclevel ? (prclevel == blklevel) : (!l_toplex || blklevel == 0))
	if ((expmatch (z, l_prcbeg, pname)).test != NIL) {
	    return (TRUE);
	}
    return (FALSE);
}


/*
 * iskw - check to see if the next word is a keyword
 *	Return its length if it is or 0 if it isn't.
 */

static int
iskw(register char *s)
{
	register char **ss = l_keywds;
	register int i = 1;
	register char *cp = s;

	/* Get token length. */
	while (++cp, isidchr(*cp))
		i++;

	while (cp = *ss++) {
		if (!STRNCMP(s,cp,i) && !isidchr(cp[i]))
			return (i);
	}
	return (0);
}

#define	LSIZE	128

static char *
fgetline(char **line, size_t *linesize, size_t *llen, FILE *fp)
{
	int c;
	size_t n = 0;

	if (*line == NULL || *linesize < LSIZE + n + 1)
		*line = realloc(*line, *linesize = LSIZE + n + 1);
	for (;;) {
		if (n >= *linesize - LSIZE / 2)
			*line = realloc(*line, *linesize += LSIZE);
		c = getc(fp);
		if (c != EOF) {
			(*line)[n++] = c;
			(*line)[n] = '\0';
			if (c == '\n')
				break;
		} else {
			if (n > 0)
				break;
			else
				return NULL;
		}
	}
	if (llen)
		*llen = n;
	return *line;
}
