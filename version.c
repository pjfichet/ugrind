#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char rcsid[] USED = "$Id: version.c,v 0.7 2013/03/24 17:29:19 pj Exp $";
/* RCS ID LIST
* makefile,v 0.3 2013/03/24 17:27:49 pj Exp
* regexp.c,v 0.4 2013/03/14 10:44:00 pj Exp
* ugrind.c,v 0.8 2013/03/24 11:01:13 pj Exp
* ugrindefs.c,v 0.4 2013/03/24 11:03:25 pj Exp
* ugrindefs.src,v 0.3 2013/03/14 17:57:39 pj Exp
* ugrindroff.c,v 0.3 2013/04/04 19:34:54 pj Exp
* VERSION,v 0.7 2013/03/24 17:29:19 pj Exp
*/
