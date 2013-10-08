/* reinterf.c

Interface file for the regular expression functions to be used as a
ScriptBasic application.

NTLIBS:
UXLIBS:
DWLIBS:

*/
#include <stdio.h>
#include <string.h>

#include "../../basext.h"

#include <sys/types.h>
#include "regex.h"

/**
=H The module RE

This module implements some simple regular expression handling functions.
They can be used to match some strings against regular expressions and to
use matched sub strings to format replace strings. This module provides
functions that are similar to the Perl language operators T<m//> and T<s///>.



*/

typedef struct _ModuleObject {
  int iErrorRegcomp;        /* error returned by regcomp */
  int iErrorRegex;          /* error returned by regex   */
  unsigned long lMatches;   /* the number of () matches during the last match */
  unsigned long *lMatchLen; /* the length of the matches */ 
  char **pszMatch;          /* string array if the () matches */
  } ModuleObject, *pModuleObject;


#define RE_REG_NOMATCH    0x00080001
#define RE_REG_BADPAT     0x00080002
#define RE_REG_ECOLLATE   0x00080003
#define RE_REG_ECTYPE     0x00080004
#define RE_REG_EESCAPE    0x00080005
#define RE_REG_ESUBREG    0x00080006
#define RE_REG_EBRACK     0x00080007
#define RE_REG_EPAREN     0x00080008
#define RE_REG_EBRACE     0x00080009
#define RE_REG_BADBR      0x0008000A
#define RE_REG_ERANGE     0x0008000B
#define RE_REG_ESPACE     0x0008000C
#define RE_REG_BADRPT     0x0008000D
#define RE_REG_EMPTY      0x0008000E
#define RE_REG_ASSERT     0x0008000F
#define RE_REG_INVARG     0x00080010

static int ConvertError(int Error){
  switch( Error ){
    case REG_NOMATCH    : return RE_REG_NOMATCH;
    case REG_BADPAT     : return RE_REG_BADPAT;
    case REG_ECOLLATE   : return RE_REG_ECOLLATE;
    case REG_ECTYPE     : return RE_REG_ECTYPE;
    case REG_EESCAPE    : return RE_REG_EESCAPE;
    case REG_ESUBREG    : return RE_REG_ESUBREG;
    case REG_EBRACK     : return RE_REG_EBRACK;
    case REG_EPAREN     : return RE_REG_EPAREN;
    case REG_EBRACE     : return RE_REG_EBRACE;
    case REG_BADBR      : return RE_REG_BADBR;
    case REG_ERANGE     : return RE_REG_ERANGE;
    case REG_ESPACE     : return RE_REG_ESPACE;
    case REG_BADRPT     : return RE_REG_BADRPT;
    case REG_EMPTY      : return RE_REG_EMPTY;
    case REG_ASSERT     : return RE_REG_ASSERT;
    case REG_INVARG     : return RE_REG_INVARG;
    default: return 0x00080011;
    }
  }


besVERSION_NEGOTIATE

  return (int)INTERFACE_VERSION;

besEND

besSUB_START
  pModuleObject p;

  besMODULEPOINTER = besALLOC(sizeof(ModuleObject));
  if( besMODULEPOINTER == NULL )return 0;
  p = (pModuleObject)besMODULEPOINTER;

  p->lMatches = 0;

besEND

#define reset_pmatches(p) \
 if( p->lMatches ){\
   besFREE(p->lMatchLen);\
   p->lMatchLen = NULL;\
   besFREE(p->pszMatch);\
   p->pszMatch = NULL;\
   p->lMatches = 0;\
   }

besSUB_FINISH
  pModuleObject p;

  p = (pModuleObject)besMODULEPOINTER;
  reset_pmatches(p);

  return 0;
besEND



/**
=section match
=H Match a string against a regular expression

=verbatim
re::match(string,regexp [, replace])
re::m(string,regexp [, replace])
=noverbatim

This function is the main regular expression match function. The first argument 
is the string that is to be matched against the regular expression. The second 
argument is the regular expression. If the string matches the regular expression the 
return value of the function is true, otherwise false. The regular expressions 
may contain parentheses inside it. If it does the substrings matching the 
regular expression part between the parentheses will be stored in a 
regular expression dollar array.

The substrings can be accessed via the function T<re::dollar()> R<dollar>.

If there is a replace string defined the return value is either false 
if the string is not matched by the regular expression; or the replace string itself.
The replace string may contain T<$0>, T<$1>, ... T<$n> literal that will be replaced by 
the actual substrings between the parentheses. This is the same way as Perl does.

T<$0> is the substring matched by the whole regular expression.

For example:
=verbatim
import re.bas

print "match result=",re::match("ahlma","h(.*)","haho $0 $1 $0 q")
print
n = re::n()
print "number of ()s =",n
print
for i=0 to n
  print i,". "
  print re::dollar(i)
  print
next i
=noverbatim
will print
=verbatim
match result=haho hlma lma hlma q
number of ()s =1
0. hlma
1. lma
=noverbatim
Note that the short for T<re::m> exists in case you are a Perl
knowing programmer, because this function is similar to the 
Perl operator T<=~ m/>.

*/
besFUNCTION(match)
  regex_t *preg;
  regmatch_t *pmatch;
  VARIABLE vString,vRegexp,vReplace;
  size_t nmatch;
  pModuleObject p;
  int iError;
  int eflags,eopt;
  int i;

  p = (pModuleObject)besMODULEPOINTER;

  /* if there was any () match before release those */
  reset_pmatches(p);

  /* get the regular expression */
  vRegexp = besARGUMENT(2);
  besDEREFERENCE(vRegexp);
  /* undef as regular expression matches nothing */
  if( vRegexp == NULL ){
    besALLOC_RETURN_LONG;
    LONGVALUE(besRETURNVALUE) = 0;
    }
  /* we need this as string */
  vRegexp = besCONVERT2STRING(vRegexp);
  preg = (regex_t *)besALLOC(sizeof(regex_t));
  if( preg == NULL )return COMMAND_ERROR_MEMORY_LOW;
  preg->re_endp = STRINGVALUE(vRegexp)+STRLEN(vRegexp);

  eflags = REG_PEND|REG_EXTENDED;
  eopt   = besOPTION("re$flag");
  if( eopt )eopt = ~ eopt;

#define SBCASE    0x0001
#define SBNEWLINE 0x0002

  if( eopt & SBCASE )eflags |= REG_ICASE;
  if( eopt & SBNEWLINE )eflags |= REG_NEWLINE;

  /* if regexp is erroneous return false */
  if( p->iErrorRegcomp = regcomp(preg, STRINGVALUE(vRegexp), eflags) ){
    besALLOC_RETURN_LONG;
    LONGVALUE(besRETURNVALUE) = 0;
    return ConvertError(p->iErrorRegcomp);
    }

  nmatch = preg->re_nsub + 1;

  vString = besARGUMENT(1);
  besDEREFERENCE(vString);
  /* we need this as string */
  vString = besCONVERT2STRING(vString);
  
  pmatch = (regmatch_t *)besALLOC(sizeof(regmatch_t) * nmatch );
  if( pmatch == NULL )return COMMAND_ERROR_MEMORY_LOW;
  pmatch->rm_so = 0;
  pmatch->rm_eo = STRLEN(vString);
  p->iErrorRegex = regexec(preg, STRINGVALUE(vString),nmatch,pmatch,REG_STARTEND);

  /* STORE THE PARENTHESED MATCH RESULTS */
  if( ! p->iErrorRegex && nmatch ){

    /* allocate space for the strings */

    p->lMatches = nmatch;
    p->lMatchLen = besALLOC(nmatch*sizeof(long));
    if( p->lMatchLen == NULL ){
      p->lMatches = 0;
      regfree(preg);
      besFREE(pmatch);
      besFREE(preg);
      return COMMAND_ERROR_MEMORY_LOW;
      }

    p->pszMatch = besALLOC(nmatch*sizeof(char *));
    if( p->pszMatch == NULL ){
      besFREE(p->lMatchLen);
      p->lMatches = 0;
      regfree(preg);
      besFREE(pmatch);
      besFREE(preg);
      return COMMAND_ERROR_MEMORY_LOW;
      }

    /* copy the substring into the dollar buffer */
    for( i=0 ; ((size_t)i) < nmatch ; i++ ){
      p->lMatchLen[i] = pmatch[i].rm_eo - pmatch[i].rm_so;
      if( p->lMatchLen[i] )
        p->pszMatch[i] = besALLOC(p->lMatchLen[i]);
      else
        p->pszMatch[i] = besALLOC(1);/* allocate a dummy byte to store a null length string */
      if( p->pszMatch[i] == NULL ){
        do{ besFREE(p->pszMatch[--i]); }while(i);
        regfree(preg);
        besFREE(pmatch);
        besFREE(preg);
        return COMMAND_ERROR_MEMORY_LOW;
        }
      if( p->lMatchLen[i] )/* copy all non-zero-length strings */
        memcpy(p->pszMatch[i],STRINGVALUE(vString)+pmatch[i].rm_so,p->lMatchLen[i]);
      }
    }

  /* if there is a replace argument we return the result string. */
  vReplace = besARGUMENT(3);
  besDEREFERENCE(vReplace);
  if( vReplace ){
    vReplace = besCONVERT2STRING(vReplace);
    i = 0;
    iError =
    besMatchSize(STRINGVALUE(vReplace),
                 STRLEN(vReplace),
                 p->lMatchLen,
                 p->lMatches,&i);
    if( iError ){
      besRETURNVALUE = NULL;
      regfree(preg);
      besFREE(pmatch);
      besFREE(preg);
      return COMMAND_ERROR_SUCCESS;
      }
    besALLOC_RETURN_STRING(i);
    iError =
    besMatchParameter(STRINGVALUE(vReplace),
                      STRLEN(vReplace),
                      p->pszMatch,
                      p->lMatchLen,
                      STRINGVALUE(besRETURNVALUE),
                      p->lMatches,
                      &(STRLEN(besRETURNVALUE)));
    if( iError ){
      besRETURNVALUE = NULL;
      regfree(preg);
      besFREE(pmatch);
      besFREE(preg);
      return COMMAND_ERROR_SUCCESS;
      }
    }else{
    besALLOC_RETURN_LONG;
    LONGVALUE(besRETURNVALUE) = p->iErrorRegex ? 0 : -1;
    }

  regfree(preg);
  besFREE(pmatch);
  besFREE(preg);
  
besEND

/**
=section n
=H number of matches

=verbatim
re::n()
=noverbatim
return the number of usable matches substrings
*/
besFUNCTION(nmatch)
  pModuleObject p;

  p = (pModuleObject)besMODULEPOINTER;

  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = p->lMatches -1;
  
besEND

/**
=section dollar
=H Return the i-th sub string

=verbatim
re::dollar(i)
re::$(i)
=noverbatim

return the i-th matched substring 
*/
besFUNCTION(dollar)
  pModuleObject p;
  VARIABLE Argument;
  long n;

  p = (pModuleObject)besMODULEPOINTER;
  n = 0; /* as a default we get the total match */
  besRETURNVALUE = NULL;
  if( p->pszMatch == NULL )return COMMAND_ERROR_SUCCESS;
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument != NULL ){
    Argument = besCONVERT2LONG(Argument);
    n = LONGVALUE(Argument);
    }
  if( ((unsigned)n) > p->lMatches || n < 0 )return COMMAND_ERROR_SUCCESS;
  besALLOC_RETURN_STRING(p->lMatchLen[n]);
  STRLEN(besRETURNVALUE) = p->lMatchLen[n];
  memcpy(STRINGVALUE(besRETURNVALUE),p->pszMatch[n],STRLEN(besRETURNVALUE));
  
besEND

/**
=section reset
=H Reset matches

=verbatim
re::reset()
=noverbatim

Delete the R<dollar> string array and release all memory that was allocated during the
last match. This function is autoamtically invoked between matches.
*/
besFUNCTION(reset)
  pModuleObject p;

  p = (pModuleObject)besMODULEPOINTER;
  /* if there was any () match before release those */
  reset_pmatches(p);
besEND

/**
=section format
=H Format a string with replacement sub-strings

=verbatim
re::format(string)
=noverbatim

Use this function to format a string using the T<$1>, T<$2>, ... T<$n> placeholders to be replaced by the actual strings after a successful pattern matching.
*/
besFUNCTION(format)
  VARIABLE vReplace;
  pModuleObject p;
  int iError;
  int i;

  p = (pModuleObject)besMODULEPOINTER;
  if( p->lMatches == 0 ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

  /* if there is a replace argument we return the result string. */
  vReplace = besARGUMENT(1);
  if( vReplace == NULL ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }
  vReplace = besCONVERT2STRING(vReplace);
  i = 0;
  iError =
  besMatchSize(STRINGVALUE(vReplace),
               STRLEN(vReplace),
               p->lMatchLen,
               p->lMatches,&i);
  if( iError ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }
  besALLOC_RETURN_STRING(i);
  iError =
  besMatchParameter(STRINGVALUE(vReplace),
                    STRLEN(vReplace),
                    p->pszMatch,
                    p->lMatchLen,
                    STRINGVALUE(besRETURNVALUE),
                    p->lMatches,
                    &(STRLEN(besRETURNVALUE)));
  if( iError ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

besEND

struct list{
  unsigned long len;
  char *string;
  struct list *next;
  };

static void release_list(pSupportTable pSt, struct list *p){
  struct list *q;

  while( p ){
    q = p;
    p = p->next;
    if( q->string )besFREE(q->string);
    besFREE(q);
    }
  }
#define ReleaseList(X) release_list(pSt,(X))

/**
=section replace
=H Search and replace
=verbatim
re::replace(string,regexp,replace)
re::s(string,regexp,replace)
=noverbatim
This function searches the string for matches of the regular expression and replaces the 
actual matches with the replace string. This is the same functionality as the T<=~s/> operator in Perl.

The function fills the R<dollar> array the same way as the function R<match>.

For example

=verbatim
import re.bas

print re::s("almakbat*","a(.)","$1s")
print
print re::$(0)," ",re::$(1)
print
=noverbatim
will print

=verbatim
lsmksbts*
at t
=noverbatim

As you can see in this case the value of T<re::$(0)> is the string that was
matched by the last replace inside the string.
*/
besFUNCTION(replace)
  regex_t *preg;
  regmatch_t *pmatch;
  VARIABLE vString,vRegexp,vReplace;
  size_t nmatch;
  pModuleObject p;
  int iError;
  int eflags,eopt;
  int i;
  int iStart; /* where the substring to match starts */
  int iTerm;  /* where the last substring finished */
  struct list *root,**last,*item;
 
  root = NULL;
  last = &root;

  p = (pModuleObject)besMODULEPOINTER;

  /* get the regular expression */
  vRegexp = besARGUMENT(2);
  besDEREFERENCE(vRegexp);
  /* undef as regular expression matches nothing */
  if( vRegexp == NULL ){
    besALLOC_RETURN_LONG;
    LONGVALUE(besRETURNVALUE) = 0;
    }
  /* we need this as string */
  vRegexp = besCONVERT2STRING(vRegexp);
  preg = (regex_t *)besALLOC(sizeof(regex_t));
  if( preg == NULL )return COMMAND_ERROR_MEMORY_LOW;
  preg->re_endp = STRINGVALUE(vRegexp)+STRLEN(vRegexp);

  eflags = REG_PEND|REG_EXTENDED;
  eopt   = besOPTION("re$flag");
  if( eopt )eopt = ~ eopt;

  if( eopt & SBCASE )eflags |= REG_ICASE;
  if( eopt & SBNEWLINE )eflags |= REG_NEWLINE;

  eopt   = besOPTION("re$replaceone");

  /* if regexp is erroneous return false */
  if( p->iErrorRegcomp = regcomp(preg, STRINGVALUE(vRegexp), eflags) ){
    besALLOC_RETURN_LONG;
    LONGVALUE(besRETURNVALUE) = 0;
    return ConvertError(p->iErrorRegcomp);
    }

  nmatch = preg->re_nsub + 1;

  vString = besARGUMENT(1);
  besDEREFERENCE(vString);
  /* we need this as string */
  vString = besCONVERT2STRING(vString);

  /* there should be a replace argument. */
  vReplace = besARGUMENT(3);
  besDEREFERENCE(vReplace);
  vReplace = besCONVERT2STRING(vReplace);

  for( iStart = 0 ; ((unsigned)iStart) < STRLEN(vString) ; ){
    pmatch = (regmatch_t *)besALLOC(sizeof(regmatch_t) * nmatch );
    if( pmatch == NULL )return COMMAND_ERROR_MEMORY_LOW;
    pmatch->rm_so = iStart;          /* from the start of the substring */
    pmatch->rm_eo = STRLEN(vString); /* to the end                      */
    p->iErrorRegex = regexec(preg, STRINGVALUE(vString),nmatch,pmatch,REG_STARTEND);

    /* if there is no more match then exit from the match loop */
    if( p->iErrorRegex == REG_NOMATCH  )break;

    /* if there was any () match before release those */
    reset_pmatches(p);

    /* STORE THE PARENTHESED MATCH RESULTS */
    if( ! p->iErrorRegex && nmatch ){

      /* allocate space for the strings */

      p->lMatches = nmatch;
      p->lMatchLen = besALLOC(nmatch*sizeof(long));
      if( p->lMatchLen == NULL ){
        p->lMatches = 0;
        regfree(preg);
        besFREE(pmatch);
        besFREE(preg);
        return COMMAND_ERROR_MEMORY_LOW;
        }

      p->pszMatch = besALLOC(nmatch*sizeof(char *));
      if( p->pszMatch == NULL ){
        besFREE(p->lMatchLen);
        p->lMatches = 0;
        regfree(preg);
        besFREE(pmatch);
        besFREE(preg);
        return COMMAND_ERROR_MEMORY_LOW;
        }

      /* copy the substring into the dollar buffer */
      for( i=0 ; ((size_t)i) < nmatch ; i++ ){
        p->lMatchLen[i] = pmatch[i].rm_eo - pmatch[i].rm_so;
        if( p->lMatchLen[i] )
          p->pszMatch[i] = besALLOC(p->lMatchLen[i]);
        else
          p->pszMatch[i] = besALLOC(1);/* allocate a dummy byte to store a null length string */
        if( p->pszMatch[i] == NULL ){
          do{ besFREE(p->pszMatch[--i]); }while(i);
          regfree(preg);
          besFREE(pmatch);
          besFREE(preg);
          return COMMAND_ERROR_MEMORY_LOW;
          }
        if( p->lMatchLen[i] )/* copy all non-zero-length strings */
          memcpy(p->pszMatch[i],STRINGVALUE(vString)+pmatch[i].rm_so,p->lMatchLen[i]);
        }
      }

    i = 0;
    iError =
    besMatchSize(STRINGVALUE(vReplace),
                 STRLEN(vReplace),
                 p->lMatchLen,
                 p->lMatches,&i);
    if( iError ){
      besRETURNVALUE = NULL;
      regfree(preg);
      besFREE(pmatch);
      besFREE(preg);
      return COMMAND_ERROR_SUCCESS;
      }

    /* copy the string that preceedes the current match into the list */
    if( pmatch->rm_so > iStart ){
      *last = besALLOC(sizeof(struct list));
      if( *last == NULL ){
        ReleaseList(root);
        besRETURNVALUE = NULL;
        regfree(preg);
        besFREE(pmatch);
        besFREE(preg);
        return COMMAND_ERROR_SUCCESS;
        }
      (*last)->next = NULL;
      (*last)->len = pmatch->rm_so-iStart;
      (*last)->string = besALLOC( (*last)->len );
      if( (*last)->string == NULL ){
        ReleaseList(root);
        besRETURNVALUE = NULL;
        regfree(preg);
        besFREE(pmatch);
        besFREE(preg);
        return COMMAND_ERROR_SUCCESS;
        }
      memcpy((*last)->string,STRINGVALUE(vString)+iStart,(*last)->len);
      last = &( (*last)->next );
      }

    /* copy the replace into the list */
    if( i ){
      *last = besALLOC(sizeof(struct list));
      if( *last == NULL ){
        ReleaseList(root);
        besRETURNVALUE = NULL;
        regfree(preg);
        besFREE(pmatch);
        besFREE(preg);
        return COMMAND_ERROR_SUCCESS;
        }
      (*last)->next = NULL;
      (*last)->len = i;
      (*last)->string = besALLOC( (*last)->len );
      if( (*last)->string == NULL ){
        ReleaseList(root);
        besRETURNVALUE = NULL;
        regfree(preg);
        besFREE(pmatch);
        besFREE(preg);
        return COMMAND_ERROR_SUCCESS;
        }
      iError =
      besMatchParameter(STRINGVALUE(vReplace),
                        STRLEN(vReplace),
                        p->pszMatch,
                        p->lMatchLen,
                        (*last)->string,
                        p->lMatches,
                        &((*last)->len));
      last = &( (*last)->next );
      if( iError ){
        ReleaseList(root);
        besRETURNVALUE = NULL;
        regfree(preg);
        besFREE(pmatch);
        besFREE(preg);
        return COMMAND_ERROR_SUCCESS;
        }
      }
    iStart = pmatch->rm_eo;
    iTerm = iStart;
    if( eopt )break;
    }

  if( ((unsigned)iTerm) < STRLEN(vString) ){
    *last = besALLOC(sizeof(struct list));
    if( *last == NULL ){
      ReleaseList(root);
      besRETURNVALUE = NULL;
      regfree(preg);
      besFREE(pmatch);
      besFREE(preg);
      return COMMAND_ERROR_SUCCESS;
      }
    (*last)->next = NULL;
    (*last)->len = STRLEN(vString)-iTerm;
    (*last)->string = besALLOC( (*last)->len );
    if( (*last)->string == NULL ){
      ReleaseList(root);
      besRETURNVALUE = NULL;
      regfree(preg);
      besFREE(pmatch);
      besFREE(preg);
      return COMMAND_ERROR_SUCCESS;
      }
    memcpy((*last)->string,STRINGVALUE(vString)+iTerm,(*last)->len);
    last = &( (*last)->next );
    }

  regfree(preg);
  besFREE(pmatch);
  besFREE(preg);

  i = 0;
  for( item = root ; item ; item = item->next ){
    i += item->len;
    }
  besALLOC_RETURN_STRING(i);
  i = 0;
  for( item = root ; item ; item = item->next ){
    memcpy(STRINGVALUE(besRETURNVALUE)+i,item->string,item->len);
    i += item->len;
    }
  ReleaseList(root);

besEND
