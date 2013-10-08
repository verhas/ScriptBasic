/*
FILE: lsp.c
HEADER: lsp.h

For documentation of this module read the embedded documentation.

TO_HEADER:

typedef struct NODE
{
   unsigned char ntype;
   union
   {
      struct
      {
         struct NODE *_car,*_cdr;
      } n_cons;
      double fvalue;
      long ivalue;
      char *svalue;

   }
   n_value;
} *LVAL;

#define NTYPE_CON 1
#define NTYPE_FLO 2
#define NTYPE_INT 3
#define NTYPE_STR 4
#define NTYPE_SYM 5
#define NTYPE_CHR 6
#define NTYPE_FRE 7


#define null(x)       ((x) == NIL)
#define freep(x)      ((x)->ntype == NTYPE_FRE)
#define numberp(x)    (floatp(x)||integerp(x))
#define endp(x) null(x)
#define eq(x,y)   ((x)==(y))

#define evenp(x)  (!oddp(x))
#define oddp(x)   (getint(x)&1)
#define minusp(x) (floatp(x) ? getfloat(x) < 0.0 : getint(x) < 0 )
#define plusp(x)  (floatp(x) ? getfloat(x) > 0.0 : getint(x) > 0 )
#define zerop(x)  (floatp(x) ? getfloat(x) == 0.0 : getint(x) == 0 )

#define first(x)  car(x)
#define second(x) cadr(x)
#define third(x)  caddr(x)
#define fourth(x) cadddr(x)

#define listp(x)  (consp(x)||null(x))
#define gettype(x)     ((x)->ntype)
#define getstring(x)   ((x)->n_value.svalue)
#define getint(x)      ((x)->n_value.ivalue)
#define getfloat(x)    ((x)->n_value.fvalue)
#define getchr(x)      getint(x)
#define getsymbol(x)   getstring(x)

#define caar(x) car(car(x))
#define cadr(x) car(cdr(x))
#define cdar(x) cdr(car(x))
#define cddr(x) cdr(cdr(x))

#define caaar(x) car(car(car(x)))
#define caadr(x) car(car(cdr(x)))
#define cadar(x) car(cdr(car(x)))
#define caddr(x) car(cdr(cdr(x)))
#define cdaar(x) cdr(car(car(x)))
#define cdadr(x) cdr(car(cdr(x)))
#define cddar(x) cdr(cdr(car(x)))
#define cdddr(x) cdr(cdr(cdr(x)))

#define caaaar(x) car(car(car(car(x))))
#define caaadr(x) car(car(car(cdr(x))))
#define caadar(x) car(car(cdr(car(x))))
#define caaddr(x) car(car(cdr(cdr(x))))
#define cadaar(x) car(cdr(car(car(x))))
#define cadadr(x) car(cdr(car(cdr(x))))
#define caddar(x) car(cdr(cdr(car(x))))
#define cadddr(x) car(cdr(cdr(cdr(x))))
#define cdaaar(x) cdr(car(car(car(x))))
#define cdaadr(x) cdr(car(car(cdr(x))))
#define cdadar(x) cdr(car(cdr(car(x))))
#define cdaddr(x) cdr(car(cdr(cdr(x))))
#define cddaar(x) cdr(cdr(car(car(x))))
#define cddadr(x) cdr(cdr(car(cdr(x))))
#define cdddar(x) cdr(cdr(cdr(car(x))))
#define cddddr(x) cdr(cdr(cdr(cdr(x))))

#define settype(x,v)    ((x)->ntype=(v))
#define setcar(x,v)     ((x)->n_value.n_cons._car=(v))
#define setcdr(x,v)     ((x)->n_value.n_cons._cdr=(v))
#define setint(x,v)     ((x)->n_value.ivalue=(v))
#define setfloat(x,v)   ((x)->n_value.fvalue=(v))
#define setstring(x,v)  ((x)->n_value.svalue=(v))
#define setchar(x,v)    setint(x,v)
#define setsymbol(x,v)  setstring(x,v)
#define sassoc(x,y) nthsassoc((x),(y),1)


#define dolist(X,Y,z) for( X= (z=Y)      ? car(z) : NIL ; z ; \
                           X= (z=cdr(z)) ? car(z) : NIL )

#define dotimes(i,x) for(i = 0 ; i < x ; i++ )
#define loop         for(;;)

#define newstring()  newnode(NTYPE_STR)
#define newsymbol()  newnode(NTYPE_SYM)
#define newint()     newnode(NTYPE_INT)
#define newfloat()   newnode(NTYPE_FLO)
#define newchar()    newnode(NTYPE_CHR)
#define NIL (LVAL)0

#define SCR_WIDTH 70
#define BUFFERLENGTH 1024
#define BUFFERINC    1024
#define ERRSTRLEN 5
#define UNGET_BUFFER_LENGTH 10

typedef struct _tLspObject {
  void *(*memory_allocating_function)(size_t, void *);
  void (*memory_releasing_function)(void *, void *);
  void *pMemorySegment;
  FILE *f;
  char cOpen,cClose; // the ( and the ) characters
  int tabpos,scrsize;
  char *buffer;
  long cbBuffer;
  int SymbolLength;
  int CaseFlag;
  int UngetBuffer[UNGET_BUFFER_LENGTH];
  int UngetCounter;
  } tLspObject,*tpLspObject;

*/

/*POD
=H Functions to handle LISP syntax files.

This module implements list handling. The actual structures and
names resemble the LISP naming as well as the handled file format is
LISP.

Using this module the programmer can easily read and write
LISP syntax file and can also handle the structures built up
in the memory.

Known bug:

You can only read a single file at a time.

CUT*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lsp.h"

static char escapers[] = "t\tn\nr\r";

#define ALLOC(X) (pLSP->memory_allocating_function((X),pLSP->pMemorySegment))
#define FREE(X)  (pLSP->memory_releasing_function((X),pLSP->pMemorySegment))
#define BUFFER   (pLSP->buffer)
#define TABPOS   (pLSP->tabpos)
#define SCRSIZE  (pLSP->scrsize)
/*
 * local function to decide wheter a character is within some set
 */
static isinset(int ch,char *string)
{
   while( ch != *string && *++string );
   return *string;
}

/*
 * Local defines used within this file.
 */
#define getnode() (LVAL)malloc(sizeof(struct NODE))
#define SRC_WIDTH 80  /* The width of the screen for pretty printing. */
#define WSPACE "\t \f\r\n"
#define CONST1 "!$%&*-+./0123456789:<=>?@[]^_{}~"
#define CONST2 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
#define NUMSET "0123456789"
#define NUMSET1 "0123456789-+"
#define numeral1(x) (isinset((x),NUMSET1))
#define space_p(x) isinset((x),WSPACE)
#define const_p(x) (isinset((x),CONST1)||isinset((x),CONST2))
#define const_p1(x) ((const_p(x))&&(!numeral1(x)))
#define numeral(x) (isinset((x),NUMSET))
#define spaceat(x,f) while(space_p(((x)=getC(pLSP,(f)))))

/*
 * StrDup well known, however rarely implemented function
 */
#define StrDup(x) c_StrDup(pLSP,(x))
static char * c_StrDup(tpLspObject pLSP,char *s)
{
   char *p;

   p = (char *)ALLOC(sizeof(char)*(strlen(s)+1));
   if( p == NULL )return NULL;
   strcpy(p,s);
   return p;
}
/*-----------------------------------------------------*/
/* Calculate 10^[a]                                    */
static double pow10(double a)
{
   int j,i;
   double pro,k;

   for( (i= a<0.0) && (a = -a) , j=(int)a , pro=1.0 , k=10; j ;
       j%2 && (pro *=k) , j /= 2 , k *= k )
      continue;
   i && (pro=1.0/pro);
   return pro;
}
/*
 * Convert a string to double or long.
 * string should contain the string
 * whatis 0 string is invalid
 *        1 string is float
 *        2 string is integer
 * dres   contains the result if whatis 1
 * lres   contains the result if whatis 2
 *
 * First version if MINIMOS in FORTRAN
 * Second version in MINIMOS2LISP converter
 * This version in lsp.c
 *   (Could you tell the origin?)
 */
static void cnumeric(char *string, int *whatis, double *dres, long *lres)
{
   double intpart,fracpart,exppart,man;
   int i,sig,esig;

   i=1;
   sig= 1;
   esig=1;
   (( *string == '-' && (sig=(-1)) ) || *string == '+') && string++;
   for( intpart = 0 ; numeral(*string) ; string++ )
   {
      intpart *= 10;
      intpart += (*string)-'0';
   }
   if( *string == '.' )
      for( man = 1.0 , fracpart = 0.0 ,i = 0 , string ++ ; numeral(*string)
          ; string ++ )
         fracpart += (man *= 0.1) * ((*string)-'0');
   if( *string == 'E' )
   {  string++;
      (*string == '-' && (esig=(-1))) || *string == '+' && string++;
      for( exppart=0.0 , i = 0 ; numeral(*string) ; string++)
         exppart = 10*exppart + (*string)-'0';
   }
   while( space_p(*string) )string++;
   if( *string )
   {
      *whatis = 0;
      return;
   }
   if( i )
   {
      *lres   = sig*(long)intpart;
      *whatis = 2;
      return;
   }
   *dres = sig*(intpart + fracpart)*pow10(esig*exppart);
   *whatis = 1;
   return;
}


static int __GETC(int (*pfGetCharacter)(void *),
                void *pvInput,
                int *UngetBuffer,
                int *UngetCounter
               ){
  if( *UngetCounter ){
    (*UngetCounter) --;
    return UngetBuffer[*UngetCounter];
    }
  return pfGetCharacter(pvInput);
  }

static void __UNGETC(int *UngetBuffer,
              int *UngetCounter,
              int ch
             ){
  UngetBuffer[(*UngetCounter)++] = ch;
  }

#define GETC(x) __GETC((int (*)(void *))getc,(x),pLSP->UngetBuffer,&(pLSP->UngetCounter))
#define UNGETC(x) __UNGETC(pLSP->UngetBuffer,&(pLSP->UngetCounter),x)

/*
 * getC to read skipping the comments from the file.
 *
 * Each comment returns a newline or EOF
 *
 * Reading strings normal getc is used!
 */
static int getC(tpLspObject pLSP,
                FILE *f){
   int ch;

   if( (ch=GETC(f)) == ';' )
      while( (ch=GETC(f)) != '\n' && ch != EOF )
            ;
   return ch;
}

#define SYMBOLLENGTH (pLSP->SymbolLength)
#define CASEFLAG     (pLSP->CaseFlag)

static void * _mya(size_t x,void *y){
  return malloc(x);
  }
static void _myf(void *x, void *y){
  free(x);
  }

/*POD
=section lsp_init
=H Inititlaize the LSP system
/*FUNCTION*/
LVAL lsp_init(tpLspObject pLSP,
              int SymLen,
              int CaseFlg,
              void *(*memory_allocating_function)(size_t, void *),
              void (*memory_releasing_function)(void *, void *),
              void *pMemorySegment
  ){
/*noverbatim
CUT*/
   SYMBOLLENGTH = SymLen;
   CASEFLAG     = CaseFlg;
   pLSP->memory_allocating_function = memory_allocating_function ?
                                      memory_allocating_function
                                               :
                                      _mya;
   pLSP->memory_releasing_function = memory_releasing_function ?
                                     memory_releasing_function
                                               :
                                     _myf;
   pLSP->pMemorySegment = pMemorySegment;
   pLSP->UngetCounter = 0;
   pLSP->cbBuffer = 0;
   pLSP->buffer = NULL;
   pLSP->cOpen = '(';
   pLSP->cClose = ')';
   return NIL;
}

/*POD
=section cons
=H Create a new cons node
/*FUNCTION*/
LVAL c_cons(tpLspObject pLSP
  ){
/*noverbatim
CUT*/
/*
TO_HEADER:
#define cons() c_cons(pLSP)
*/
   LVAL p;

   if( null((p = getnode())) )
      return NIL;
   settype(p,NTYPE_CON);
   setcar(p,NIL);
   setcdr(p,NIL);
   return p;
}

/*
TO_HEADER:
#define newnode(x) c_newnode(pLSP,(x))
*/
/*POD
=section newnode
=H Create a new node
Create a new node with the given type.
/*FUNCTION*/
LVAL c_newnode(tpLspObject pLSP,
               unsigned char type
  ){
/*noverbatim
CUT*/
   LVAL p;

   if( null((p = getnode())) )
      return NIL;

   settype(p,type);
   switch( type )
   {
   case NTYPE_CON:
      return NULL;
   case NTYPE_FLO:
      setfloat(p,0.0);
      break;
   case NTYPE_INT:
      setint(p,0);
      break;
   case NTYPE_STR:
      setstring(p,NULL);
      break;
   case NTYPE_SYM:
      setsymbol(p,NULL);
      break;
   case NTYPE_CHR:
      setchar(p,(char)0);
      break;
   default:
      return NULL;
   }
   return p;
}

/*
TO_HEADER:
#define symcmp(x,y) c_symcmp(pLSP,(x),(y))
*/
/*POD
=section symcmp
=H Compare a symbol to a string.

If the symbol is the same as the string then it returns the pointer to the
symbol! (Garanteed.) Otherwise it returns NIL.

The symbol and the string matches if the first SymbolLength characters
are matching, and CaseFlag says the case sensitivity.

/*FUNCTION*/
LVAL c_symcmp(tpLspObject pLSP,
              LVAL p,
              char *s
  ){
/*noverbatim
CUT*/
  int i;
  char *w,cw,cs;

  if( null(p) || !symbolp(p) )return NIL;
  /* NOTE: A string should not be so long that decrementing -1 gets to 0! */
  for( i = SYMBOLLENGTH , w = getstring(p) ;
       i && *s && *w ; i-- , s++ , w++  ){
    cw = !CASEFLAG && islower(*w) ? toupper(*w) : *w;
    cs = !CASEFLAG && islower(*s) ? toupper(*s) : *s;
    if( cw != cs )
          return NIL;
    }
  return  i && ( *w || *s ) ? NIL : p;
  }

/*
TO_HEADER:
#define nthsassoc(x,y,z) c_nthsassoc(pLSP,(x),(y),(z))
*/
/*POD
=section nthsassoc
=H Get the nth assoc from a list.

This works only for B<symbols> !!!
The second argument should be a 
string for symbol comparision.

/*FUNCTION*/
LVAL c_nthsassoc(tpLspObject pLSP,
                 LVAL p,
                 char *s,
                 int n
  ){
/*noverbatim
CUT*/
  LVAL fp;

  if( null(p) || !consp(p) )return NIL;
  for( fp = p ; fp ; fp = cdr(fp) )
    if( !car(fp) || !consp(car(fp)) || !symbolp(caar(fp)) )
            continue;
    else
      if( symcmp(caar(fp),s) && !--n )return car(fp);
  return NIL;
  }

/*
TO_HEADER:
#define freelist(x) c_freelist(pLSP,(x))
*/
/*POD
=section freelist
=H Free a list

/*FUNCTION*/
LVAL c_freelist(tpLspObject pLSP,
              LVAL p
  ){
/*noverbatim
CUT*/
   if( null(p) || freep(p) )return NIL;
   if(consp(p) )
   {
      settype(p,NTYPE_FRE);
      freelist(car(p));
      freelist(cdr(p));
   }
   if( stringp(p) )
      FREE(getstring(p));
   else if( symbolp(p) )
      FREE(getsymbol(p));
   FREE(p);
   return NIL;
}

/*
TO_HEADER:
#define flatc(x) c_flatc(pLSP,(x))
*/
/*POD
=section flatc
=H flatc returns the length of printstring

/*FUNCTION*/
int c_flatc(tpLspObject pLSP,
            LVAL p
  ){
/*noverbatim
CUT*/
  int j;
  LVAL fp;

  if( null(p) )return 3;
  switch( gettype(p) ){
    case NTYPE_CON:
      for( fp = p , j = 1/*(*/ ; fp ; fp = cdr(fp) )
      j+= flatc(car(fp))+1/*space*/;
      return p ? j : 1+j; /*) was calculated as a space. (Not always.) */
    case NTYPE_FLO:
      sprintf(BUFFER,"%lf",getfloat(p));
      break;
    case NTYPE_INT:
      sprintf(BUFFER,"%ld",getint(p));
      break;
    case NTYPE_STR:
      sprintf(BUFFER,"\"%s\"",getstring(p));
      break;
    case NTYPE_SYM:
      sprintf(BUFFER,"%s",getsymbol(p));
      break;
    case NTYPE_CHR:
      sprintf(BUFFER,"#\\%c",getchr(p));
      break;
    default:
      return 0;
      }
  return strlen(BUFFER);
  }

/*
 * local pprinting function
 *
 * Do not try to understand how it works. When I wrote it I and God knew how
 * it worked. I have forgotten...
 * Ask God!
 *
 * p is the expression is to print.
 * k is magic argument to handle non-algorithmic behaviour of pprint.
 *   (k holds internal beautiness (!) factor of printout. (AI!) :-)
 *  Serious:
 *    k=1  we dunno anything about expression
 *    k=1  the tabulatig spaces were alrady printed!
 *    k=2  we are in flatc mode
 *          -dont print tabulating space
 *          -there is no need to check flatc size.
 */
static LVAL __pprint(tpLspObject pLSP,LVAL p,int k)
#define _pprint(x) __pprint(pLSP,(x),1)
{
   LVAL fp;
   int j,multiline;
   char *s;

   if( null(p) )
   {
      fprintf(pLSP->f,"NIL");
      return NIL;
   }
   switch(gettype(p))
   {
   case NTYPE_CON:
      if( k == 2 || flatc(p) < SCRSIZE-TABPOS )
      {
         /* Print in flat mode. */
         if( k == 1 )
            fprintf(pLSP->f,"%*s(",TABPOS,"");
         else
            fprintf(pLSP->f,"(");
         for( fp = p ; fp ;  )
         {
            __pprint(pLSP,car(fp),2);
            fp = cdr(fp);
            if( fp )
               fprintf(pLSP->f," ");
         }
         fprintf(pLSP->f,")");
         return NIL;
      }
      if( atom(fp=car(p)) || flatc(fp) < (SCRSIZE-TABPOS)/2 )
      {
         fprintf(pLSP->f,"(");
         SCRSIZE--; /* Schrink screen size thinking of the closing paren. */
         j = flatc(fp)+2;/* ([flatc]SPACE */
         TABPOS += j;
         __pprint(pLSP,fp,0);
         if( cdr(p) )
         {
            fprintf(pLSP->f," ");
            __pprint(pLSP,cadr(p),0);
            fprintf(pLSP->f,"\n");
            for( fp = cdr(cdr(p)) ; fp ; )
            {
               fprintf(pLSP->f,"%*s",TABPOS,"");
               __pprint(pLSP,car(fp),0);
               fp = cdr(fp);
               if( fp )
                  fprintf(pLSP->f,"\n");
            }
         }
         TABPOS -= j;
         fprintf(pLSP->f,")");
         SCRSIZE++;
         return NIL;
      }
      fprintf(pLSP->f,"(");
       /* Schrink screen size thinking of the closing paren. */
      SCRSIZE--;
      TABPOS++;
      __pprint(pLSP,car(p),0);
      if( fp = cdr(p) )
         fprintf(pLSP->f,"\n");
      while( fp )
      {
         fprintf(pLSP->f,"%*s",TABPOS,"");
         _pprint(car(fp));
         fp = cdr(fp);
            if( fp )
               fprintf(pLSP->f,"\n");
      }
      TABPOS--;
      fprintf(pLSP->f,")");
      SCRSIZE++;
      return NIL;
   case NTYPE_FLO:
      fprintf(pLSP->f,"%lf",getfloat(p));
      return NIL;
   case NTYPE_INT:
      fprintf(pLSP->f,"%ld",getint(p));
      return NIL;
   case NTYPE_STR:
      multiline = 0;
      for( s=getstring(p) ; *s ; s++ )
        if( *s == '\n' ){
          multiline = 1;
          break;
          }

      fprintf(pLSP->f,multiline ? "\"\"\"" : "\"");
      for( s=getstring(p) ; *s ; s++ )
         switch( *s )
         {                      /* Handle spacial characters. */
         case '\"':
            fprintf(pLSP->f,"\\\"");
            break;
         default:
            fprintf(pLSP->f,"%c",*s);
            break;
         }
      fprintf(pLSP->f,multiline ? "\"\"\"" : "\"");
      return NIL;
   case NTYPE_SYM:
      fprintf(pLSP->f,"%s",getsymbol(p));
      return NIL;
   case NTYPE_CHR:
      fprintf(pLSP->f,"#\\%c",getchr(p));
      return NIL;
   default:
      return NIL;
   }
   fprintf(pLSP->f,BUFFER);
   return NIL;
}


/*
TO_HEADER:
#define pprint(x,y) c_pprint(pLSP,(x),(y))
*/
/*POD
=section pprint
=H Pretty print a list.
=verbatim
pp-list
 Pretty-print a list expression.
     IF <the flatsize length of *expr is less than pp-maxlen*>
         THEN print the expression on one line,
     ELSE
     IF <the car of the expression is an atom> or
        <the flatsize length of the car of the expression is less than
         the half of the rest of the space>
         THEN print the expression in the following form:
                 "(<item0> <item1>
                           <item2>
                             ...
                           <itemn> )"
     ELSE
     IF <the car of the expression is a list>
         THEN print the expression in the following form:
                 "(<list1>
                   <item2>
                     ...
                   <itemn> )"


If an expression can not fit into the area
| -------------------------------------------------------- | then it falls out and gets the end into the new line without printing \n :-(
=noverbatim
/*FUNCTION*/
LVAL c_pprint(tpLspObject pLSP,
            LVAL p,
            FILE *file
  ){
/*noverbatim
CUT*/
  /* We start in the first column. */
  TABPOS = 0;
  /* Screen is not schrinked. */
  SCRSIZE = SCR_WIDTH;
  pLSP->f = file;
  _pprint(p);
  fprintf(pLSP->f,"\n");
  return NIL;
  }

/*
 * local function to read a cons node
 */
static LVAL readcons(tpLspObject pLSP,FILE *f)
{
   int ch;

   spaceat(ch,f);
   if( ch == pLSP->cClose )return NIL;
   UNGETC(ch);
   return readlist(f);
}

/* Store a character in the buffer at position 'index'
   reallocate the buffer if neccessary.
*/
static int storech(tpLspObject pLSP,int i,int ch){
  char *pszNewBuffer;
  if( i >= pLSP->cbBuffer - 1 ){
    pszNewBuffer = ALLOC(pLSP->cbBuffer+BUFFERINC);
    if( pszNewBuffer == NULL )return 1;
    if( pLSP->cbBuffer )
      memcpy(pszNewBuffer,pLSP->buffer,pLSP->cbBuffer);
    if( pLSP->buffer )
      FREE(pLSP->buffer);
    pLSP->buffer = pszNewBuffer;
    pLSP->cbBuffer += BUFFERINC;
    }
  pLSP->buffer[i++] = ch;
  pLSP->buffer[i] = (char)0;
  return 0;
  }

/*
 * local function to read an expression
 */
static LVAL _readexpr(tpLspObject pLSP,FILE *f)
{
   int ch,ch1,ch2,i;
   LVAL p;
   char *s;
   double dval;
   long lval;


   spaceat(ch,f);
   if( ch == EOF )
   {
      return NIL;
   }
   if( ch == pLSP->cClose )
   {
      return NIL;
   }

   if( ch == pLSP->cOpen )/* Read a cons node. */
      return readcons(pLSP,f);

   /**** Note: XLISP allows 1E++10 as a symbol. This is dangerous.
         We do not change XLISP (so far), but here I exclude all symbol
         names starting with numeral. */
   if( const_p1(ch) )/* Read a symbol. */
   {
      for( i = 0 ; const_p(ch) ; i++ ){
        if( storech(pLSP,i,ch) )return NIL;
        ch = getC(pLSP,f);
        }
      UNGETC(ch);
      /* Recognize NIL and nil symbols. */
      if( !strcmp(BUFFER,"NIL") || !strcmp(BUFFER,"nil") )
         return NIL;
      p = newsymbol();
      s = StrDup( BUFFER );
      if( null(p) || s == NULL )return NIL;
      setsymbol(p,s);
      return p;
   }
   if( ch == '\"' ){
     ch = GETC(f);
     storech(pLSP,0,0); /* inititalize the buffer */
     if( ch != '\"' )goto SimpleString;
     ch = GETC(f);
     if( ch != '\"' ){
       UNGETC(ch);
       ch = '\"';/* ch should hold the first character of the string that is " now */
       goto SimpleString;
       }
     ch = GETC(f);     
     /* multi line string */
     for( i = 0 ; ch != EOF ; i++ ){
       if( ch == '\"' ){
         ch1 = GETC(f);
         ch2 = GETC(f);
         if( ch1 == '\"' && ch2 == '\"' )break;
         UNGETC(ch2);
         UNGETC(ch1);
         }
       if( ch == '\\' ){
         ch = GETC(f);
         s = escapers;
         while( *s ){
           if( *s++ == ch ){
             ch = *s;
             break;
             }
           if( *s )s++;
           }
         }
       if( storech(pLSP,i,ch) )return NIL;
       ch = GETC(f);
       }
     p = newstring();
     s = StrDup( BUFFER );
     if( null(p) || s == NULL )return NIL;
     setstring(p,s);
     return p;
     }

   if( ch == '\"' ){/* Read a string. */
     ch = GETC(f);/* Eat the " character. */
SimpleString:
     for( i = 0 ; ch != '\"' && ch != EOF ; i++ ){
       if( ch == '\\' ){
         ch = GETC(f);
         s = escapers;
         while( *s ){
           if( *s++ == ch ){
             ch = *s;
             break;
             }
           if( *s )s++;
           }
         }
       if( ch == '\n' )return NIL;
       if( storech(pLSP,i,ch) )return NIL;
       ch = GETC(f);
       }
      p = newstring();
      s = StrDup( BUFFER );
      if( null(p) || s == NULL )
      {
         return NIL;
      }
      setstring(p,s);
      return p;
   }
   if( numeral1(ch) )
   {
      for( i = 0 ; isinset(ch,"0123456789+-eE.") ; i++ )
      {
         if( storech(pLSP,i,ch) )return NIL;
         ch = getC(pLSP,f);
      }
      UNGETC(ch);
      cnumeric(BUFFER,&i,&dval,&lval);
      switch( i )
      {
      case 0:
         return NIL;
      case 1:
         /* A float number is coming. */
         p = newfloat();
         if( null(p) )
         {
            return NIL;
         }
         setfloat(p,dval);
         return p;
      case 2:
         /* An integer is coming. */
         p = newint();
         if( null(p) )
         {
            return NIL;
         }
         setint(p,lval);
         return p;
      default:
         return NIL;
      }
   }
   return NIL;
}

/*
TO_HEADER:
#define readlist(x) c_readlist(pLSP,(x))
*/
/*POD
=section readlist
=H Read a list from a file.

The opening '(' character should be away already!
/*FUNCTION*/
LVAL c_readlist(tpLspObject pLSP,
                FILE *f
  ){
/*noverbatim
CUT*/
   int ch;
   LVAL p,q;

   spaceat(ch,f);
   if( ch == pLSP->cClose || ch == EOF )return NIL;
   UNGETC(ch);
   q = cons();
   if( null(q) )
   {
      return NIL;
   }
   p = _readexpr(pLSP,f);
   setcar(q,p);
   setcdr(q,readlist(f));
   return q;
}


/*
TO_HEADER:
#define readexpr(x) c_readexpr(pLSP,(x))
*/
/*POD
=section readexpr
=H Read an expression from a file.
/*FUNCTION*/
LVAL c_readexpr(tpLspObject pLSP,
                FILE *f
  ){
/*noverbatim
CUT*/
  int ch;

  spaceat(ch,f);
  if( ch == EOF )return NIL;
  UNGETC(ch);
  return _readexpr(pLSP,f);
  }

/*
TO_HEADER:
#define skipexpr(x) c_skipexpr(pLSP,(x))
*/
/*POD
=section skipexpr
=H  Reads an expression and forgets
/*FUNCTION*/
LVAL c_skipexpr(tpLspObject pLSP,
                FILE *f
  ){
/*noverbatim
CUT*/
  LVAL p;

  p = readexpr(f);
  freelist(p);
  return NIL;
  }

/*
TO_HEADER:
#define llength(x) c_llength(pLSP,(x))
*/
/*POD
=section llength
=H  Calculates the length of a list.
/*FUNCTION*/
int c_llength(tpLspObject pLSP,
              LVAL p
  ){
/*noverbatim
CUT*/
  int k;

  for( k = 0 ; p ; k++ )
    p = cdr(p);
  return k;
  }

/*
TO_HEADER:
#define nth(x,y) c_nth(pLSP,(x),(y))
*/
/*POD
=section nth
=H Returns the nth element of a list.
/*FUNCTION*/
LVAL c_nth(tpLspObject pLSP,
         int n,
         LVAL p
  ){
/*noverbatim
CUT*/
  LVAL q;

  for( q = p ; n && q ; q = cdr(q) )n--;

  return q ? car(q) : NIL;
  }

/*
TO_HEADER:
#define nthcdr(x,y) c_nthcdr(pLSP,(x),(y))
*/
/*POD
=section nthcdr
=H Returns the nthcdr element of a list.
/*FUNCTION*/
LVAL c_nthcdr(tpLspObject pLSP,
              int n,
              LVAL p
  ){
/*noverbatim
CUT*/
  LVAL q;

  for( q = p ; n && q ; q = cdr(q) )n--;

  return q;
  }

/*
TO_HEADER:
#define char_code(x) c_char_code(pLSP,(x))
*/
/*POD
=section char_code
=H Returns the character code of a character.
/*FUNCTION*/
LVAL c_char_code(tpLspObject pLSP,
                 LVAL p
  ){
/*noverbatim
CUT*/
  LVAL q;

  if( null(p) || !characterp(p) )return NIL;
  q = newint();
  setint(q,(int)getchr(p));
  return q;
  }

/*
TO_HEADER:
#define code_char(x) c_code_char(pLSP,(x))
*/
/*POD
=section code_char
=H Returns the character character of a code.
/*FUNCTION*/
LVAL c_code_char(tpLspObject pLSP,
                 LVAL p
  ){
/*noverbatim
CUT*/
  LVAL q;

  if( null(p) || !integerp(p) )return NIL;
  q = newchar();
  setchar(q,(char)getint(p));
  return q;
  }

/*
TO_HEADER:
#define char_downcase(x) c_char_downcase(pLSP,(x))
*/
/*POD
=section char_downcase
=H Returns the lower case equivalent of the character.
/*FUNCTION*/
LVAL c_char_downcase(tpLspObject pLSP,
                     LVAL p
  ){
/*noverbatim
CUT*/
  LVAL q;

  if( null(p) || !characterp(p) )return NIL;
  q = newchar();
  setchar(q, (isalpha(getchr(p)) && isupper(getchr(p))) ?
          tolower((int) getchr(p)) : getchr(p));
  return q;
  }

/*
TO_HEADER:
#define char_upcase(x) c_char_upcase(pLSP,(x))
*/
/*POD
=section char_upcase
=H Returns the upper case equivalent of the character.
/*FUNCTION*/
LVAL c_char_upcase(tpLspObject pLSP,
                   LVAL p
  ){
/*noverbatim
CUT*/
  LVAL q;

  if( null(p) || !characterp(p) )return NIL;
  q = newchar();
  setchar(q, (isalpha(getchr(p)) && islower(getchr(p))) ?
          toupper((int) getchr(p)) : getchr(p));
  return q;
  }

/*
TO_HEADER:
#define equal(x,y) c_equal(pLSP,(x),(y))
*/
/*POD
=section equal
=H Performs equal LISP function.

Returns 1 if p and q are equal and 0 if not.
/*FUNCTION*/
int c_equal(tpLspObject pLSP,
            LVAL p,
            LVAL q
  ){
/*noverbatim
CUT*/
  if( p == q ) return 1;
  if( gettype(p) != gettype(q) )return 0;
  switch( gettype(p) ){
    case NTYPE_CON:
      return equal(car(p),car(q)) && equal(cdr(p),cdr(q));
    case NTYPE_FLO:
      return getfloat(p)==getfloat(q);
    case NTYPE_INT:
      return getint(p)==getint(q);
    case NTYPE_STR:
      return  getstring(p) == getstring(q) ||
                       !strcmp(getstring(p),getstring(q));
    case NTYPE_SYM:
      return getsymbol(p) == getsymbol(q) ||
                 !strcmp(getsymbol(p),getsymbol(q));
    case NTYPE_CHR:
      return getchr(p) == getchr(q);
    default:
      return 0;
      break;
    }
  }

/*
TO_HEADER:
#define car(x) c_car(pLSP,(x))
*/
/*POD
=section car
=H car

/*FUNCTION*/
LVAL c_car(tpLspObject pLSP,
         LVAL x
  ){
/*noverbatim
CUT*/
  if( null(x) )return NIL;
  return ((x)->n_value.n_cons._car);
  }

/*
TO_HEADER:
#define cdr(x) c_cdr(pLSP,(x))
*/
/*POD
=section cdr
=H cdr

/*FUNCTION*/
LVAL c_cdr(tpLspObject pLSP,
         LVAL x
  ){
/*noverbatim
CUT*/
   if( null(x) )return NIL;
   return ((x)->n_value.n_cons._cdr);
}

/*
TO_HEADER:
#define consp(x) c_consp(pLSP,(x))
*/
/*POD
=section consp
=H consp

/*FUNCTION*/
int c_consp(tpLspObject pLSP,
            LVAL x
  ){
/*noverbatim
CUT*/
  if( null(x) )return 0;
  return ((x)->ntype == NTYPE_CON);
  }

/*
TO_HEADER:
#define floatp(x) c_floatp(pLSP,(x))
*/
/*POD
=section floatp
=H floatp

/*FUNCTION*/
int c_floatp(tpLspObject pLSP,
         LVAL x
  ){
/*noverbatim
CUT*/
  if( null(x) )return 0;
  return ((x)->ntype == NTYPE_FLO);
  }

/*
TO_HEADER:
#define integerp(x) c_integerp(pLSP,(x))
*/
/*POD
=section integerp
=H integerp

/*FUNCTION*/
int c_integerp(tpLspObject pLSP,
         LVAL x
  ){
/*noverbatim
CUT*/
  if( null(x) )return 0;
  return ((x)->ntype == NTYPE_INT);
  }

/*
TO_HEADER:
#define stringp(x) c_stringp(pLSP,(x))
*/
/*POD
=section stringp
=H stringp

/*FUNCTION*/
int c_stringp(tpLspObject pLSP,
         LVAL x
  ){
/*noverbatim
CUT*/
  if( null(x) )return 0;
  return ((x)->ntype == NTYPE_STR);
  }

/*
TO_HEADER:
#define symbolp(x) c_symbolp(pLSP,(x))
*/
/*POD
=section symbolp
=H symbolp

/*FUNCTION*/
int c_symbolp(tpLspObject pLSP,
         LVAL x
  ){
/*noverbatim
CUT*/
  if( null(x) )return 0;
  return ((x)->ntype == NTYPE_SYM);
  }

/*
TO_HEADER:
#define characterp(x) c_characterp(pLSP,(x))
*/
/*POD
=section characterp
=H characterp

/*FUNCTION*/
int c_characterp(tpLspObject pLSP,
         LVAL x
  ){
/*noverbatim
CUT*/
  if( null(x) )return 0;
  return ((x)->ntype == NTYPE_CHR);
  }

/*
TO_HEADER:
#define atom(x) c_atom(pLSP,(x))
*/
/*POD
=section atom
=H atom

/*FUNCTION*/
int c_atom(tpLspObject pLSP,
         LVAL x
  ){
/*noverbatim
CUT*/
  if( null(x) )return 0;
  return ((x)->ntype != NTYPE_CON);
  }
