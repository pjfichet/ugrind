#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char rcsid[] USED = "$Id: version.c,v 0.10 2013/04/10 16:01:04 pj Exp pj $";
/* RCS ID LIST
* makefile,v 0.6 2013/09/09 20:16:04 pj Exp pj
* readme.tr,v 1.9 2013/09/08 15:43:13 pj Exp pj
* regexp.c,v 0.4 2013/03/14 10:44:00 pj Exp pj
* ugrind.c,v 0.8 2013/03/24 11:01:13 pj Exp pj
* ugrindefs.c,v 0.4 2013/03/24 11:03:25 pj Exp pj
* ugrindefs.src,v 0.3 2013/03/14 17:57:39 pj Exp pj
* ugrindroff.c,v 0.4 2013/04/09 16:55:53 pj Exp pj
* ugrind.tr,v 1.12 2013/09/08 15:43:13 pj Exp pj
* utroff-ugrind.tr,v 1.15 2013/09/08 15:43:13 pj Exp pj
* VERSION,v 0.10 2013/04/10 16:01:19 pj Exp pj
*/
