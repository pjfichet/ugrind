#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char rcsid[] USED = "$Id: version.c,v 0.11 2013/10/30 15:07:51 pj Exp pj $";
/* RCS ID LIST
* VERSION,v 0.11 2013/10/30 15:07:56 pj Exp pj
*/
