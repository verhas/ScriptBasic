/* 

  FILE   : interface.c
  HEADER : interface.h
  AUTHOR: Peter Verhas

  for the ScriptBasic extension module to access Berkeley DB files.

  v2.0


These lines are needed by the configurator to generate the file 'libraries.jim'
NTLIBS: libdb41s.lib
UXLIBS: -lbdb

*/
#include <sys/types.h>

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <db.h>

#include "../../basext.h"


#define BDB_ERROR_INVALID_FILE_NAME      0x00080001
#define BDB_ERROR_INVALID_DB_HANDLE      0x00080002
#define BDB_ERROR_TRANSACTION_NOT_CLOSED 0x00080003
#define BDB_ERROR_TRANSACTION_NOT_OPENED 0x00080004
#define BDB_ERROR_KEY_NOT_FOUND          0x00080005
#define BDB_INCOMPLETE                   0x00080006
#define BDB_KEYEMPTY                     0x00080007
#define BDB_KEYEXIST                     0x00080008
#define BDB_LOCK_DEADLOCK                0x00080009
#define BDB_LOCK_NOTGRANTED              0x0008000A
#define BDB_NOTFOUND                     0x0008000B
#define BDB_OLD_VERSION                  0x0008000C
#define BDB_RUNRECOVERY                  0x0008000D
#define BDB_DELETED                      0x0008000E
#define BDB_NEEDSPLIT                    0x0008000F
#define BDB_SWAPBYTES                    0x00080010
#define BDB_TXN_CKP                      0x00080011

typedef struct _MyDb {
  DB *pdb;
  DBC *pCursor;
  DB_TXN *txnid;

  struct _MyDb *next,*prev;
  } MyDb, *pMyDb;

/*
 The module object that the module pointer points to. This
stores the "global" variables for the actual interpreter
thread. The name BdbObject stands for Berkeley Data Base Object.
*/
typedef struct _BdbObject {
  DB_ENV *dbenv;
  u_int32_t flags;
  int UnixMode;
  int InTransaction; /* true if we are in a transaction*/
  pMyDb pDbList;
  } BdbObject, *pBdbObject;

static unsigned long ConvertBdbErrors(int BdbError){
  switch( BdbError ){
#ifdef DB_INCOMPLETE
    case DB_INCOMPLETE       : return BDB_INCOMPLETE      ;
#endif
    case DB_KEYEMPTY         : return BDB_KEYEMPTY        ;
    case DB_KEYEXIST         : return BDB_KEYEXIST        ;
    case DB_LOCK_DEADLOCK    : return BDB_LOCK_DEADLOCK   ;
    case DB_LOCK_NOTGRANTED  : return BDB_LOCK_NOTGRANTED ;
    case DB_NOTFOUND         : return BDB_NOTFOUND        ;
    case DB_OLD_VERSION      : return BDB_OLD_VERSION     ;
    case DB_RUNRECOVERY      : return BDB_RUNRECOVERY     ;
    case DB_DELETED          : return BDB_DELETED         ;
    case DB_NEEDSPLIT        : return BDB_NEEDSPLIT       ;
    case DB_SWAPBYTES        : return BDB_SWAPBYTES       ;
    case DB_TXN_CKP          : return BDB_TXN_CKP         ;
    }
  return (unsigned long)BdbError;
  }

besVERSION_NEGOTIATE

  return (int)INTERFACE_VERSION;

besEND

/*
int
db_appinit(char *db_home,
    char * const *db_config, DB_ENV *dbenv, u_int32_t flags);


*/
besSUB_START
  pBdbObject p;
  char *pszConfigBdbHome,*s;
  int iConfigLines,i;

  besMODULEPOINTER = besALLOC(sizeof(BdbObject));
  if( besMODULEPOINTER == NULL )return COMMAND_ERROR_MEMORY_LOW;
  p = (pBdbObject)besMODULEPOINTER;

  p->InTransaction = 0;
  p->pDbList = NULL;

  if( s = besCONFIG("bdb.mode") )
    p->UnixMode = atoi(s);
  else
    p->UnixMode = 0;

#define X(A,B) A = besCONFIG(B);\
               if( A && !*A )A=NULL;\
               if( A )iConfigLines++;

  iConfigLines = 0;
  X(pszConfigBdbHome   ,"bdb.dir.home")

  p->flags = 0;
#undef X
#define X(A,B) if( besCONFIG(B) && !strcmp(besCONFIG(B),"yes") )p->flags |= A;

  X(DB_CREATE          ,"bdb.flags.create")
  X(DB_INIT_CDB        ,"bdb.flags.init_cdb")
  X(DB_INIT_LOCK       ,"bdb.flags.init_lock")
  X(DB_INIT_LOG        ,"bdb.flags.init_log")
  X(DB_INIT_MPOOL      ,"bdb.flags.init_mpool")
  X(DB_INIT_TXN        ,"bdb.flags.init_txn")
  X(DB_PRIVATE         ,"bdb.flags.private")
  X(DB_NOMMAP          ,"bdb.flags.nommap")
  X(DB_RECOVER         ,"bdb.flags.recover")
  X(DB_RECOVER_FATAL   ,"bdb.flags.recover_fatal")
  X(DB_THREAD          ,"bdb.flags.thread")
  X(DB_TXN_NOSYNC      ,"bdb.flags.txn_nosync")
  X(DB_USE_ENVIRON     ,"bdb.flags.use_environ")
  X(DB_USE_ENVIRON_ROOT,"bdb.flags.use_environ_root")
  X(DB_LOCKDOWN        ,"bdb.flags.lockdown")
  X(DB_SYSTEM_MEM      ,"bdb.flags.system_mem")

  p->dbenv = NULL;
  if( i = db_env_create(&(p->dbenv),0) ){
    return i|0x80000000; 
    }

#undef X
#define X(A,B) if( s=besCONFIG(A) )p->dbenv->B = atol(s);
#ifdef DB_INCOMPLETE
  X("bdb.limits.lg_max"     ,lg_max)
#else
  if( s=besCONFIG("bdb.limits.lg_max") )p->dbenv->set_lg_max(p->dbenv,atol(s));
#endif
  X("bdb.limits.mp_mmapsize",mp_mmapsize)
  X("bdb.limits.mp_size"    ,mp_size)
  X("bdb.limits.tx_max"     ,tx_max)
  X("bdb.limits.lk_max"     ,lk_max)

#undef X

  if( s=besCONFIG("bdb.lockstrategy") ){
    if( !strcmp(s,"default" ) )p->dbenv->lk_detect = DB_LOCK_DEFAULT; else
    if( !strcmp(s,"oldest"  ) )p->dbenv->lk_detect = DB_LOCK_OLDEST;  else
    if( !strcmp(s,"random"  ) )p->dbenv->lk_detect = DB_LOCK_RANDOM;  else
    if( !strcmp(s,"youngest") )p->dbenv->lk_detect = DB_LOCK_YOUNGEST;
    }

  if( s = besCONFIG("bdb.dir.data") )
    p->dbenv->set_data_dir(p->dbenv,s);
  if( s = besCONFIG("bdb.dir.log") )
    p->dbenv->set_lg_dir(p->dbenv,s);
  if( s = besCONFIG("bdb.dir.temp") )
    p->dbenv->set_tmp_dir(p->dbenv,s);

  if( i=p->dbenv->open(p->dbenv,pszConfigBdbHome,p->flags,p->UnixMode) ){
    besMODULEPOINTER = NULL;
    besFREE(p);
    return i|0x80000000;
    }

  return 0;
besEND

besSUB_FINISH
  pBdbObject p;
  pMyDb pTable;

  p = (pBdbObject)besMODULEPOINTER;

  if( p == NULL )return 0;

  /* if an unhandled error occured or databse remained open */
  pTable = p->pDbList;
  while( pTable ){
    if( pTable->txnid )txn_abort(pTable->txnid);
    pTable->txnid = NULL;
    if( pTable->pCursor )
      pTable->pCursor->c_close(pTable->pCursor);
    if( pTable->pdb )
      pTable->pdb->close(pTable->pdb,0);
    pTable = pTable->next;
    }

  p->dbenv->close(p->dbenv,0);

  return 0;
besEND

/*

int
db_open(const char *file,
        DBTYPE type,  // DB_BTREE, DB_HASH, DB_RECNO or DB_UNKNOWN (should exist)
        u_int32_t flags, // DB_CREATE, DB_NOMMAP, DB_RDONLY, DB_THREAD, DB_TRUNCATE
        int mode, // unix access mode bits
        DB_ENV *dbenv,
        DB_INFO *dbinfo,
        DB **dbpp);

*/
besFUNCTION(sb_db_open)
  pBdbObject p;
  VARIABLE Argument;
  char *pszFileName;
  long lflags;
  u_int32_t flags;
  DBTYPE type;
  int mode;
  DB *dbpp;
  pMyDb pTable;
  int ret;


  p = (pBdbObject)besMODULEPOINTER;

  /* get the file name */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ){
    besRETURNVALUE = NULL;
    return BDB_ERROR_INVALID_FILE_NAME;
    }
  besCONVERT2ZCHAR(Argument,pszFileName);

  /* get the type */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  besCONVERT2LONG(Argument);
  if( Argument )
    type = (DBTYPE)LONGVALUE(Argument);
  else
    type = 0;
  switch( type ){
    case 1: type = DB_BTREE;   break;
    case 2: type = DB_HASH;    break;
    case 4: type = DB_RECNO;   break;
    case 8: type = DB_QUEUE;   break;
    default:
    case 16: type = DB_UNKNOWN; break;
    }

  /* get the flags */
  Argument = besARGUMENT(3);
  besDEREFERENCE(Argument);
  besCONVERT2LONG(Argument);
  if( Argument )
    lflags = ~ LONGVALUE(Argument);
  else
    lflags = 0;
  flags = 0;
  if( lflags & 0x01 )flags |= DB_CREATE;
  if( lflags & 0x02 )flags |= DB_NOMMAP;
  if( lflags & 0x04 )flags |= DB_RDONLY;
  if( lflags & 0x08 )flags |= DB_THREAD;
  if( lflags & 0x10 )flags |= DB_TRUNCATE;
  if( lflags & 0x20 )flags |= DB_EXCL;

  /* get the file mode */
  Argument = besARGUMENT(4);
  besDEREFERENCE(Argument);
  besCONVERT2LONG(Argument);
  if( Argument )
    mode = (int)LONGVALUE(Argument);
  else
    mode = p->UnixMode;

  ret = db_create(&dbpp,p->dbenv,0);
  if( ret ){
    besFREE(pszFileName);
    besRETURNVALUE = NULL;
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */
    }

  dbpp->set_flags( dbpp,DB_DUP );
  ret = dbpp->open(dbpp,
#ifndef DB_INCOMPLETE
                   NULL,
#endif
                   pszFileName,
                   NULL, /* sub databases are not supported yet */
                   type,
                   flags,
                   mode);
  besFREE(pszFileName);
  if( ret ){
    besRETURNVALUE = NULL;
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */
    }
  pTable = besALLOC(sizeof(MyDb));
  if( pTable == NULL )return COMMAND_ERROR_MEMORY_LOW;

  pTable->next = p->pDbList;
  if( pTable->next )pTable->next->prev = pTable;
  p->pDbList = pTable;
  pTable->prev = NULL;

  if( p->InTransaction ){
    ret = txn_begin(p->dbenv, NULL, &(pTable->txnid),0);
    if( ret )
      return ConvertBdbErrors(ret);
    }else{
    pTable->txnid = NULL;
    }

  pTable->pdb = dbpp;
  ret = dbpp->cursor(pTable->pdb,pTable->txnid,&(pTable->pCursor),0);
  if( ret ){
    besRETURNVALUE = NULL;
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */
    }
  besALLOC_RETURN_STRING(sizeof(pMyDb));
  memcpy(STRINGVALUE(besRETURNVALUE),&pTable,sizeof(DB *));
  return COMMAND_ERROR_SUCCESS;
besEND

/* int
DB->close(DB *db, u_int32_t flags);

*/
besFUNCTION(sb_db_close)
  VARIABLE Argument;
  DB *dbpp;
  int ret;
  pMyDb pTable;
  pBdbObject p;

  p = (pBdbObject)besMODULEPOINTER;

  besRETURNVALUE = NULL;

  /* get the db argument */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ||
      Argument->vType != VTYPE_STRING ||
      STRLEN(Argument) != sizeof(DB*) ){
    return BDB_ERROR_INVALID_DB_HANDLE;
    }
  memcpy(&pTable,STRINGVALUE(Argument),sizeof(DB *));
  dbpp = pTable->pdb;
  if( pTable->pCursor )
    pTable->pCursor->c_close(pTable->pCursor);
  ret = dbpp->close(dbpp,0);

  if( pTable->prev )
    pTable->prev = pTable->next;
  else
    p->pDbList = pTable->next;
  if( pTable->next )pTable->next = pTable->prev;

  /* if a table is closed with open transaction then the transaction is aborted */
  if( pTable->txnid )txn_abort(pTable->txnid);

  besFREE(pTable);
  if( ret )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */
  return COMMAND_ERROR_SUCCESS;
besEND

/*

int
DB->put(DB *db,
    DB_TXN *txnid, DBT *key, DBT *data, u_int32_t flags);

*/
besFUNCTION(sb_db_put)
  pBdbObject p;
  VARIABLE Argument;
  DB *dbpp;
  int ret;
  DBT key,data;
  pMyDb pTable;
  long lflags;
  u_int32_t flags;

  p = (pBdbObject)besMODULEPOINTER;
  besRETURNVALUE = NULL;

  /* get the db argument */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ||
      Argument->vType != VTYPE_STRING ||
      STRLEN(Argument) != sizeof(DB*) ){
    return BDB_ERROR_INVALID_DB_HANDLE;
    }
  memcpy(&pTable,STRINGVALUE(Argument),sizeof(pMyDb));
  dbpp = pTable->pdb;

  memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
  /* get the key */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( Argument == NULL ){
    key.data = "";
    key.size = 0;
    }else{
    Argument = besCONVERT2STRING(Argument);
    key.data = STRINGVALUE(Argument);
    key.size = STRLEN(Argument);
    }

  /* get the data */
  Argument = besARGUMENT(3);
  besDEREFERENCE(Argument);
  if( Argument == NULL ){
    data.data = "";
    data.size = 0;
    }else{
    Argument = besCONVERT2STRING(Argument);
    data.data = STRINGVALUE(Argument);
    data.size = STRLEN(Argument);
    }

  /* get the flags */
  Argument = besARGUMENT(4);
  besDEREFERENCE(Argument);
  besCONVERT2LONG(Argument);
  if( Argument )
    lflags = ~ LONGVALUE(Argument);
  else
    lflags = 0;
  flags = 0;
  if( lflags & 0x01 )flags |= DB_APPEND;
  if( lflags & 0x02 )flags |= DB_NOOVERWRITE;

  /* here we call "put" to update the last read record */
  ret = dbpp->put(dbpp,pTable->txnid,&key,&data,flags);

  if( ret )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  return COMMAND_ERROR_SUCCESS;
besEND

besFUNCTION(sb_db_get)
  pBdbObject p;
  VARIABLE Argument;
  DB *dbpp;
  int ret;
  DBT key,data;
  pMyDb pTable;
  u_int32_t flags;

  flags = DB_SET_RANGE;
  p = (pBdbObject)besMODULEPOINTER;
  besRETURNVALUE = NULL;

  /* get the db argument */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ||
      Argument->vType != VTYPE_STRING ||
      STRLEN(Argument) != sizeof(DB*) ){
    return BDB_ERROR_INVALID_DB_HANDLE;
    }
  memcpy(&pTable,STRINGVALUE(Argument),sizeof(DB *));
  dbpp = pTable->pdb;

  memset(&key, 0, sizeof(DBT));
  /* get the key */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( Argument == NULL ){
    flags = DB_FIRST;
    key.data = "";
    key.size =  0;
    }else{
    Argument = besCONVERT2STRING(Argument);
    key.data = STRINGVALUE(Argument);
    key.size = STRLEN(Argument);
    }

  if( pTable->txnid )flags |= DB_RMW;

	memset(&data, 0, sizeof(DBT));
  data.data = NULL;
  data.ulen = 0;
  data.flags = DB_DBT_USERMEM;

  /* here we call "get" to retrieve the neccessary size. Get should return ENOMEM */
  ret = pTable->pCursor->c_get(pTable->pCursor,&key,&data,flags);
  if( ret == DB_KEYEMPTY || ret == DB_NOTFOUND )return COMMAND_ERROR_SUCCESS;
  if( ret && ret != ENOMEM )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  besALLOC_RETURN_STRING(data.size);
  data.data = STRINGVALUE(besRETURNVALUE);
  data.ulen = STRLEN(besRETURNVALUE);
  ret = pTable->pCursor->c_get(pTable->pCursor,&key,&data,flags);
  if( ret )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  return COMMAND_ERROR_SUCCESS;
besEND

besFUNCTION(sb_db_next)
  pBdbObject p;
  VARIABLE Argument;
  DB *dbpp;
  int ret;
  DBT key,data;
  pMyDb pTable;

  p = (pBdbObject)besMODULEPOINTER;
  besRETURNVALUE = NULL;

  /* get the db argument */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ||
      Argument->vType != VTYPE_STRING ||
      STRLEN(Argument) != sizeof(DB*) ){
    return BDB_ERROR_INVALID_DB_HANDLE;
    }
  memcpy(&pTable,STRINGVALUE(Argument),sizeof(DB *));
  dbpp = pTable->pdb;

  memset(&key, 0, sizeof(DBT));
  /* get the key */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( Argument == NULL ){
    key.data = "";
    key.size =  0;
    }else{
    Argument = besCONVERT2STRING(Argument);
    key.data = STRINGVALUE(Argument);
    key.size = STRLEN(Argument);
    }

	memset(&data, 0, sizeof(DBT));
  data.data = NULL;
  data.ulen = 0;
  data.flags = DB_DBT_USERMEM;

  /* here we call "get" to retrieve the neccessary size. Get should return ENOMEM */
  ret = pTable->pCursor->c_get(pTable->pCursor,&key,&data,DB_NEXT);
  if( ret == DB_KEYEMPTY || ret == DB_NOTFOUND )return COMMAND_ERROR_SUCCESS;
  if( ret && ret != ENOMEM )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  besALLOC_RETURN_STRING(data.size);
  data.data = STRINGVALUE(besRETURNVALUE);
  data.ulen = STRLEN(besRETURNVALUE);
  ret = pTable->pCursor->c_get(pTable->pCursor,&key,&data,DB_NEXT);
  if( ret )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  return COMMAND_ERROR_SUCCESS;
besEND

/* return the key of the current record */
besFUNCTION(sb_db_key)
  pBdbObject p;
  VARIABLE Argument;
  DB *dbpp;
  int ret;
  DBT key,data;
  pMyDb pTable;

  p = (pBdbObject)besMODULEPOINTER;
  besRETURNVALUE = NULL;

  /* get the db argument */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ||
      Argument->vType != VTYPE_STRING ||
      STRLEN(Argument) != sizeof(DB*) ){
    return BDB_ERROR_INVALID_DB_HANDLE;
    }
  memcpy(&pTable,STRINGVALUE(Argument),sizeof(DB *));
  dbpp = pTable->pdb;

  memset(&key, 0, sizeof(DBT));
  key.data = NULL;
  key.ulen =  0;
  key.flags = DB_DBT_USERMEM;

	memset(&data, 0, sizeof(DBT));
  data.data = NULL;
  data.ulen = 0;
  data.flags = DB_DBT_USERMEM;

  /* here we call "get" to retrieve the neccessary size. Get should return ENOMEM */
  ret = pTable->pCursor->c_get(pTable->pCursor,&key,&data,DB_CURRENT);
  if( ret == DB_KEYEMPTY || ret == DB_NOTFOUND )return COMMAND_ERROR_SUCCESS;
  if( ret && ret != ENOMEM )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  besALLOC_RETURN_STRING(key.size);
  key.data = STRINGVALUE(besRETURNVALUE);
  key.ulen = STRLEN(besRETURNVALUE);

  ret = pTable->pCursor->c_get(pTable->pCursor,&key,&data,DB_CURRENT);
  if( ret && ret != ENOMEM )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  return COMMAND_ERROR_SUCCESS;
besEND


besFUNCTION(sb_db_prev)
  pBdbObject p;
  VARIABLE Argument;
  DB *dbpp;
  int ret;
  DBT key,data;
  pMyDb pTable;

  p = (pBdbObject)besMODULEPOINTER;
  besRETURNVALUE = NULL;

  /* get the db argument */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ||
      Argument->vType != VTYPE_STRING ||
      STRLEN(Argument) != sizeof(DB*) ){
    return BDB_ERROR_INVALID_DB_HANDLE;
    }
  memcpy(&pTable,STRINGVALUE(Argument),sizeof(DB *));
  dbpp = pTable->pdb;

  memset(&key, 0, sizeof(DBT));
  /* get the key */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( Argument == NULL ){
    key.data = "";
    key.size =  0;
    }else{
    Argument = besCONVERT2STRING(Argument);
    key.data = STRINGVALUE(Argument);
    key.size = STRLEN(Argument);
    }

	memset(&data, 0, sizeof(DBT));
  data.data = NULL;
  data.ulen = 0;
  data.flags = DB_DBT_USERMEM;

  /* here we call "get" to retrieve the neccessary size. Get should return ENOMEM */
  ret = pTable->pCursor->c_get(pTable->pCursor,&key,&data,DB_PREV);
  if( ret == DB_KEYEMPTY || ret == DB_NOTFOUND )return COMMAND_ERROR_SUCCESS;
  if( ret && ret != ENOMEM )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  besALLOC_RETURN_STRING(data.size);
  data.data = STRINGVALUE(besRETURNVALUE);
  data.ulen = STRLEN(besRETURNVALUE);
  ret = pTable->pCursor->c_get(pTable->pCursor,&key,&data,DB_PREV);
  if( ret )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  return COMMAND_ERROR_SUCCESS;
besEND

besFUNCTION(sb_db_last)
  pBdbObject p;
  VARIABLE Argument;
  DB *dbpp;
  int ret;
  DBT key,data;
  pMyDb pTable;

  p = (pBdbObject)besMODULEPOINTER;
  besRETURNVALUE = NULL;

  /* get the db argument */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ||
      Argument->vType != VTYPE_STRING ||
      STRLEN(Argument) != sizeof(DB*) ){
    return BDB_ERROR_INVALID_DB_HANDLE;
    }
  memcpy(&pTable,STRINGVALUE(Argument),sizeof(DB *));
  dbpp = pTable->pdb;

  memset(&key, 0, sizeof(DBT));

	memset(&data, 0, sizeof(DBT));
  data.data = NULL;
  data.ulen = 0;
  data.flags = DB_DBT_USERMEM;

  /* here we call "get" to retrieve the neccessary size. Get should return ENOMEM */
  ret = pTable->pCursor->c_get(pTable->pCursor,&key,&data,DB_LAST);
  if( ret == DB_KEYEMPTY || ret == DB_NOTFOUND )return COMMAND_ERROR_SUCCESS;
  if( ret && ret != ENOMEM )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  besALLOC_RETURN_STRING(data.size);
  data.data = STRINGVALUE(besRETURNVALUE);
  data.ulen = STRLEN(besRETURNVALUE);
  ret = pTable->pCursor->c_get(pTable->pCursor,&key,&data,DB_LAST);
  if( ret )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  return COMMAND_ERROR_SUCCESS;
besEND

/*
int
DB->del(DB *db, DB_TXN *txnid, DBT *key, u_int32_t flags);
*/
besFUNCTION(sb_db_del)
  pBdbObject p;
  VARIABLE Argument;
  DB *dbpp;
  int ret;
  DBT key;
  pMyDb pTable;

  p = (pBdbObject)besMODULEPOINTER;
  besRETURNVALUE = NULL;

  /* get the db argument */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ||
      Argument->vType != VTYPE_STRING ||
      STRLEN(Argument) != sizeof(DB*) ){
    return BDB_ERROR_INVALID_DB_HANDLE;
    }
  memcpy(&pTable,STRINGVALUE(Argument),sizeof(DB *));
  dbpp = pTable->pdb;

  memset(&key, 0, sizeof(DBT));
  /* get the key */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( Argument == NULL ){
    return COMMAND_ERROR_SUCCESS;
    }else{
    Argument = besCONVERT2STRING(Argument);
    key.data = STRINGVALUE(Argument);
    key.size = STRLEN(Argument);
    }

  ret = dbpp->del(dbpp,pTable->txnid,&key,0);
  if( ret == DB_NOTFOUND )return BDB_ERROR_KEY_NOT_FOUND;
  if( ret == EINVAL )return COMMAND_ERROR_SUCCESS;
  if( ret )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  return COMMAND_ERROR_SUCCESS;
besEND

/*
int
DBcursor->c_del(DBC *cursor, u_int32_t flags);
*/
besFUNCTION(sb_db_eracrec)
  pBdbObject p;
  VARIABLE Argument;
  DB *dbpp;
  int ret;
  pMyDb pTable;

  p = (pBdbObject)besMODULEPOINTER;
  besRETURNVALUE = NULL;

  /* get the db argument */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ||
      Argument->vType != VTYPE_STRING ||
      STRLEN(Argument) != sizeof(DB*) ){
    return BDB_ERROR_INVALID_DB_HANDLE;
    }
  memcpy(&pTable,STRINGVALUE(Argument),sizeof(DB *));
  dbpp = pTable->pdb;

  ret = pTable->pCursor->c_del(pTable->pCursor,0);
  if( ret )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  return COMMAND_ERROR_SUCCESS;
besEND

besFUNCTION(sb_db_update)
  pBdbObject p;
  VARIABLE Argument;
  DB *dbpp;
  int ret;
  DBT key,data;
  pMyDb pTable;

  p = (pBdbObject)besMODULEPOINTER;
  besRETURNVALUE = NULL;

  /* get the db argument */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ||
      Argument->vType != VTYPE_STRING ||
      STRLEN(Argument) != sizeof(DB*) ){
    return BDB_ERROR_INVALID_DB_HANDLE;
    }
  memcpy(&pTable,STRINGVALUE(Argument),sizeof(DB *));
  dbpp = pTable->pdb;

  memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

  /* get the data */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( Argument == NULL ){
    data.data = "";
    data.size = 0;
    }else{
    Argument = besCONVERT2STRING(Argument);
    data.data = STRINGVALUE(Argument);
    data.size = STRLEN(Argument);
    }

  /* here we call "put" to update the last read record */
  ret = pTable->pCursor->c_put(pTable->pCursor,&key,&data,DB_CURRENT);
  if( ret == DB_KEYEMPTY || ret == DB_NOTFOUND )return COMMAND_ERROR_SUCCESS;
  if( ret && ret != ENOMEM )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  if( ret )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  return COMMAND_ERROR_SUCCESS;
besEND

besFUNCTION(sb_db_remove)
  pBdbObject p;
  VARIABLE Argument;
  DB *dbpp;
  int ret;
  char *pszDataBase;

  p = (pBdbObject)besMODULEPOINTER;
  besRETURNVALUE = NULL;

  ret = db_create(&dbpp,p->dbenv,0);
  if( ret )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  /* get the database name */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL ){
    return BDB_ERROR_INVALID_FILE_NAME;
    }else{
    Argument = besCONVERT2STRING(Argument);
    besCONVERT2ZCHAR(Argument,pszDataBase);
    }

  ret = dbpp->remove(dbpp,pszDataBase,NULL,0);
  if( ret )
    return ConvertBdbErrors(ret);/* high bit set is needed to separate from scriba errors */

  return COMMAND_ERROR_SUCCESS;
besEND


/*
int
txn_begin(DB_TXNMGR *txnp, DB_TXN *parent, DB_TXN **tid);

*/
besFUNCTION(sb_db_transact)
  pBdbObject p;
  int ret;
  pMyDb pTable;

  p = (pBdbObject)besMODULEPOINTER;
  besRETURNVALUE = NULL;
  if( p->InTransaction )return BDB_ERROR_TRANSACTION_NOT_CLOSED;

  pTable = p->pDbList;
  while( pTable ){
    ret = txn_begin(p->dbenv, NULL, &(pTable->txnid),0);
    if( ret )
      return ConvertBdbErrors(ret);
    pTable = pTable->next;
    }
  p->InTransaction = 1;
  return COMMAND_ERROR_SUCCESS;
besEND

besFUNCTION(sb_db_trcommit)
  pBdbObject p;
  pMyDb pTable;

  p = (pBdbObject)besMODULEPOINTER;
  besRETURNVALUE = NULL;
  if( ! p->InTransaction )return BDB_ERROR_TRANSACTION_NOT_OPENED;

  pTable = p->pDbList;
  while( pTable ){
    txn_commit(pTable->txnid,0);
    pTable->txnid = NULL;
    if( pTable->pCursor )
      pTable->pCursor->c_close(pTable->pCursor);
    pTable->pCursor = NULL;
    pTable->pdb->cursor(pTable->pdb,pTable->txnid,&(pTable->pCursor),0);
    pTable = pTable->next;
    }
  p->InTransaction = 0;
  return COMMAND_ERROR_SUCCESS;
besEND

besFUNCTION(sb_db_trabort)
  pBdbObject p;
  pMyDb pTable;

  p = (pBdbObject)besMODULEPOINTER;
  besRETURNVALUE = NULL;
  if( ! p->InTransaction )return BDB_ERROR_TRANSACTION_NOT_OPENED;

  pTable = p->pDbList;
  while( pTable ){
    txn_abort(pTable->txnid);
    pTable->txnid = NULL;
    if( pTable->pCursor )
      pTable->pCursor->c_close(pTable->pCursor);
    pTable->pCursor = NULL;
    pTable->pdb->cursor(pTable->pdb,pTable->txnid,&(pTable->pCursor),0);
    pTable = pTable->next;
    }
  p->InTransaction = 0;
  return COMMAND_ERROR_SUCCESS;
besEND
