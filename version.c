#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char rcsid[] USED = "$Id: version.c,v 0.2 2013/03/13 17:56:29 pj Exp pj $";
/* SLIST
 * grind.c,v 0.4 2013/03/14 10:33:07 pj Exp pj $
 * grindefs.c,v 0.3 2013/03/13 18:10:05 pj Exp pj $
 * grindefs.src,v 0.2 2013/03/13 17:52:59 pj Exp pj $
 * regexp.c,v 0.4 2013/03/14 10:44:00 pj Exp pj $
*/