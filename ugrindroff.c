/*
 * Copyright (c) 2012
 * Pierre-Jean Fichet. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 
 *   1.  Redistributions of source code must retain the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer.
 *   2.  Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials
 *     provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: ugrindroff.c,v 0.3 2013/04/04 19:34:54 pj Exp pj $
 */

/*
 * ugrindroff.c
 * The following code insert color definition
 * in troff code between vS and vE macros.
 * It is hardcoded, instead of using ugrindefs,
 * because it's simpler than hacking ugrindefs.c.
 */

#include <stdio.h>

/* trprinter:
** print a char,
** escape it if needed
*/
void
trprinter(char c)
{
	switch (c) {
		case '\\':
			printf ("\\*e\\&"); // \\e
			break;
		case ('.'):
			printf("\\&.");
			break;
		case ('\''):
			printf("\\&'");
			break;
		case '_':
			printf("\\*_");
			break;
		case '-':
			printf("\\*-");
			break;
		default:
			putchar(c);
			break;
	}

}




/* escape:
** "\" has been found
*/
int
trescape(FILE *fp)
{
  char c;
  char d;
  int nl=0;

  c=getc(fp);

  // newline
  if (c=='\n'){
	printf("\\*(+C\\*e\\*(-C\n");
	nl=1;
  }

  // double slash
  else if (c=='\\'){
    printf("\\*(+C\\*e\\*(-C");
    trescape(fp); 
  }

  // two letter character name
  else if (c=='('){
	printf("\\*(+V\\*e%c\\*(-V\\*(+S", c);
	c=getc(fp);
	trprinter(c);
	c=getc(fp);
	trprinter(c);
	printf("\\*(-S");
  }

  // long character name
  else if (c=='['){
	printf("\\*(+V\\*e%c\\*(-V\\*(+S", c);
	while( (c=getc(fp)) != ']')
		trprinter(c);
	printf("\\*(-S\\*(+V]\\*(-V");
  }

  // point size change function
  else if (c=='s'){
	printf("\\*(+V\\*e");
	putchar(c);
	c=getc(fp);
	if (c=='-' || c=='+'){
		putchar(c);
		c=getc(fp);
	}
	if (c=='[' || c=='\'' || c=='"' ){
		d=c;
		putchar(c);
		printf("\\*(-V\\*(+S");
		while( (c=getc(fp)) != d)
			trprinter(c);
		printf("\\*(-S\\*(+V%s\\*(-V", d);
	}
	else {
		putchar(c);
	    printf("\\*(-V");
  	}
  }

  // bracketed escape sequence:
  // string, number register, font, variable...
  else if (c=='*' || c=='$' || c=='f' || c=='g' || c=='k' \
	|| c=='n' || c=='P' || c=='V' || c=='Y'){
    printf("\\*(+V\\*e");
    putchar(c);
    c=getc(fp);
    if (c=='-' || c=='+'){
      putchar(c);
      c=getc(fp);
    }
    if (c=='('){
      printf("(\\*(-V\\*(+S");
        c=getc(fp);
        putchar(c);
        c=getc(fp);
        putchar(c);
      printf("\\*(-S");
    }
    else if (c=='['){
      printf("[\\*(-V\\*(+S");
      while( (c=getc(fp)) != ']')
        trprinter(c);
      printf("\\*(-S\\*(+V]\\*(-V", c);
    }
  else{
    putchar(c);
    printf("\\*(-V");
  }
  }

  // comment
  else if (c=='"'){
    printf("\\*(+C\\*e");
    putchar(c);
    while ( (c=getc(fp)) != '\n')
      trprinter(c);
    printf("\\*(-C");
    putchar(c);
    nl=1;
  }

  // quoted escaped sequence
  else if ( c=='A' || c=='b' || c=='B' || c=='C' || c=='D'
	|| c=='h' ||c=='H' || c=='j' || c=='J' || c=='l'
	|| c=='L' || c=='N' || c=='o' || c=='R' || c=='S' || c=='T'
	|| c=='U' || c=='v' || c=='w' ||c=='W' || c=='x' || c=='X'
	|| c=='Z' ){
    printf("\\*(+V\\*e");
	putchar(c);
    c=getc(fp);
    if (c=='\''){
      printf("'\\*(-V\\*(+S");
      while ( (c=getc(fp)) !='\'')
        trprinter(c);
      printf("\\*(-S\\*(+V'\\*(-V");
    }
    else if (c=='"'){
      printf("\"\\*(-V\\*(+S");
      while ( (c=getc(fp)) !='"')
       	trprinter(c);
      printf("\\*(-S\\*(+V\"\\*(-V");
    }
  }

  // everything else
  else
	printf("\\*(+V\\*e\\&%c\\*(-V", c);

return nl;
}


/* macro:
** "\n." has been found
** insert color definition
*/
int
trmacro(FILE *fp, char dot)
{
  char c;
  char *p;
  char name[100];
  int nl=0;
  char space[100];

  // get space before name
  p=space;
  while ( (c=getc(fp)) == ' ' || c =='\t')
    *p++=c;
  *p++='\0';

  // get macro name
  p=name;
  *p++=c;
  // macro is an escaped string, use escape
  if( c=='\\'){
    printf("\\*(+K\\&%c\\*(-K%s", dot, space);	
    nl=trescape(fp);
  }
  else {
    while ( (c=getc(fp)) != ' ' && c !='\t' && c !='\n')
      *p++=c;
    *p++='\0';
  
    // macro is vE
    if (name[0]=='v' && name[1]=='E'){
      //printf(".%s%s%c", space, name, c);
      if (c!='\n'){
        while ( (c=getc(fp)) != '\n')
          putchar(c);
      }
      nl=2;
    }
    else {
      // print macro name
      printf("\\*(+K\\&%c\\*(-K%s", dot, space);
      printf("\\*(+K%s\\*(-K", name);
      putchar(c);
      if(c=='\n');
        nl=1;
  
      // macro is if or ie
      if ( strcmp(name, "if")==0 || strcmp(name, "ie")==0 ){
        // print space
        while ( (c=getc(fp))== ' ' || c=='\t')
          putchar(c);
        // print test
        printf("\\*(+V");
        trprinter(c);
        while ( (c=getc(fp))!='\t' && c != ' ')
          trprinter(c);
        printf("\\*(-V");
        putchar(c);
      }
  
      // macro is ds, nr, lds, lnr
      else if (strcmp(name, "ds")==0 || strcmp(name, "lds")==0 \
      || strcmp(name, "nr")==0 || strcmp(name, "lnr")==0){
        while ( (c=getc(fp))== ' ' || c=='\t')
          putchar(c);
        // name of string 
        printf("\\*(+V");
        trprinter(c);
        while ( (c=getc(fp))!=' ' && c!='\t' && c!='\n')
          trprinter(c);
        printf("\\*(-V");
        // space
        putchar(c);
        while ( (c=getc(fp))==' ' || c=='\t' )
          putchar(c);
        // content of string
        printf("\\*(+S");
        trprinter(c);
        while ( (c=getc(fp))!='\n')
          trprinter(c); 
        printf("\\*(-S");
        putchar(c);
        nl=1;
      }
      // macro is de
      else if (strcmp(name, "de")==0) {
        while ( (c=getc(fp))== ' ' || c=='\t')
          putchar(c);
        // name of macro
        printf("\\*(+K");
        trprinter(c);
        while ( (c=getc(fp))!=' ' && c!='\t' && c!='\n')
          trprinter(c);
        printf("\\*(-K");
        putchar(c);
        if (c=='\n')
          nl=1;
      }

      // macro is tl
      else if (strcmp(name, "tl")==0)
		nl=0;

    } // macro is not vE
  } // macro is not an escaped string


return nl;
}
    

/*
** trtroff:
** ".vS [t|n]roff" has been found
** insert color definition
** and escape dots.
*/
int
trtroff(FILE *fp)
{
  char c;
  int nl=1; // 1 after "\n"

  // printf( ".vS troff\n");

  while( (c=getc(fp)) != EOF){
    if ( c=='\n'){
      putchar(c);
      nl=1;
    }
    // macro
    else if ( nl==1 && (c=='.' || c=='\'') ) {
      	nl=trmacro(fp, c);
      	if (nl==2)
        	break;
    }
    // escape
    else if ( c=='\\')
      nl=trescape(fp);
    else{
      putchar(c);
      nl=0;
    }
  }
return 0;
}


