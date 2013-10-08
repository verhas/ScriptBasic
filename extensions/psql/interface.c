/* FILE:pgsqlinterface.c
 *
 * by pts@fazekas.hu at Wed May  8 17:50:51 CEST 2002
 *
 * This file implements PostgreSQL 7.0 database inteface functionality using
 * the libpq C client library (shipped with the PostgreSQL sources) for the
 * ScriptBasic interpreter as an external module.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

UXLIBS: -lpq
#NTLIBS: psql.lib
 */

/* #define PTS_DEBUG 1 */
#define PTS_MODULE "pgsqlinterf.c"

/* vvv Debian GNU/Linux: /usr/include/postgresql/libpq-fe.h */
#include <postgresql/libpq-fe.h>
#include "../../basext.h"
#include <stdio.h> /* simple debugging */
#include <string.h> /* memcmp() */
#include <stdlib.h> /* free() */

#ifdef PTS_DEBUG
#  define DEBUGMSG(x) x
#  include <assert.h>
#  undef NDEBUG
#else
#  define DEBUGMSG(x)
#  define assert(x)
#  define NDEBUG 1
#endif

#ifndef InvalidOid
#define InvalidOid 0
#endif

typedef enum _TY {
  CON=1,
  RES=2,
} TY;

struct _Wrapper;
typedef struct _Wrapper {
  union {
    PGconn   *con;
    PGresult *res;
  } u;
  struct _Wrapper *next, *prev; /* non-circular double-linked list */
  unsigned long handle; /* stupid scriba interface cannot return it */
  TY ty; /* CON or RES */
} Wrapper, *pWrapper;

typedef struct _ModuleGlobal {
  void *ha; /* HandleArray for PGconn and PGresult */
  Wrapper *first;
  VARIABLE s_keyword, s_envvar, s_compiled, s_val, s_label, s_dispchar, s_dispsize;
} ModuleGlobal, *pModuleGlobal;

/* Left value argument is needed for the command */
#define PGSQL_ERROR_CON_EXPECTED  0x00081001
#define PGSQL_ERROR_RES_EXPECTED  0x00081002
#define PGSQL_ERROR_HAN_EXPECTED  0x00081003
#define PGSQL_ERROR_CONNGET_KEY   0x00081004
#define PGSQL_ERROR_LVAL_EXPECTED 0x00081005
#define PGSQL_ERROR_EST_EXPECTED  0x00081006

#define copystr(dst,src) (\
  ((dst)=besNEWSTRING(STRLEN(src))) && \
  (memcpy(STRINGVALUE(dst),STRINGVALUE(src),STRLEN(src)), 1) \
)
#define copystrz(dst,src) (tmplen=strlen(src), \
  ((dst)=besNEWSTRING(tmplen)) && \
  (memcpy(STRINGVALUE(dst),src,tmplen), 1) \
)
/** like copystrz, but converts NULL -> undef */
#define copystry(dst,src) (src ? (tmplen=strlen(src), \
  ((dst)=besNEWSTRING(tmplen)) && \
  (memcpy(STRINGVALUE(dst),src,tmplen), 1) \
) : ((dst)=NULL, 1) )
#define copylong(dst,src) (\
  ((dst)=besNEWLONG) && \
  (LONGVALUE(dst)=src, 1) \
)
#define streq(var,strz) \
  (STRLEN(var)==strlen(strz) && 0==memcmp(STRINGVALUE(var),strz,STRLEN(var)))
#define strbegins(var,strz) \
  (STRLEN(var)>=strlen(strz) && 0==memcmp(STRINGVALUE(var),strz,strlen(strz)))

static void stderrNoticeProcessor(void * arg, const char * message) {
  (void)arg;
  /* Dat: message already contains \n */
  fprintf(stderr, "PGSQL: %s", message);
}
static void silentNoticeProcessor(void * arg, const char * message) {
  (void)arg;
  (void)message;
}

besSUB_ERRMSG
  (void)pSt; (void)ppModuleInternal;
  /* assert(0); */
  DEBUGMSG(fprintf(stderr, PTS_MODULE": iError=%lx.\n", (long)iError););
  switch (iError) {
   case PGSQL_ERROR_CON_EXPECTED: return "PGSQL error: PGSQL Connection handle expected";
   case PGSQL_ERROR_RES_EXPECTED: return "PGSQL error: PGSQL ResultSet handle expected";
   case PGSQL_ERROR_HAN_EXPECTED: return "PGSQL error: PGSQL handle expected";
   case PGSQL_ERROR_CONNGET_KEY:  return "PGSQL error: invalid key specified for PGconnget()";
   case PGSQL_ERROR_LVAL_EXPECTED:return "PGSQL error: left value expected";
   case PGSQL_ERROR_EST_EXPECTED: return "PGSQL error: ExecStatusType expected";
  }
  return NULL;
  /* return "ERRMSG from "PTS_MODULE; */
besEND

besVERSION_NEGOTIATE
  (void)Version; (void)pszVariation; (void)ppModuleInternal;
  return (int)INTERFACE_VERSION;
besEND

besDLL_MAIN

SUPPORT_MULTITHREAD

besSUB_PROCESS_START
  INIT_MULTITHREAD
  (void)lThreadCounter;
  DEBUGMSG(fprintf(stderr, PTS_MODULE ": Process Hi!\n"););
  return 1;
besEND

besSUB_PROCESS_FINISH
  DEBUGMSG(fprintf(stderr, PTS_MODULE ": Process Bye!\n"););
besEND

/* Imp: ?? */
besSUB_KEEP
  return 0;
besEND

static MUTEX mutex;

besSUB_START
  unsigned long tmplen;
  ModuleGlobal *p;
  (void)pEo; (void)pParameters; (void)pReturnValue;
  INITLOCK /* Imp: ez kell?? */
  besInitMutex(&mutex);
  INITUNLO
  if (NULL==(besMODULEPOINTER=besALLOC(sizeof*p))) return COMMAND_ERROR_MEMORY_LOW;
  p=(ModuleGlobal*)besMODULEPOINTER;
  p->ha=NULL;
  p->first=NULL;
  if (0
   || !copystrz(p->s_keyword , "keyword")
   || !copystrz(p->s_envvar  , "envvar")
   || !copystrz(p->s_compiled, "compiled")
   || !copystrz(p->s_val     , "val")
   || !copystrz(p->s_label   , "label")
   || !copystrz(p->s_dispchar, "dispchar")
   || !copystrz(p->s_dispsize, "dispsize")
     ) return COMMAND_ERROR_MEMORY_LOW;
  DEBUGMSG(fprintf(stderr, PTS_MODULE ": Hi %p!\n", p););
besEND

besSUB_FINISH
  ModuleGlobal *p;
  Wrapper *q;
  (void)pEo; (void)pParameters; (void)pReturnValue;
  DEBUGMSG(fprintf(stderr, PTS_MODULE ": Bye!\n"););
  if (NULL!=(p=(ModuleGlobal*)besMODULEPOINTER)) {
    /* Dat: PGconn and PGresult are independent */
    for (q = p->first ; q ; q = q->next) {
      DEBUGMSG(fprintf(stderr, PTS_MODULE ": Finish %p!\n", q););
      switch (q->ty) {
       case CON: PQfinish(q->u.con); break;
       case RES: PQclear (q->u.res); break;
       default:  assert(0);
      }
    }
    besHandleDestroyHandleArray(p->ha);
  }
  #if 0 /* No need to free these because scriba frees them when the memory segment is freed */
    pSt->ReleaseVariable(pSt->pEo->pMo, p->s_keyword);
    pSt->ReleaseVariable(pSt->pEo->pMo, p->s_envvar);
    pSt->ReleaseVariable(pSt->pEo->pMo, p->s_compiled);
    pSt->ReleaseVariable(pSt->pEo->pMo, p->s_val);
    pSt->ReleaseVariable(pSt->pEo->pMo, p->s_label);
    pSt->ReleaseVariable(pSt->pEo->pMo, p->s_dispchar);
    pSt->ReleaseVariable(pSt->pEo->pMo, p->s_dispsize);
  #endif
  return 0;                                                                     
besEND

/** May return NULL */
static Wrapper *alloc_Wrapper(pSupportTable pSt, ModuleGlobal *mg/*, unsigned long *handle_ret*/) {
  Wrapper *w=besALLOC(sizeof*w);
  if (w!=NULL) {
    DEBUGMSG(fprintf(stderr, PTS_MODULE ": Alloc %p -> %p!\n", mg, w););
    besLockMutex(&mutex);
    if (mg->first!=NULL) mg->first->prev=w;
    w->next=mg->first;
    mg->first=w;
    w->prev=NULL;
    /*if (NULL!=handle_ret) *handle_ret=*/w->handle=besHandleGetHandle(mg->ha, w);
    besUnlockMutex(&mutex);
  }
  return w;
}

/** Also calls PGfinish() or PGclear() */
static void delete_Wrapper(pSupportTable pSt, ModuleGlobal *mg, Wrapper *w) {
  if (w!=NULL) {
    switch (w->ty) {
     case CON: PQfinish(w->u.con); break;
     case RES: PQclear (w->u.res); break;
     default:  assert(0);
    }
    besLockMutex(&mutex);
    if (w->prev!=NULL) w->prev->next=w->next;
                  else mg->first=w->next;
    if (w->next!=NULL) w->next->prev=w->prev;
    assert((w->prev=w->next=NULL, 1)); /* play it safe */
    besHandleFreeHandle(mg->ha, w->handle);
    besUnlockMutex(&mutex);
    DEBUGMSG(fprintf(stderr, PTS_MODULE ": Free %p!\n", w););
    besFREE(w);
  } 
}

besFUNCTION(PGopen)
  Wrapper *w=NULL;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  (void)pEo;
  besDEREFERENCE(Argument);
  if (NULL==Argument) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)==VTYPE_LONG) { /* PQreset() */
    if (NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
     || w->ty!=CON
       ) return PGSQL_ERROR_CON_EXPECTED;
    if (besARGNR>1) return EX_ERROR_TOO_MANY_ARGUMENTS;
    PQreset(w->u.con);
  } else if (TYPE(Argument)==VTYPE_STRING) { /* PQconnectdb() */
    char *specs;
    PGconn *con;
    if (besARGNR>1) return EX_ERROR_TOO_MANY_ARGUMENTS;
    besCONVERT2ZCHAR(Argument,specs); /* Dat: no automatic sentinel in scriba :-( */
    con=PQconnectdb(specs);
    /* assert(con!=NULL);: almost always true */
    besFREE(specs);
    if (!con || PQstatus(con)!=CONNECTION_OK) {
      /* printf("conn err=(%s)\n", PQerrorMessage(con)); */
      specs=con ? PQerrorMessage(con) : "PQconnectdb() returned NULL";
      besALLOC_RETURN_STRING(strlen(specs));
      memcpy(STRINGVALUE(besRETURNVALUE),specs,strlen(specs));
      if (con) PQfinish(con); /* free mem ?? */
      return 0;
      /* return COMMAND_ERROR_CHDIR; */
    }
    if (NULL==(w=alloc_Wrapper(pSt, (ModuleGlobal*)besMODULEPOINTER))) {
      PQfinish(con);
      return COMMAND_ERROR_MEMORY_LOW;
    }
    w->ty=CON; w->u.con=con;
    PQsetNoticeProcessor(con, stderrNoticeProcessor, con);
  }
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE)=(long)w->handle;
  /*return NULL;*/ /*notreached*/
besEND

besFUNCTION(PGclose)
  Wrapper *w;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  (void)pEo;
  besDEREFERENCE(Argument);
  if (besARGNR>1) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument) return EX_ERROR_TOO_FEW_ARGUMENTS;
  /* Dat: automatic type conversion is disabled deliberately (author's taste) */
  if (TYPE(Argument)!=VTYPE_LONG || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument))))
    return PGSQL_ERROR_HAN_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  delete_Wrapper(pSt, (ModuleGlobal*)besMODULEPOINTER, w);
  /* besALLOC_RETURN_LONG;  LONGVALUE(besRETURNVALUE)=0; */
  besRETURNVALUE=NULL; /* return undef */
besEND

static int isSSL(PGconn const*c) {
#ifdef USE_SSL
  return (SSL*)0!=PQgetssl(c);
#else
  (void)c;
  return 0;
#endif
}

besFUNCTION(PGconnget)
  char *s=NULL;
  Wrapper *w;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  VARIABLE Argument2=besARGUMENT(2);
  (void)pEo;
  besDEREFERENCE(Argument);
  besDEREFERENCE(Argument2);
  if (besARGNR>2) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument2) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=CON
     ) return PGSQL_ERROR_CON_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  besCONVERT2STRING(Argument2);
  if (STRLEN(Argument2)==2 && 0==memcmp("db", STRINGVALUE(Argument2), 2)) s=PQdb(w->u.con);
  else if (STRLEN(Argument2)==4 && 0==memcmp("user", STRINGVALUE(Argument2), 4)) s=PQuser(w->u.con);
  else if (STRLEN(Argument2)==4 && 0==memcmp("pass", STRINGVALUE(Argument2), 4)) s=PQpass(w->u.con);
  else if (STRLEN(Argument2)==4 && 0==memcmp("host", STRINGVALUE(Argument2), 4)) s=PQhost(w->u.con);
  else if (STRLEN(Argument2)==4 && 0==memcmp("port", STRINGVALUE(Argument2), 4)) s=PQport(w->u.con);
  else if (STRLEN(Argument2)==3 && 0==memcmp("tty", STRINGVALUE(Argument2), 3)) s=PQtty(w->u.con);
  else if (STRLEN(Argument2)==7 && 0==memcmp("options", STRINGVALUE(Argument2), 7)) s=PQoptions(w->u.con);
  else {
    int pid;
    if (STRLEN(Argument2)==7 && 0==memcmp("SSLused", STRINGVALUE(Argument2), 7)) pid=-isSSL(w->u.con);
    else if (STRLEN(Argument2)==10 && 0==memcmp("backendPID", STRINGVALUE(Argument2), 10)) pid=PQbackendPID(w->u.con);
    else return PGSQL_ERROR_CONNGET_KEY/*COMMAND_ERROR_ARGUMENT_RANGE*/;
    besALLOC_RETURN_LONG;
    LONGVALUE(besRETURNVALUE)=(long)pid;
    return 0;
  }
  besALLOC_RETURN_STRING(strlen(s));
  memcpy(STRINGVALUE(besRETURNVALUE), s, STRLEN(besRETURNVALUE));
besEND

besFUNCTION(PGok)
  /* char *s=NULL; */
  ExecStatusType est;
  Wrapper *w;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  (void)pEo;
  besDEREFERENCE(Argument);
  if (besARGNR>1) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || (w->ty!=CON && w->ty!=RES)
     ) return PGSQL_ERROR_HAN_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE)=-(long)(
    w->ty==CON ? CONNECTION_OK==PQstatus(w->u.con)
               : PGRES_COMMAND_OK==(est=PQresultStatus(w->u.res)) || PGRES_TUPLES_OK==est
  );
besEND

/** argument 1 is result arrayref */
besFUNCTION(PGconndefaults)
  unsigned long tmplen;
  PQconninfoOption *cio, *p, *cioend;
  LEFTVALUE Lval;
  VARIABLE Argument=besARGUMENT(1), v;
  long __refcount_; /* used by besLEFTVALUE */
  (void)pEo;
  (void)ppModuleInternal;
  /* besDEREFERENCE(Argument); */
  /* fprintf(stderr, "%d\n", besARGNR); */
  if (besARGNR>1) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument) return EX_ERROR_TOO_FEW_ARGUMENTS;
  /* assert(0); */
  besLEFTVALUE(Argument,Lval);
  if (!Lval) return PGSQL_ERROR_LVAL_EXPECTED;
  besRELEASE(*Lval); *Lval=NULL;

  cio=PQconndefaults();
  cioend=cio; while (cioend->keyword!=NULL) cioend++;
  if (!(*Lval=besNEWARRAY(0,cioend-cio-1))) { /* min idx, max idx */
   outmem:
    PQconninfoFree(cio);
    return COMMAND_ERROR_MEMORY_LOW;
  }
  for (p=cio; p!=cioend-3; p++) {
    /* Imp: free temp alloced memory, even when out of memory */
    /* vvv Imp: really should make refs to ->s_* */
    if (!(v=ARRAYVALUE(*Lval,p-cio)=besNEWARRAY(0,13))
     || !copystr (ARRAYVALUE(v, 0), ((ModuleGlobal*)besMODULEPOINTER)->s_keyword)
     || !copystrz(ARRAYVALUE(v, 1), p->keyword)
     || !copystr (ARRAYVALUE(v, 2), ((ModuleGlobal*)besMODULEPOINTER)->s_envvar)
     || !copystrz(ARRAYVALUE(v, 3), p->envvar)
     || !copystr (ARRAYVALUE(v, 4), ((ModuleGlobal*)besMODULEPOINTER)->s_compiled)
     || !copystry(ARRAYVALUE(v, 5), p->compiled)
     || !copystr (ARRAYVALUE(v, 6), ((ModuleGlobal*)besMODULEPOINTER)->s_val)
     || !copystry(ARRAYVALUE(v, 7), p->val)
     || !copystr (ARRAYVALUE(v, 8), ((ModuleGlobal*)besMODULEPOINTER)->s_label)
     || !copystrz(ARRAYVALUE(v, 9), p->label)
     || !copystr (ARRAYVALUE(v,10), ((ModuleGlobal*)besMODULEPOINTER)->s_dispchar)
     || !copystrz(ARRAYVALUE(v,11), p->dispchar)
     || !copystr (ARRAYVALUE(v,12), ((ModuleGlobal*)besMODULEPOINTER)->s_dispsize)
     || !copylong(ARRAYVALUE(v,13), p->dispsize)
       ) goto outmem;
  }
  PQconninfoFree(cio);
  besRETURNVALUE=NULL; /* return undef */
besEND

besFUNCTION(PGnotified)
  long __refcount_; /* used by besLEFTVALUE */
  unsigned len;
  Wrapper *w;
  LEFTVALUE Lval;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  VARIABLE Argument2=besARGUMENT(2);
  VARIABLE Argument3=besARGUMENT(3);
  PGnotify *no;
  (void)pEo;
  besDEREFERENCE(Argument);
  /* besDEREFERENCE(Argument2); */
  /* besDEREFERENCE(Argument3); */
  if (besARGNR>3) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument3) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=CON
     ) return PGSQL_ERROR_CON_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;

  if (0==PQconsumeInput(w->u.con)) {
    besRETURNVALUE=NULL; /* return undef */
    return 0;
  }
  if (!(no=PQnotifies(w->u.con))) {
    besALLOC_RETURN_LONG; LONGVALUE(besRETURNVALUE)=(long)0;
    return 0;
    besRETURNVALUE=NULL; /* return undef */
  }
  
  besLEFTVALUE(Argument2,Lval);
  len=0; while (len<NAMEDATALEN && no->relname[len]!='\0') len++;
  *Lval=besNEWSTRING(len);
  memcpy(STRINGVALUE(*Lval), no->relname, len);

  besLEFTVALUE(Argument3,Lval);
  *Lval=besNEWLONG;
  LONGVALUE(*Lval)=no->be_pid;
  
  free(no);
  besALLOC_RETURN_LONG; LONGVALUE(besRETURNVALUE)=(long)-1;
besEND


besFUNCTION(PGdumpNotices)
  /* char *s=NULL; */
  Wrapper *w;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  VARIABLE Argument2=besARGUMENT(2); /* 0! */
  (void)pEo;
  besDEREFERENCE(Argument);
  besDEREFERENCE(Argument2);
  if (besARGNR>2) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument2) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=CON
     ) return PGSQL_ERROR_CON_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  PQsetNoticeProcessor(w->u.con, besGETLONGVALUE(Argument2) ? stderrNoticeProcessor : silentNoticeProcessor, w->u.con);
  besRETURNVALUE=NULL; /* return undef */
besEND


besFUNCTION(PGexec)
  /* char *s=NULL; */
  ExecStatusType est;
  unsigned long sumlen;
  size_t tmp;
  unsigned i, argi;
  char **subs, *query, *dst, *s;
  Wrapper *w;
  PGresult *res;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  (void)pEo;
  besDEREFERENCE(Argument);
  /* if (besARGNR>2) return EX_ERROR_TOO_MANY_ARGUMENTS; */
  if (NULL==Argument) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=CON
     ) return PGSQL_ERROR_CON_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  if (!(subs=besALLOC(besARGNR*sizeof*subs))) return COMMAND_ERROR_MEMORY_LOW;
  /* ^^^ Dat: we allocate a little more */
  for (sumlen=1, i=0, argi=2; argi<=(unsigned)besARGNR; argi++) {
    if ((i&1)==0) { /* unquoted */
      Argument=besCONVERT2STRING(besARGUMENT((int)argi));
      sumlen+=STRLEN(Argument);
      i++;
    } else if (besARGUMENT((int)argi)==NULL/*undef*/ && (int)argi<besARGNR) {
      argi++;
      Argument=besCONVERT2STRING(besARGUMENT((int)argi));
      tmp=STRLEN(Argument);
      if (!(subs[i]=PQescapeBytea(STRINGVALUE(Argument), tmp, &tmp))) return COMMAND_ERROR_MEMORY_LOW;
      /* Dat: SUXX: PQescapeBytea in PostgreSQL 7.2 never returns NULL, but it crashes the process */
      sumlen+=2+strlen(subs[i++]);
    } else {
      Argument=besCONVERT2STRING(besARGUMENT((int)argi));
      if (!(subs[i]=besALLOC(STRLEN(Argument)*2+1))) return COMMAND_ERROR_MEMORY_LOW;
      PQescapeString(subs[i], STRINGVALUE(Argument), STRLEN(Argument));
      sumlen+=2+strlen(subs[i++]);
    }
  }
  /* Dat: this function is binary safe, because PQescape* always return a
   *      null-terminated string
   */
  if (!(query=besALLOC(sumlen))) return COMMAND_ERROR_MEMORY_LOW;
  for (dst=query, i=0, argi=2; (int)argi<=besARGNR; argi++) {
    /* fprintf(stderr, "NOW %u\n", i); */
    if ((i&1)==0) { /* unquoted */
      Argument=besCONVERT2STRING(besARGUMENT((int)argi));
      memcpy(dst, STRINGVALUE(Argument), STRLEN(Argument));
      /* Imp: change \0s ?! */
      dst+=STRLEN(Argument);
      i++;
    } else {
      sumlen=strlen(subs[i]);
      *dst++='\'';
      memcpy(dst, subs[i], sumlen);
      dst+=sumlen;
      *dst++='\'';
      if (besARGUMENT((int)argi)==NULL/*undef*/ && (int)argi<besARGNR) {
        argi++;
        free(subs[i]);
      } else besFREE(subs[i]);
      i++; /* Dat: unsafe inside subs[i] */
    }
  }
  *dst='\0';
  DEBUGMSG(fprintf(stderr, "qry=(%s)\n", query);)
  besFREE(subs);
  res=PQexec(w->u.con, query);
  besFREE(query);
  if (!res) { besRETURNVALUE=NULL; return 0; /* return undef */ }
  /* ^^^ Dat: only on fatal error conditions such as out of memory */
  est=!res ? PGRES_FATAL_ERROR : PQresultStatus(res);
  if (est==PGRES_COMMAND_OK || est==PGRES_TUPLES_OK) {
    if (NULL==(w=alloc_Wrapper(pSt, (ModuleGlobal*)besMODULEPOINTER))) {
      PQclear(res);
      return COMMAND_ERROR_MEMORY_LOW;
    }
    w->ty=RES; w->u.res=res;
    besALLOC_RETURN_LONG; LONGVALUE(besRETURNVALUE)=(long)w->handle;
    return 0;
  }
  s=PQresStatus(est); /* "PGRES_..." */
  dst=res ? PQresultErrorMessage(res) : "PQexec() returned NULL";
  besALLOC_RETURN_STRING(strlen(s)+2+strlen(dst));
  memcpy(STRINGVALUE(besRETURNVALUE), s, strlen(s));
  memcpy(STRINGVALUE(besRETURNVALUE)+strlen(s), ": ", 2);
  memcpy(STRINGVALUE(besRETURNVALUE)+strlen(s)+2, dst, strlen(dst));
  if (res) PQclear(res);
besEND

besFUNCTION(PGresultStatus)
  /* char *s=NULL; */
  ExecStatusType est;
  Wrapper *w;
  char *s, *dst;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  (void)pEo;
  besDEREFERENCE(Argument);
  if (besARGNR>1) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=RES
     ) return PGSQL_ERROR_RES_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;

  assert(NULL!=w->u.res);

  est=!w->u.res ? PGRES_FATAL_ERROR : PQresultStatus(w->u.res);
  if (est==PGRES_COMMAND_OK) {
    besALLOC_RETURN_STRING(1);
    STRINGVALUE(besRETURNVALUE)[0]='-';
  } else if (est==PGRES_TUPLES_OK) {
    besALLOC_RETURN_STRING(0);
  } else {
    s=PQresStatus(est); /* "PGRES_..." */
    dst=w->u.res ? PQresultErrorMessage(w->u.res) : "PQexec() returned NULL";
    besALLOC_RETURN_STRING(strlen(s)+2+strlen(dst));
    memcpy(STRINGVALUE(besRETURNVALUE), s, strlen(s));
    memcpy(STRINGVALUE(besRETURNVALUE)+strlen(s), ": ", 2);
    memcpy(STRINGVALUE(besRETURNVALUE)+strlen(s)+2, dst, strlen(dst));
  }
besEND

besFUNCTION(PGmakeEmptyPGresult)
  ExecStatusType est;
  Wrapper *w;
  PGconn *con=NULL;
  PGresult *res;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  VARIABLE Argument2=besARGUMENT(2);
  (void)pEo;
  besDEREFERENCE(Argument);
  besDEREFERENCE(Argument2);
  if (besARGNR>2) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument2) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (Argument!=NULL) {
    if (TYPE(Argument)!=VTYPE_LONG
     || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
     || w->ty!=CON
       ) return PGSQL_ERROR_CON_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
    con=w->u.con;    
  }
  besCONVERT2STRING(Argument2);

  if (strbegins(Argument2,"PGRES_EMPTY_QUERY")) est=PGRES_EMPTY_QUERY;
  else if (strbegins(Argument2,"PGRES_COMMAND_OK")) est=PGRES_COMMAND_OK;
  else if (strbegins(Argument2,"PGRES_TUPLES_OK")) est=PGRES_TUPLES_OK;
  else if (strbegins(Argument2,"PGRES_COPY_OUT")) est=PGRES_COPY_OUT;
  else if (strbegins(Argument2,"PGRES_COPY_IN")) est=PGRES_COPY_IN;
  else if (strbegins(Argument2,"PGRES_BAD_RESPONSE")) est=PGRES_BAD_RESPONSE;
  else if (strbegins(Argument2,"PGRES_NONFATAL_ERROR")) est=PGRES_NONFATAL_ERROR;
  else if (strbegins(Argument2,"PGRES_FATAL_ERROR")) est=PGRES_FATAL_ERROR;
  else return PGSQL_ERROR_EST_EXPECTED;

  if (!(res=PQmakeEmptyPGresult(con, est))) return COMMAND_ERROR_MEMORY_LOW;
  if (NULL==(w=alloc_Wrapper(pSt, (ModuleGlobal*)besMODULEPOINTER))) {
    PQclear(res);
    return COMMAND_ERROR_MEMORY_LOW;
  }
  w->ty=RES; w->u.res=res;
  besALLOC_RETURN_LONG; LONGVALUE(besRETURNVALUE)=(long)w->handle;
besEND

besFUNCTION(PGoid)
  /* char *s=NULL; */
  Oid oid;
  Wrapper *w;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  (void)pEo;
  besDEREFERENCE(Argument);
  if (besARGNR>1) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=RES
     ) return PGSQL_ERROR_RES_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  if (InvalidOid==(oid=PQoidValue(w->u.res))) {
    besRETURNVALUE=NULL;
  } else {
    besALLOC_RETURN_LONG; LONGVALUE(besRETURNVALUE)=(long)oid;
  }
besEND

besFUNCTION(PGescapeString)
  char *s;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  (void)pEo;
  (void)ppModuleInternal;
  besDEREFERENCE(Argument);
  if (besARGNR>1) return EX_ERROR_TOO_MANY_ARGUMENTS;
  besCONVERT2STRING(Argument);
  s=besALLOC(2*STRLEN(Argument)+1);
  PQescapeString(s, STRINGVALUE(Argument), STRLEN(Argument));
  besALLOC_RETURN_STRING(strlen(s));
  memcpy(STRINGVALUE(besRETURNVALUE), s, STRLEN(besRETURNVALUE));
  besFREE(s);
besEND

besFUNCTION(PGescapeBytea)
  char *s;
  size_t tmp;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  (void)pEo;
  (void)ppModuleInternal;
  besDEREFERENCE(Argument);
  if (besARGNR>1) return EX_ERROR_TOO_MANY_ARGUMENTS;
  besCONVERT2STRING(Argument);
  tmp=STRLEN(Argument);
  if (!(s=PQescapeBytea(STRINGVALUE(Argument), tmp, &tmp))) return COMMAND_ERROR_MEMORY_LOW;
  /* ^^^ Dat: SUXX: PQescapeBytea in PostgreSQL 7.2 never returns NULL, but it crashes the process */
  tmp=strlen(s);
  besALLOC_RETURN_STRING(tmp);
  memcpy(STRINGVALUE(besRETURNVALUE), s, STRLEN(besRETURNVALUE));
  free(s);
besEND

besFUNCTION(PGnrows)
  /* char *s=NULL; */
  Wrapper *w;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  (void)pEo;
  besDEREFERENCE(Argument);
  if (besARGNR>1) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=RES
     ) return PGSQL_ERROR_RES_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE)=PQntuples(w->u.res);
besEND

besFUNCTION(PGncols)
  /* char *s=NULL; */
  Wrapper *w;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  (void)pEo;
  besDEREFERENCE(Argument);
  if (besARGNR>1) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=RES
     ) return PGSQL_ERROR_RES_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE)=PQnfields(w->u.res);
besEND

besFUNCTION(PGcol)
  Wrapper *w;
  char *s;
  size_t tmp;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  VARIABLE Argument2=besARGUMENT(2);
  (void)pEo;
  besDEREFERENCE(Argument);
  besDEREFERENCE(Argument2);
  if (besARGNR>2) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument2) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=RES
     ) return PGSQL_ERROR_RES_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  if (TYPE(Argument2)==VTYPE_LONG) {
    if (NULL==(s=PQfname(w->u.res, LONGVALUE(Argument2)))) {
      besRETURNVALUE=NULL;
    } else {
      tmp=strlen(s);
      besALLOC_RETURN_STRING(tmp);
      memcpy(STRINGVALUE(besRETURNVALUE), s, STRLEN(besRETURNVALUE));
    }
  } else {
    int idx;
    besCONVERT2ZCHAR(besCONVERT2STRING(Argument2),s); /* Dat: no automatic sentinel in scriba :-( */
    idx=PQfnumber(w->u.res, s);
    besFREE(s);
    if (idx<0) {
      besRETURNVALUE=NULL;
    } else {
      besALLOC_RETURN_LONG; LONGVALUE(besRETURNVALUE)=(long)idx;
    }
  }
besEND

besFUNCTION(PGcoltype)
  Oid oid;
  Wrapper *w;
  int idx=-1;
  char *s=NULL;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  VARIABLE Argument2=besARGUMENT(2);
  (void)pEo;
  besDEREFERENCE(Argument);
  besDEREFERENCE(Argument2);
  if (besARGNR>2) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument2) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=RES
     ) return PGSQL_ERROR_RES_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  if (TYPE(Argument2)!=VTYPE_LONG) {
    besCONVERT2ZCHAR(besCONVERT2STRING(Argument2),s); /* Dat: no automatic sentinel in scriba :-( */
    idx=PQfnumber(w->u.res, s);
    if (idx<0) { besFREE(s); besRETURNVALUE=NULL; return 0; }
  } else idx=LONGVALUE(Argument2);
  if (InvalidOid==(oid=PQftype(w->u.res, idx))) {
    besRETURNVALUE=NULL; /* should really not hapen */
  } else {
    besALLOC_RETURN_LONG; LONGVALUE(besRETURNVALUE)=(long)oid;
  }
  if (s) besFREE(s);
besEND

besFUNCTION(PGcolmod)
  Oid oid;
  Wrapper *w;
  int idx=-1;
  char *s=NULL;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  VARIABLE Argument2=besARGUMENT(2);
  (void)pEo;
  besDEREFERENCE(Argument);
  besDEREFERENCE(Argument2);
  if (besARGNR>2) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument2) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=RES
     ) return PGSQL_ERROR_RES_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  if (TYPE(Argument2)!=VTYPE_LONG) {
    besCONVERT2ZCHAR(besCONVERT2STRING(Argument2),s); /* Dat: no automatic sentinel in scriba :-( */
    idx=PQfnumber(w->u.res, s);
    if (idx<0) { besFREE(s); besRETURNVALUE=NULL; return 0; }
  } else idx=LONGVALUE(Argument2);
  if (InvalidOid==(oid=PQfmod(w->u.res, idx))) {
    besRETURNVALUE=NULL; /* should really not hapen */
  } else {
    besALLOC_RETURN_LONG; LONGVALUE(besRETURNVALUE)=(long)oid;
  }
  if (s) besFREE(s);
besEND

besFUNCTION(PGcolsize)
  Oid oid;
  Wrapper *w;
  int idx=-1;
  char *s=NULL;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  VARIABLE Argument2=besARGUMENT(2);
  (void)pEo;
  besDEREFERENCE(Argument);
  besDEREFERENCE(Argument2);
  if (besARGNR>2) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument2) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=RES
     ) return PGSQL_ERROR_RES_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  if (TYPE(Argument2)!=VTYPE_LONG) {
    besCONVERT2ZCHAR(besCONVERT2STRING(Argument2),s); /* Dat: no automatic sentinel in scriba :-( */
    DEBUGMSG(fprintf(stderr, "colname=(%s)\n", s);)
    idx=PQfnumber(w->u.res, s);
    if (idx<0) { besFREE(s); besRETURNVALUE=NULL; return 0; }
    DEBUGMSG(fprintf(stderr, "colidx=(%d)\n", idx);)
  } else idx=LONGVALUE(Argument2);
  if (InvalidOid==(oid=PQfsize(w->u.res, idx))) {
    besRETURNVALUE=NULL; /* should really not hapen */
  } else {
    besALLOC_RETURN_LONG; LONGVALUE(besRETURNVALUE)=(long)oid;
  }
  if (s) besFREE(s);
besEND

besFUNCTION(PGgetvalue)
  Wrapper *w;
  int idx=-1;
  size_t tmp;
  char *s=NULL, *v;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  VARIABLE Argument2=besARGUMENT(2);
  VARIABLE Argument3=besARGUMENT(3);
  (void)pEo;
  besDEREFERENCE(Argument);
  besDEREFERENCE(Argument2);
  besDEREFERENCE(Argument3);
  if (besARGNR>3) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument3) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=RES
     ) return PGSQL_ERROR_RES_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  if (TYPE(Argument3)!=VTYPE_LONG) {
    besCONVERT2ZCHAR(besCONVERT2STRING(Argument2),s); /* Dat: no automatic sentinel in scriba :-( */
    idx=PQfnumber(w->u.res, s);
    if (idx<0) { besFREE(s); besRETURNVALUE=NULL; return 0; }
  } else idx=LONGVALUE(Argument3);
  besCONVERT2LONG(Argument2);
  /* Dat: memory returned in v is managed by libpq */
  if (!(v=PQgetvalue(w->u.res, LONGVALUE(Argument2), idx))
    || PQgetisnull(w->u.res, LONGVALUE(Argument2), idx)>0) {
    besRETURNVALUE=NULL; /* invalid field index? */
  } else {
    tmp=strlen(v);
    besALLOC_RETURN_STRING(tmp);
    memcpy(STRINGVALUE(besRETURNVALUE), v, STRLEN(besRETURNVALUE));
  }
  if (s) besFREE(s);
besEND

besFUNCTION(PGgetlength)
  Wrapper *w;
  int idx=-1;
  long len;
  char *s=NULL;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  VARIABLE Argument2=besARGUMENT(2);
  VARIABLE Argument3=besARGUMENT(3);
  (void)pEo;
  besDEREFERENCE(Argument);
  besDEREFERENCE(Argument2);
  besDEREFERENCE(Argument3);
  if (besARGNR>3) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument3) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=RES
     ) return PGSQL_ERROR_RES_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  if (TYPE(Argument3)!=VTYPE_LONG) {
    besCONVERT2ZCHAR(besCONVERT2STRING(Argument2),s); /* Dat: no automatic sentinel in scriba :-( */
    idx=PQfnumber(w->u.res, s);
    if (idx<0) { besFREE(s); besRETURNVALUE=NULL; return 0; }
  } else idx=LONGVALUE(Argument3);
  besCONVERT2LONG(Argument2);
  if (0>(len=PQgetlength(w->u.res, LONGVALUE(Argument2), idx))) {
    besRETURNVALUE=NULL; /* should really not hapen */
  } else {
    besALLOC_RETURN_LONG; LONGVALUE(besRETURNVALUE)=len;
  }
  if (s) besFREE(s);
besEND

besFUNCTION(PGgetisnull)
  Wrapper *w;
  int idx=-1;
  long len;
  char *s=NULL;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  VARIABLE Argument2=besARGUMENT(2);
  VARIABLE Argument3=besARGUMENT(3);
  (void)pEo;
  besDEREFERENCE(Argument);
  besDEREFERENCE(Argument2);
  besDEREFERENCE(Argument3);
  if (besARGNR>3) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument3) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=RES
     ) return PGSQL_ERROR_RES_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  if (TYPE(Argument3)!=VTYPE_LONG) {
    besCONVERT2ZCHAR(besCONVERT2STRING(Argument2),s); /* Dat: no automatic sentinel in scriba :-( */
    idx=PQfnumber(w->u.res, s);
    if (idx<0) { besFREE(s); besRETURNVALUE=NULL; return 0; }
  } else idx=LONGVALUE(Argument3);
  besCONVERT2LONG(Argument2);
  if (0>(len=PQgetisnull(w->u.res, LONGVALUE(Argument2), idx))) {
    besRETURNVALUE=NULL; /* should really not hapen */
  } else {
    besALLOC_RETURN_LONG; LONGVALUE(besRETURNVALUE)=len;
  }
  if (s) besFREE(s);
besEND

besFUNCTION(PGbinaryTuples)
  /* char *s=NULL; */
  Wrapper *w;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  (void)pEo;
  besDEREFERENCE(Argument);
  if (besARGNR>1) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=RES
     ) return PGSQL_ERROR_RES_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE)=PQbinaryTuples(w->u.res) ? -1 : 0;
besEND

besFUNCTION(PGcmdStatus)
  char *s;
  size_t tmp;
  Wrapper *w;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  (void)pEo;
  besDEREFERENCE(Argument);
  if (besARGNR>1) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=RES
     ) return PGSQL_ERROR_RES_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  besALLOC_RETURN_LONG;
  if (!(s=PQcmdStatus(w->u.res))) { besRETURNVALUE=NULL; return 0; }
  tmp=strlen(s);
  besALLOC_RETURN_STRING(tmp);
  memcpy(STRINGVALUE(besRETURNVALUE), s, STRLEN(besRETURNVALUE));
besEND

besFUNCTION(PGcmdTuples)
  char *s;
  size_t tmp;
  Wrapper *w;
  VARIABLE Argument=besARGUMENT(1); /* 0! */
  (void)pEo;
  besDEREFERENCE(Argument);
  if (besARGNR>1) return EX_ERROR_TOO_MANY_ARGUMENTS;
  if (NULL==Argument) return EX_ERROR_TOO_FEW_ARGUMENTS;
  if (TYPE(Argument)!=VTYPE_LONG
   || NULL==(w=besHandleGetPointer(((ModuleGlobal*)besMODULEPOINTER)->ha, besGETLONGVALUE(Argument)))
   || w->ty!=RES
     ) return PGSQL_ERROR_RES_EXPECTED/*COMMAND_ERROR_ARGUMENT_RANGE*/;
  besALLOC_RETURN_LONG;
  if (!(s=PQcmdTuples(w->u.res))) { besRETURNVALUE=NULL; return 0; }
  tmp=strlen(s);
  besALLOC_RETURN_STRING(tmp);
  memcpy(STRINGVALUE(besRETURNVALUE), s, STRLEN(besRETURNVALUE));
  besRETURNVALUE=besCONVERT2LONG(besRETURNVALUE);
besEND


/* __END__ */
