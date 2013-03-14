#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char rcsid[] USED = "$Id: version.c,v 0.4 2013/03/14 18:02:21 pj Exp pj $";
/* SLIST
 * grind.c,v 0.7 2013/03/14 18:19:48 pj Exp pj $
 * grindefs.c,v 0.3 2013/03/13 18:10:05 pj Exp pj $
 * grindefs.src,v 0.3 2013/03/14 17:57:39 pj Exp pj $
 * grindroff.c,v 0.1 2013/03/14 18:27:04 pj Exp pj $
 * regexp.c,v 0.4 2013/03/14 10:44:00 pj Exp pj $
*/
