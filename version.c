#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char rcsid[] USED = "$Id$";
/* SLIST
 * grind.c,v 0.2 2013/03/13 17:52:24 pj Exp pj $
 * grindefs.c,v 0.2 2013/03/13 17:52:53 pj Exp pj $
 * grindefs.src,v 0.2 2013/03/13 17:52:59 pj Exp pj $
 * regexp.c,v 0.2 2013/03/13 17:53:05 pj Exp pj $
*/
