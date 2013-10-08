/* mysqlinterf.c interface to MySQL for ScriptBasic

NTLIBS: mysqlclient.lib ws2_32.lib advapi32.lib
UXLIBS: -lmysqlclient

*/
#if WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>

#include <mysql/mysql.h>
#include <mysql/errmsg.h>

/* set this macro to 1 if you use MySQL 3.23.29 or newer 
   set it to 0 if you want to compile this interface for an older version of MySQL
*/
#ifndef VERSION323
#define VERSION323 1
#endif

#include "../../basext.h"

typedef struct _mymysqlHANDLE {
  MYSQL *hSQL;
  MYSQL_RES *result;
  int num_fields;
  struct _mymysqlHANDLE *next,*prev;
  } mymysqlHANDLE, *pmymysqlHANDLE;

typedef struct _myOBJECT {
  void *HandleArray;
  pmymysqlHANDLE first;
  } myOBJECT, *pmyOBJECT;

static int err_conv_table[] = {
  CR_CONN_HOST_ERROR,
  CR_CONNECTION_ERROR,
  CR_IPSOCK_ERROR,
  CR_OUT_OF_MEMORY,
  CR_SOCKET_CREATE_ERROR,
  CR_UNKNOWN_HOST,
  CR_VERSION_ERROR,
  0
  };

#define MAXERR 7

static char * (errtxt[]) = {
  "Connection to host error",
  "Connection error",
  "IP socket error",
  "Out of memory error",
  "Socket create error",
  "Unknown host error",
  "Version error",
  NULL
  };

static int convert_error(int iMyError){
  int i;

  for( i=0 ; err_conv_table[i] ; i++ )
    if( err_conv_table[i] == iMyError )return 0x00080002+i;
  return 0x00080001;
  }

/* Left value argument is needed for the command */
#define MYSQL_ERROR_LVAL 0x00081001
/* Data seek without alive result set. */
#define MYSQL_ERROR_SEEK 0x00081002
/* Data seek offset is out of range */
#define MYSQL_ERROR_SEKO 0x00081003
/* fecth without a valid result set */
#define MYSQL_ERROR_NORS 0x00081004
/* no defined connection name or connection is not defined */
#define MYSQL_ERROR_NOCN 0x00081005
/* the connectionname is too long */
#define MYSQL_ERROR_BDCN 0x00081006

besSUB_ERRMSG
  if( iError < 0x80000+MAXERR && iError > 0x80000 )
    return errtxt[iError-0x80000];
  else
    return NULL;
besEND

besVERSION_NEGOTIATE
  return (int)INTERFACE_VERSION;
besEND


besDLL_MAIN

besSUB_PROCESS_START
besEND

besSUB_PROCESS_FINISH
besEND

besSUB_START
  pmyOBJECT p;

  besMODULEPOINTER = besALLOC(sizeof(myOBJECT));
  if( besMODULEPOINTER == NULL )return COMMAND_ERROR_MEMORY_LOW;
  p = (pmyOBJECT)besMODULEPOINTER;
  p->first = NULL; /* list of opened mySQL myhandles */
  p->HandleArray = NULL;
  return 0;
besEND

besSUB_FINISH
  pmyOBJECT p;
  pmymysqlHANDLE q;

  p = (pmyOBJECT)besMODULEPOINTER;
  if( p != NULL ){
    for( q = p->first ; q ; q = q->next ){
      mysql_close(q->hSQL);
      }
    besHandleDestroyHandleArray(p->HandleArray);
    }
  return 0;
besEND

#define GET_DB_HANDLE \
  p = (pmyOBJECT)besMODULEPOINTER;\
  Argument = besARGUMENT(1);\
  besDEREFERENCE(Argument);\
  if( Argument == NULL )return EX_ERROR_TOO_FEW_ARGUMENTS;\
  q = besHandleGetPointer(p->HandleArray,besGETLONGVALUE(Argument));\
  if( q == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;



besFUNCTION(mys_affected_rows)
  VARIABLE Argument;
  pmymysqlHANDLE q;
  pmyOBJECT p;

  GET_DB_HANDLE

  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = (long)mysql_affected_rows(q->hSQL);
besEND

besFUNCTION(mys_get_client_info)
  char *pszClientInfo;

  pszClientInfo = mysql_get_client_info();
  if( pszClientInfo ){
    besALLOC_RETURN_STRING(strlen(pszClientInfo));
    memcpy(STRINGVALUE(besRETURNVALUE),pszClientInfo,STRLEN(besRETURNVALUE));
    }else besRETURNVALUE = NULL;
besEND

besFUNCTION(mys_get_host_info)
  VARIABLE Argument;
  char *pszHostInfo;
  pmymysqlHANDLE q;
  pmyOBJECT p;

  GET_DB_HANDLE

  pszHostInfo = mysql_get_host_info(q->hSQL);
  if( pszHostInfo ){
    besALLOC_RETURN_STRING(strlen(pszHostInfo));
    memcpy(STRINGVALUE(besRETURNVALUE),pszHostInfo,STRLEN(besRETURNVALUE));
    }else besRETURNVALUE = NULL;
besEND

besFUNCTION(mys_get_server_info)
  VARIABLE Argument;
  char *pszServerInfo;
  pmymysqlHANDLE q;
  pmyOBJECT p;

  GET_DB_HANDLE

  pszServerInfo = mysql_get_server_info(q->hSQL);
  if( pszServerInfo ){
    besALLOC_RETURN_STRING(strlen(pszServerInfo));
    memcpy(STRINGVALUE(besRETURNVALUE),pszServerInfo,STRLEN(besRETURNVALUE));
    }else besRETURNVALUE = NULL;
besEND

besFUNCTION(mys_stat)
  VARIABLE Argument;
  char *pszStat;
  pmymysqlHANDLE q;
  pmyOBJECT p;

  GET_DB_HANDLE

  pszStat = mysql_stat(q->hSQL);
  if( pszStat ){
    besALLOC_RETURN_STRING(strlen(pszStat));
    memcpy(STRINGVALUE(besRETURNVALUE),pszStat,STRLEN(besRETURNVALUE));
    }else besRETURNVALUE = NULL;
besEND

besFUNCTION(mys_thread_id)
  VARIABLE Argument;
  pmymysqlHANDLE q;
  pmyOBJECT p;

  GET_DB_HANDLE

  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = 0; //mysql_thread_id(q->hSQL);
besEND


besFUNCTION(mys_info)
  VARIABLE Argument;
  char *pszInfo;
  pmymysqlHANDLE q;
  pmyOBJECT p;

  GET_DB_HANDLE

  pszInfo = NULL; //mysql_info(q->hSQL);
  if( pszInfo ){
    besALLOC_RETURN_STRING(strlen(pszInfo));
    memcpy(STRINGVALUE(besRETURNVALUE),pszInfo,STRLEN(besRETURNVALUE));
    }else besRETURNVALUE = NULL;
besEND

besFUNCTION(mys_get_proto_info)
  VARIABLE Argument;
  pmymysqlHANDLE q;
  unsigned int uProtoInfo;
  pmyOBJECT p;

  GET_DB_HANDLE

  uProtoInfo  = mysql_get_proto_info(q->hSQL);
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = uProtoInfo;
besEND

besFUNCTION(mys_insert_id)
  VARIABLE Argument;
  pmymysqlHANDLE q;
  unsigned int uInsertId;
  pmyOBJECT p;

  GET_DB_HANDLE

  uInsertId  = 0;//(unsigned long)mysql_insert_id(q->hSQL);
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = uInsertId;
besEND

besFUNCTION(mys_kill)
  VARIABLE Argument;
  pmymysqlHANDLE q;
  unsigned long pid;
  pmyOBJECT p;

  GET_DB_HANDLE

  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( ! Argument )return EX_ERROR_TOO_FEW_ARGUMENTS;
  Argument = besCONVERT2LONG(Argument);
  pid = LONGVALUE(Argument);

  mysql_kill(q->hSQL,pid);

  besRETURNVALUE= NULL;
besEND

besFUNCTION(mys_error)
  VARIABLE Argument;
  pmymysqlHANDLE q;
  char *pszError;
  pmyOBJECT p;

  GET_DB_HANDLE

  pszError = mysql_error(q->hSQL);

  besALLOC_RETURN_STRING(strlen(pszError));
  memcpy(STRINGVALUE(besRETURNVALUE),pszError,STRLEN(besRETURNVALUE));
besEND

besFUNCTION(mys_ping)
  VARIABLE Argument;
  pmymysqlHANDLE q;
  pmyOBJECT p;

  GET_DB_HANDLE

  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = mysql_ping(q->hSQL) ? 0 : -1;

besEND

besFUNCTION(mys_real_escape_string)
  VARIABLE Argument;
  pmymysqlHANDLE q;
  char *pszTo;
  unsigned long lLen;
  pmyOBJECT p;

  GET_DB_HANDLE

  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( ! Argument )return EX_ERROR_TOO_FEW_ARGUMENTS;
  Argument = besCONVERT2STRING(Argument);

  pszTo = besALLOC( 2*STRLEN(Argument) + 1 );
  if( pszTo == NULL )return COMMAND_ERROR_MEMORY_LOW;
#if VERSION323
  lLen = mysql_real_escape_string(q->hSQL,pszTo,STRINGVALUE(Argument),STRLEN(Argument));

  besALLOC_RETURN_STRING(lLen);
  memcpy(STRINGVALUE(besRETURNVALUE),pszTo,STRLEN(besRETURNVALUE));
  besFREE(pszTo);
#else
  besRETURNVALUE = NULL;
#endif

besEND

besFUNCTION(mys_select_db)
  VARIABLE Argument;
  pmymysqlHANDLE q;
  char *pszDb;
  pmyOBJECT p;

  GET_DB_HANDLE

  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( ! Argument )return EX_ERROR_TOO_FEW_ARGUMENTS;
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszDb);

  mysql_select_db(q->hSQL,pszDb);
  besFREE(pszDb);
  besRETURNVALUE = NULL;
besEND

besFUNCTION(mys_shutdown)
  VARIABLE Argument;
  pmymysqlHANDLE q;
  pmyOBJECT p;

  GET_DB_HANDLE

  mysql_shutdown(q->hSQL);
  besRETURNVALUE = NULL;
besEND

besFUNCTION(mys_data_seek)
  VARIABLE Argument;
  pmymysqlHANDLE q;
  unsigned long lOffset,lLimit;
  pmyOBJECT p;

  GET_DB_HANDLE

  besRETURNVALUE = NULL;
  Argument = besARGUMENT(2);
  if( Argument == NULL ){
    lOffset = 0L;
    }else{
    Argument = besCONVERT2LONG(Argument);
    lOffset = LONGVALUE(Argument);
    }

  if( q->result ){
    lLimit = (unsigned long)mysql_num_rows(q->result);
    if( 0 <= lOffset && lOffset < lLimit )
      mysql_data_seek(q->result,(my_ulonglong)lOffset);
    else
      return MYSQL_ERROR_SEKO;
    }
  else
    return MYSQL_ERROR_SEEK;

besEND

besFUNCTION(mys_change_user)
  VARIABLE Argument;
  pmymysqlHANDLE q;
  char *pszUser,*pszPassword,*pszDB;
  int iError;
  pmyOBJECT p;

  GET_DB_HANDLE

  /* Get the user */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( Argument ){
    besCONVERT2ZCHAR(Argument,pszUser);
  }else pszUser = NULL;


  /* Get the password */
  Argument = besARGUMENT(3);
  besDEREFERENCE(Argument);
  if( Argument ){
    besCONVERT2ZCHAR(Argument,pszPassword);
  }else pszPassword = NULL;

  /* Get the dbname */
  Argument = besARGUMENT(4);
  besDEREFERENCE(Argument);
  if( Argument ){
    besCONVERT2ZCHAR(Argument,pszDB);
  }else pszDB = NULL;

#if VERSION323
  iError = mysql_change_user(q->hSQL,pszUser,pszPassword,pszDB);
#else
  iError = -1;
#endif

  if( pszUser       )besFREE(pszUser);
  if( pszPassword   )besFREE(pszPassword);
  if( pszDB         )besFREE(pszDB);

  besRETURNVALUE = NULL;
  if( iError )return convert_error(iError); else return COMMAND_ERROR_SUCCESS;
besEND

besFUNCTION(mys_character_set_name)
  VARIABLE Argument;
  pmymysqlHANDLE q;
  const char *pszCharacterSetName;
  pmyOBJECT p;

  GET_DB_HANDLE

#if VERSION323
  pszCharacterSetName = mysql_character_set_name(q->hSQL);

  besALLOC_RETURN_STRING(strlen(pszCharacterSetName));

  memcpy(STRINGVALUE(besRETURNVALUE),pszCharacterSetName,STRLEN(besRETURNVALUE));
#else
  besRETURNVALUE = NULL;
#endif

besEND

besFUNCTION(mys_fetcharray)
  VARIABLE Argument;
  LEFTVALUE Lval;
  pmymysqlHANDLE q;
  int i;
  unsigned long *lengths;
  MYSQL_ROW row;
  unsigned int numfields;
  unsigned long __refcount_;
  pmyOBJECT p;

  GET_DB_HANDLE

  besRETURNVALUE = NULL;
  /* db , array */
  if( besARGNR < 2 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  if( q->result == NULL )return MYSQL_ERROR_NORS;

  Argument = besARGUMENT(2);

  besLEFTVALUE(Argument,Lval);
  if( ! Lval )return MYSQL_ERROR_LVAL;

  besRELEASE(*Lval);
  *Lval = NULL; /* to be safe it is undef until we fill in some new value */

  /* get the number of fields in the result */
  numfields = mysql_num_fields(q->result);
  if( numfields == 0 ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

  *Lval = besNEWARRAY(0,numfields-1);
  if( *Lval == NULL )return COMMAND_ERROR_MEMORY_LOW;

  /* get the actual data of the row */
  row = mysql_fetch_row(q->result);
  if( row == NULL ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

  lengths = mysql_fetch_lengths(q->result);
  if( lengths == NULL ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

  for( i= 0 ; ((unsigned)i) < numfields ; i++ ){
    ARRAYVALUE(*Lval,i) = besNEWSTRING(lengths[i]);
    if( ARRAYVALUE(*Lval,i) == NULL )return COMMAND_ERROR_MEMORY_LOW;
    memcpy(STRINGVALUE(ARRAYVALUE(*Lval,i)),row[i],lengths[i]);
    }
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = -1;
besEND

besFUNCTION(mys_fetchhash)
  VARIABLE Argument;
  LEFTVALUE Lval;
  pmymysqlHANDLE q;
  int i;
  unsigned long *lengths;
  MYSQL_ROW row;
  MYSQL_FIELD *field;
  unsigned int numfields;
  unsigned long __refcount_;
  pmyOBJECT p;

  GET_DB_HANDLE

  besRETURNVALUE = NULL;
  /* db , array */
  if( besARGNR < 2 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  if( q->result == NULL )return MYSQL_ERROR_NORS;

  Argument = besARGUMENT(2);

  besLEFTVALUE(Argument,Lval);
  if( ! Lval )return MYSQL_ERROR_LVAL;

  besRELEASE(*Lval);
  *Lval = NULL; /* to be safe it is undef until we fill in some new value */

  /* get the number of fields in the result */
  numfields = mysql_num_fields(q->result);
  if( numfields == 0 ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

  *Lval = besNEWARRAY(0,2*numfields-1);
  if( *Lval == NULL )return COMMAND_ERROR_MEMORY_LOW;

  field = mysql_fetch_fields(q->result);
  if( field == NULL ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

  /* get the actual data of the row */
  row = mysql_fetch_row(q->result);
  if( row == NULL ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

  lengths = mysql_fetch_lengths(q->result);
  if( lengths == NULL ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

  for( i= 0 ; ((unsigned)i) < numfields ; i++ ){
    ARRAYVALUE(*Lval,2*i) = besNEWSTRING(strlen(field[i].name));
    if( ARRAYVALUE(*Lval,2*i) == NULL )return COMMAND_ERROR_MEMORY_LOW;
    memcpy(STRINGVALUE(ARRAYVALUE(*Lval,2*i)),field[i].name,
                                         STRLEN(ARRAYVALUE(*Lval,2*i)));

    ARRAYVALUE(*Lval,2*i+1) = besNEWSTRING(lengths[i]);
    if( ARRAYVALUE(*Lval,2*i+1) == NULL )return COMMAND_ERROR_MEMORY_LOW;
    memcpy(STRINGVALUE(ARRAYVALUE(*Lval,2*i+1)),row[i],lengths[i]);
    }
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = -1;
besEND

besFUNCTION(mys_query)
  VARIABLE Argument;
  pmymysqlHANDLE q;
  int iMyError;
  pmyOBJECT p;

  GET_DB_HANDLE

  /* we need the db connection and the query */
  if( besARGNR < 2 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  /* if there is any result from any previous query then release it */
  if( q->result ){
    mysql_free_result(q->result);
    q->result = NULL;
    }

  /* get the query */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( ! Argument )return EX_ERROR_TOO_FEW_ARGUMENTS;

  Argument = besCONVERT2STRING(Argument);

  iMyError = mysql_real_query(q->hSQL,STRINGVALUE(Argument),STRLEN(Argument));
  if( iMyError != 0 )return convert_error(iMyError);
    q->result = mysql_store_result(q->hSQL);
    if( q->result == NULL ){
      iMyError = mysql_errno(q->hSQL);
      if( iMyError )return convert_error(iMyError);
      return COMMAND_ERROR_SUCCESS;
      }
    q->num_fields = mysql_num_fields(q->result);
  besRETURNVALUE = NULL;
besEND

besFUNCTION(mys_close)
  VARIABLE Argument;
  pmymysqlHANDLE q;
  pmyOBJECT p;

  GET_DB_HANDLE

  besHandleFreeHandle(p->HandleArray,besGETLONGVALUE(Argument));

  /* unlink it from the list of DB handles */
  if( q->prev )
    q->prev->next = q->next;
  else
    p->first = q->next;

  if( q->next )
    q->next->prev = q->prev;

  /* this is not needed, but I want to be safe and clean */
  q->prev = q->next = NULL;






  mysql_close(q->hSQL);


  besFREE(q);
  besRETURNVALUE = NULL;
  return COMMAND_ERROR_SUCCESS;
besEND

besFUNCTION(mys_real_connect)
  MYSQL *q;
  int iMyError;
  pmymysqlHANDLE pH;
  VARIABLE Argument;
  char *pszHost,*pszUser,*pszPassword,*pszDB,*pszUnixSocket;
  int lPort,lClientFlag;
  pmyOBJECT p;

  p = (pmyOBJECT)besMODULEPOINTER;

  pH = besALLOC(sizeof(mymysqlHANDLE));
  if( pH == NULL )return COMMAND_ERROR_MEMORY_LOW;

  pH->hSQL = mysql_init(NULL);
  pH->result = NULL;

  if( besARGNR < 4 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  /* set the default values for the optional arguments */
  lPort = MYSQL_PORT;
  pszUnixSocket = NULL;
  lClientFlag = 0;

  /* Get the host */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument ){
    besCONVERT2ZCHAR(Argument,pszHost);
  }else pszHost = NULL;

  /* Get the user */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( Argument ){
    besCONVERT2ZCHAR(Argument,pszUser);
  }else pszUser = NULL;


  /* Get the password */
  Argument = besARGUMENT(3);
  besDEREFERENCE(Argument);
  if( Argument ){
    besCONVERT2ZCHAR(Argument,pszPassword);
  }else pszPassword = NULL;

  /* Get the dbname */
  Argument = besARGUMENT(4);
  besDEREFERENCE(Argument);
  if( Argument ){
    besCONVERT2ZCHAR(Argument,pszDB);
  }else pszDB = NULL;

  if( besARGNR > 4 ){
    /* Get the port */
    Argument = besARGUMENT(5);
    besDEREFERENCE(Argument);
    if( Argument )
      lPort = (int)LONGVALUE(Argument);
    else lPort = MYSQL_PORT;
    }

  if( besARGNR > 5 ){
    /* Get the unix socket name */
    Argument = besARGUMENT(6);
    besDEREFERENCE(Argument);
    if( Argument ){
      besCONVERT2ZCHAR(Argument,pszUnixSocket);
    }else pszUnixSocket = NULL;
    }

  if( besARGNR > 6 ){
    /* Get the client flag */
    Argument = besARGUMENT(7);
    besDEREFERENCE(Argument);
    if( Argument ){
      lClientFlag = (int)LONGVALUE(Argument);
    }else lClientFlag = MYSQL_PORT;
    }

  q = mysql_real_connect(pH->hSQL,pszHost,pszUser,pszPassword,pszDB,lPort,pszUnixSocket,lClientFlag);

  if( pszHost       )besFREE(pszHost);
  if( pszUser       )besFREE(pszUser);
  if( pszPassword   )besFREE(pszPassword);
  if( pszDB         )besFREE(pszDB);
  if( pszUnixSocket )besFREE(pszUnixSocket);

  if( q ){
    besALLOC_RETURN_LONG;
    if( p->first )p->first->prev = pH;
    pH->next = p->first;
    p->first = pH;
    pH->prev = NULL;
    LONGVALUE(besRETURNVALUE) = besHandleGetHandle(p->HandleArray,pH);
    return COMMAND_ERROR_SUCCESS;
    }else{
    iMyError = mysql_errno(pH->hSQL);
    besFREE(pH);
    besRETURNVALUE = NULL;
    return convert_error(iMyError);
    }
besEND

besFUNCTION(mys_config_connect)
  MYSQL *q;
  int iMyError;
  pmymysqlHANDLE pH;
  VARIABLE Argument;
  char *pszHost,*pszUser,*pszPassword,*pszDB,*pszUnixSocket;
  long lPort,lClientFlag;
  pmyOBJECT p;
#define CONFLEN 100
#define CONFROOT "mysql.connections."
#define MAXKL 20 /* the maximum key length used in connection config */
  char szConfigPath[CONFLEN],*pszConf;
  char *pszCname;

  p = (pmyOBJECT)besMODULEPOINTER;

  pH = besALLOC(sizeof(mymysqlHANDLE));
  if( pH == NULL )return COMMAND_ERROR_MEMORY_LOW;

  pH->hSQL = mysql_init(NULL);
  pH->result = NULL;

  /* set the default values for the optional arguments */
  lPort = MYSQL_PORT;
  pszUnixSocket = NULL;
  lClientFlag = 0;

  /* Get connection name  */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument ){
    besCONVERT2ZCHAR(Argument,pszCname);
  }else return MYSQL_ERROR_NOCN;

  strcpy(szConfigPath,CONFROOT);
  if( STRLEN(Argument) > CONFLEN - strlen(CONFROOT) - MAXKL )
    return MYSQL_ERROR_BDCN;
  pszConf = szConfigPath + strlen(szConfigPath);

  memcpy(pszConf,STRINGVALUE(Argument),STRLEN(Argument));
  pszConf += STRLEN(Argument);
  *pszConf++ = '.';

  /* Get the host */
  strcpy(pszConf,"host");
  pszHost = besCONFIG(szConfigPath);

  /* Get the user */
  strcpy(pszConf,"user");
  pszUser = besCONFIG(szConfigPath);

  /* Get the password */
  strcpy(pszConf,"password");
  pszPassword = besCONFIG(szConfigPath);

  /* Get the dbname */
  strcpy(pszConf,"db");
  pszDB = besCONFIG(szConfigPath);

  /* Get the port */
  strcpy(pszConf,"port");
  besCONFIGEX(pSt->pEo->pConfig,szConfigPath,NULL,NULL,&lPort,NULL,NULL);

  /* Get the unix socket name */
  strcpy(pszConf,"socket");
  pszUnixSocket = besCONFIG(szConfigPath);
  if( pszUnixSocket && *pszUnixSocket == (char)0 )pszUnixSocket = NULL;

  /* Get the client flag */
  strcpy(pszConf,"flag");
  besCONFIGEX(pSt->pEo->pConfig,szConfigPath,NULL,NULL,&lClientFlag,NULL,NULL);

  q = mysql_real_connect(pH->hSQL,pszHost,pszUser,pszPassword,pszDB,(int)lPort,pszUnixSocket,lClientFlag);

  if( q ){
    besALLOC_RETURN_LONG;
    if( p->first )p->first->prev = pH;
    pH->next = p->first;
    p->first = pH;
    pH->prev = NULL;
    LONGVALUE(besRETURNVALUE) = besHandleGetHandle(p->HandleArray,pH);
    return COMMAND_ERROR_SUCCESS;
    }else{
    iMyError = mysql_errno(pH->hSQL);
    besFREE(pH);
    besRETURNVALUE = NULL;
    return convert_error(iMyError);
    }
besEND
