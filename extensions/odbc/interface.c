/* odbcinterf.c interface to ODBC for ScriptBasic

NTLIBS: odbc32.lib odbccp32.lib
UXLIBS: -lodbc

*/
#ifdef WIN32
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>

#include <stdio.h>

#define BYTE_TYPE_ALREADY_DEFINED 1
#include "../../basext.h"


/* The opened connections are stored in linked list */
typedef struct _odbcHANDLE {
  SQLHANDLE hConn;
  SQLHANDLE hStmt;
  SQLHANDLE hDesc;
  SQLSMALLINT num_fields;
  struct _odbcHANDLE *next,*prev;
  } odbcHANDLE, *podbcHANDLE;

/* each interpreter opens an environment and uses it to store connections */
typedef struct _odbcOBJECT {
  SQLHANDLE hEnv;
  void *HandleArray;
  podbcHANDLE first;
  } odbcOBJECT, *podbcOBJECT;


/**
ODBC Database Connection Module

This module implements ODBC interface to connect to relational databases. Using this module
you can use SQL RDBMS implementations from Windows/Unix/Linux platform independant of the 
actual database system.

Though this module does not deliver all functions of the ODBC system it does implement
most of the functions that are needed to write SQL based applications. You can connect to
ODBC databases, disconnect, issue SQL commands and retrieve the data resulted by the queries.

To use this module from a BASIC program you have to include the file T<odbc.bas> with the line

=verbatim
import odbc.bas
=noverbatim

to have all the neccessary function defintiions. This file and the neccessary T<odbc.dll> (or T<odbc.so>)
are installed in the include and in the modules directories on Windows.

A typical BASIC application using this module first calls R<Connect> or R<RealConnect> to get a 
connection to the database. Then the program calls R<Query> to execute SQL queries, altering the
database and R<FetchHash> and/or R<FetchArray> to get the result of queries. Finally the program
optionally calls R<Close> to close the connection. 

(Calling R<Close> is optional. R<Close> is automatically called for each opened database connection 
when the interpreter finishes.)

The underlying ODBC system layer implements connection pooling on process level. Thus there is
no need to use the ScriptBasic resource pooling module as it is done by the MySQL module. (By the
time I write this the ScriptBasic resource pool support module is still experimental, and so is the
MySQL module interface utilizing it.)

When the ODBC module is initialized the module requests the underlying ODBC layer to perform process
level connection pooling. The module also implements the multi-thread interface functions that allow
the module to remain in memory if used in multi-thread application.

The module raises module specific errors. For more information see R<Error>.

*/


/* no such connection name as specified */
#define ODBC_ERROR_NOCN 0x00081001
/* bad connection this means an error in the configuration file  */
#define ODBC_ERROR_BDCN 0x00081002
/* connection was refused by the odbc subsystem */
#define ODBC_ERROR_CREF 0x00081003
/* execute returned error */
#define ODBC_ERROR_EXEC 0x00081004
/* there was no statement executed before fetch */
#define ODBC_ERROR_NORS 0x00081005
/* fetch requires left value */
#define ODBC_ERROR_LVAL 0x00081006


/**
=H Error Codes

The module raises error in case when there is some problem with the underlying
database handling. The error codes the module may raise:

=itemize
=item 0x00081001 there is no such connection name
=item 0x00081002 there is some problem with the connection name. This is an error in the configuration file.
=item 0x00081003 the connection was refused by the database.
=item 0x00081004 query execution error.
=item 0x00081005 the program tries to call R<FetchArray> or R<Fetchhash> without a preceeding query execution.
=item 0x00081006 the second argument of a fetch does not evaluate to left value.
=noitemize

*/
besSUB_ERRMSG

  switch( iError ){
    case ODBC_ERROR_NOCN: return "No such connection name.";
    case ODBC_ERROR_BDCN: return "Bad connection information in the configuration file.";
    case ODBC_ERROR_CREF: return "Connection refused.";
    case ODBC_ERROR_EXEC: return "Query execution error.";
    case ODBC_ERROR_NORS: return "There was no query before the fetch.";
    case ODBC_ERROR_LVAL: return "Fetch resures left value as second argument.";
     }
  return "Unknown error.";
besEND

besVERSION_NEGOTIATE
  return (int)INTERFACE_VERSION;
besEND



besDLL_MAIN

SUPPORT_MULTITHREAD

besSUB_PROCESS_START
  INIT_MULTITHREAD
  return 1;
besEND

besSUB_PROCESS_FINISH
besEND

besSUB_KEEP
  long lTC;

  /* get the actual value of the thread counter that counts the number of threads using the library
     currently */
  GET_THREAD_COUNTER(lTC);
  /* if this was the last thread then we will return 0 not to unload the library and therefore
     this thread finish should not be counted */
  if( lTC == 0 ){
    INC_THREAD_COUNTER
    }
  /* conver the long counter to (int) boolean */
  return lTC ? 1 : 0;
besEND

besSUB_START
  podbcOBJECT p;
  SQLRETURN ret;

  INITLOCK /* lock the init mutex */
  if( iFirst ){ /* if it was not initialized yet*/
    ret = SQLSetEnvAttr(NULL, /* we set the process level connection pooling */
                        SQL_ATTR_CONNECTION_POOLING ,
                        (SQLPOINTER)SQL_CP_ONE_PER_DRIVER, /* process level connection pooling is done */
                        0 /* this is ignored */
                        );
    iFirst = 0;/* successful initialization was done*/
    }
  INITUNLO /* unlock the init mutex */

  besMODULEPOINTER = besALLOC(sizeof(odbcOBJECT));
  if( besMODULEPOINTER == NULL )return COMMAND_ERROR_MEMORY_LOW;
  p = (podbcOBJECT)besMODULEPOINTER;
  p->HandleArray = NULL;
  p->first = NULL; /* list of opened ODBC handles */
  /* allocate the environment */
  ret = SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&(p->hEnv));
  if( ret == SQL_ERROR )return COMMAND_ERROR_MEMORY_LOW;
  ret = SQLSetEnvAttr(p->hEnv,SQL_ATTR_CP_MATCH,SQL_CP_STRICT_MATCH ,0);
  ret = SQLSetEnvAttr(p->hEnv,SQL_ATTR_ODBC_VERSION,(SQLPOINTER)SQL_OV_ODBC3,0);
  return 0;
besEND

besSUB_FINISH
  podbcOBJECT p;
  podbcHANDLE q;

  p = (podbcOBJECT)besMODULEPOINTER;
  if( p != NULL ){
    for( q = p->first ; q ; q = q->next ){
      SQLDisconnect(q->hConn);
      SQLFreeHandle(SQL_HANDLE_DBC,q->hConn);
      }
    besHandleDestroyHandleArray(p->HandleArray);
    }
  SQLFreeHandle(SQL_HANDLE_ENV,p->hEnv);
  return 0;
besEND

#define GET_DB_HANDLE \
  p = (podbcOBJECT)besMODULEPOINTER;\
  Argument = besARGUMENT(1);\
  besDEREFERENCE(Argument);\
  if( ! Argument )return EX_ERROR_TOO_FEW_ARGUMENTS;\
  Argument = besCONVERT2LONG(Argument);\
  q = besHandleGetPointer(p->HandleArray,LONGVALUE(Argument));\
  if( q == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;


static int _GetData(pSupportTable pSt,
                    LEFTVALUE Lval,
                    podbcHANDLE q,
                    long i,
                    long j
  ){
  char szTmp[256];
  SQLINTEGER cbCol,cbrCol;
  SQLRETURN ret;
  VARIABLE vDebug;
  long iIndex;

  /* first get the first 256 characters of the record */
  cbCol = 256;
  ret = SQLGetData(q->hStmt,
                   i+1,
                   SQL_C_CHAR,
                   szTmp,
                   cbCol,
                   &cbrCol);
  if( ! (SQL_SUCCEEDED(ret)) )return ODBC_ERROR_EXEC;
  if( cbrCol == 0 ){
    ARRAYVALUE(*Lval,j) = NULL;
    }else{
    vDebug = ARRAYVALUE(*Lval,j) = besNEWSTRING(cbrCol);
    if( vDebug == NULL )return COMMAND_ERROR_MEMORY_LOW;
    iIndex = 0;
    while( 1 ){
      memcpy(STRINGVALUE(vDebug)+iIndex,szTmp, (cbrCol < cbCol ? cbrCol : cbCol) );
      iIndex += cbCol-1;
      if( cbrCol <= cbCol )break;
      /* read more value */
      ret = SQLGetData(q->hStmt,
                       i+1,
                       SQL_C_CHAR,
                       szTmp,
                       cbCol,
                       &cbrCol);
      if( ! (SQL_SUCCEEDED(ret)) )return ODBC_ERROR_EXEC;
      }
    }
  return COMMAND_ERROR_SUCCESS;
  }

/**
=section FetchArray
=H FetchArray

=verbatim
R = ODBC::FetchArray(DB,arr)
=noverbatim

Use this function to fetch one row from the results after a successfully executed T<SELECT> statement.
The first argument to the function is T<DB> the database handle. The second argument should be a variable.
This variable will hold the array containing the data of the actual row. The array is indexed from zero to
T<n-1> where T<n> is the number of columns in the result set.

Use consecutive calls to this fucntion to get the rwos of the result one after the other. You can mix
consecutive calls to R<FetchHash> and T<FetchArray>.

The function returns T<-1> (TRUE) if fetching a row of data was successful or T<0> (FALSE) if there
are no more rows in the dataset. In the latter case T<arr> will be T<undef>.

See also R<FetchHash>

This function calls the ODBC functions T<SQLFetch>, T<SQLDescribeCol>, T<SQLGetData>.
*/
besFUNCTION(odbc_fetcharray)
  VARIABLE Argument;
  LEFTVALUE Lval;
  podbcHANDLE q;
  SQLSMALLINT i;
  unsigned long __refcount_;
  podbcOBJECT p;
  SQLRETURN ret;
  int iError;

  GET_DB_HANDLE

  besRETURNVALUE = NULL;
  /* db , array */
  if( besARGNR < 2 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  if( q->hStmt == NULL )return ODBC_ERROR_NORS;

  Argument = besARGUMENT(2);

  besLEFTVALUE(Argument,Lval);
  if( ! Lval )return ODBC_ERROR_LVAL;

  besRELEASE(*Lval);
  *Lval = NULL; /* to be safe it is undef until we fill in some new value */

  /* if there are no columns */
  if( q->num_fields == 0 ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

  *Lval = besNEWARRAY(0,q->num_fields-1);
  if( *Lval == NULL )return COMMAND_ERROR_MEMORY_LOW;

  /* fetch the next row */
  ret = SQLFetch(q->hStmt);
  if( ! (SQL_SUCCEEDED(ret)) ){
    besALLOC_RETURN_LONG;
    LONGVALUE(besRETURNVALUE) = 0;
    return COMMAND_ERROR_SUCCESS;
    }

  for( i= 0 ; i < q->num_fields ; i++ ){

    iError = _GetData(pSt,Lval,q,i,i);
    if( iError )return iError;
    }
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = -1;
besEND

/**
=section FetchHash
=H FetchHash

=verbatim
R = ODBC::FetchHash(DB,arr)
=noverbatim

Use this function to fetch one row from the results after a successfully executed T<SELECT> statement.
The first argument to the function is T<DB> the database handle. The second argument should be a variable.
This variable will hold the hash-array containing the data of the actual row. The array is indexed from zero to
T<2*n-1> where T<n> is the number of columns in the result set. Every even element (starting with index zero) will 
hold the name of a column and the next odd element of the array will hold the value of the column. This is
according to the storage strategy of ScriptBasic for hashes.

After the successful execution of this function you will be able to access the value of the column named T<"column">
with the syntax T<arr{"column"}>. Note however that column names in SQL are usually case insensitive, but ScriptBasic
hash indexing is case sensitive. 

Use consecutive calls to this fucntion to get the rows of the result one after the other. You can mix
consecutive calls to T<FetchHash> and R<FetchArray>.

The function returns T<-1> (TRUE) if fetching a row of data was successful or T<0> (FALSE) if there
are no more rows in the dataset. In the latter case T<arr> will be T<undef>.

See also R<FetchArray>.

This function calls the ODBC functions T<SQLFetch>, T<SQLDescribeCol>, T<SQLGetData>.

*/
besFUNCTION(odbc_fetchhash)
  VARIABLE Argument;
  LEFTVALUE Lval;
  podbcHANDLE q;
  int i;
  unsigned long __refcount_;
  podbcOBJECT p;
  char *pszColNameBuffer;
  SQLSMALLINT cbColNameBuffer;
  SQLSMALLINT cbrColNameBuffer;
  SQLSMALLINT DataType;
  SQLUINTEGER ColSize;
  SQLSMALLINT DecimalDigits;
  SQLSMALLINT Nullable;
  SQLRETURN ret;
  int iError;

  /* the initial size of the buffer to store the name of the columns */
  cbColNameBuffer = 256;

  /* allocate the initial column name buffer */
  pszColNameBuffer = besALLOC(cbColNameBuffer);
  if( pszColNameBuffer == NULL )return COMMAND_ERROR_MEMORY_LOW;

  GET_DB_HANDLE

  besRETURNVALUE = NULL;
  /* db , array */
  if( besARGNR < 2 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  if( q->hStmt == NULL )return ODBC_ERROR_NORS;

  Argument = besARGUMENT(2);

  besLEFTVALUE(Argument,Lval);
  if( ! Lval )return ODBC_ERROR_LVAL;

  besRELEASE(*Lval);
  *Lval = NULL; /* to be safe it is undef until we fill in some new value */

  /* if there are no columns */
  if( q->num_fields == 0 ){
    besRETURNVALUE = NULL;
    return COMMAND_ERROR_SUCCESS;
    }

  /* fetch the next row */
  ret = SQLFetch(q->hStmt);
  if( ! (SQL_SUCCEEDED(ret)) ){
    besALLOC_RETURN_LONG;
    LONGVALUE(besRETURNVALUE) = 0;
    return COMMAND_ERROR_SUCCESS;
    }

  *Lval = besNEWARRAY(0,2*q->num_fields-1);
  if( *Lval == NULL )return COMMAND_ERROR_MEMORY_LOW;

  for( i= 0 ; i < q->num_fields ; i++ ){
    ret = SQLDescribeCol(q->hStmt,
                         i+1,
                         pszColNameBuffer,
                         cbColNameBuffer,
                        &cbrColNameBuffer,
                        &DataType,
                        &ColSize,
                        &DecimalDigits,
                        &Nullable);

    /* if the column name is longer than the currently available buffer
       then reallocate the buffer and call the column describing function again*/
    if( cbrColNameBuffer > cbColNameBuffer - 1 ){
      cbColNameBuffer = cbrColNameBuffer +1;
      besFREE(pszColNameBuffer);
      pszColNameBuffer = besALLOC(cbColNameBuffer);
      if( pszColNameBuffer == NULL )return COMMAND_ERROR_MEMORY_LOW;
      ret = SQLDescribeCol(q->hStmt,
                           i+1,
                           pszColNameBuffer,
                           cbColNameBuffer,
                          &cbrColNameBuffer,
                          &DataType,
                          &ColSize,
                          &DecimalDigits,
                          &Nullable);
      }

    /* store the name of the column */
    ARRAYVALUE(*Lval,2*i) = besNEWSTRING(cbrColNameBuffer);
    if( ARRAYVALUE(*Lval,2*i) == NULL )return COMMAND_ERROR_MEMORY_LOW;
    memcpy(STRINGVALUE(ARRAYVALUE(*Lval,2*i)),pszColNameBuffer,
                                         STRLEN(ARRAYVALUE(*Lval,2*i)));

    iError = _GetData(pSt,Lval,q,i,2*i+1);
    if( iError )return iError;
    }
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = -1;
besEND

/**
=section AffectedRows
=H AffectedRows

=verbatim
R = ODBC::AffectedRows(DB)
=noverbatim

Use this function to get the number of rows affected by a previous T<UPDATE>, T<DELETE> or
T<INSERT> command. The function returns undefined number in case the last executed was
none of the above command types.

The underlying ODBC function this function calls is T<SQLRowCount>.

*/
besFUNCTION(odbc_affected_rows)
  VARIABLE Argument;
  podbcHANDLE q;
  podbcOBJECT p;
  SQLINTEGER nRow;
  SQLRETURN ret;

  p = (podbcOBJECT)besMODULEPOINTER;
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( ! Argument )return EX_ERROR_TOO_FEW_ARGUMENTS;
  Argument = besCONVERT2LONG(Argument);
  q = besHandleGetPointer( p->HandleArray,LONGVALUE(Argument));

  ret = SQLRowCount(q->hStmt,&nRow);
  if( ret == SQL_ERROR )return ODBC_ERROR_EXEC;
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = (long)nRow;
besEND

/**
=section Error
=H Error

=verbatim
R = ODBC::Error(DB)
=noverbatim

Use this function to get the txtual report of an error that has happened during an ODBC call.
Calling ScriptBasic ODBC module functions raise error if the underlying ODBC calls report some error.
This error can be captured using the ScriptBasic T<ON ERROR GOTO> function. The error handling routine can call
this function to get the text of the error as reported by the ODBC driver.

The return value is the text of the error.

The underlying ODBC function this function calls is T<SQLError>.

*/
besFUNCTION(odbc_error)
  VARIABLE Argument;
  podbcHANDLE q;
  podbcOBJECT p;
  char szSqlState[256],szErrorMsg[256];
  SDWORD NativeError;
  SWORD cbErrorMsgMax=256;

  GET_DB_HANDLE

  SQLError(p->hEnv,q->hConn,q->hStmt,szSqlState,&NativeError,szErrorMsg,cbErrorMsgMax,&cbErrorMsgMax);
  besALLOC_RETURN_STRING(strlen(szErrorMsg));
  memcpy(STRINGVALUE(besRETURNVALUE),szErrorMsg,STRLEN(besRETURNVALUE));
besEND

/**
=section Query
=H Query

=verbatim
ODBC::Query DB, query
=noverbatim

This function should be used to execute an SQL statement. The first argument
T<DB> is the connection handle, the second argument T<query> is the text of the
SQL query.

The function returns T<undef>.

The function calls the underlying ODBC function T<SQLExecDirect> and T<SQLNumResultCols>.

*/
besFUNCTION(odbc_query)
  VARIABLE Argument;
  podbcHANDLE q;
  podbcOBJECT p;
  SQLRETURN ret;

  GET_DB_HANDLE

  /* we need the db connection and the query */
  if( besARGNR < 2 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  /* if there is any statement handle from any previous query then release it */
  if( q->hStmt ){
    SQLFreeHandle(SQL_HANDLE_STMT,q->hStmt);
    q->hStmt = NULL;
    }

  /* get the query */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( ! Argument )return EX_ERROR_TOO_FEW_ARGUMENTS;

  Argument = besCONVERT2STRING(Argument);

  ret = SQLAllocHandle(SQL_HANDLE_STMT,q->hConn,&(q->hStmt));
  if( ret == SQL_ERROR )return COMMAND_ERROR_MEMORY_LOW;
  ret = SQLExecDirect(q->hStmt,STRINGVALUE(Argument),STRLEN(Argument));

  if( ret == SQL_ERROR )return ODBC_ERROR_EXEC;

  ret = SQLNumResultCols(q->hStmt,&(q->num_fields));
  if( ret == SQL_ERROR )return ODBC_ERROR_EXEC;
  besRETURNVALUE = NULL;
besEND

/**
=section Close
=H Close

=verbatim
ODBC::Close DB
=noverbatim

Use this function to release connection. The connection may still be kept alive by the 
underlying ODBC layer for later use.

The function returns T<undef>.

The function calls the underlying ODBC function T<SQLDisconnect> and T<SQLFreeHandle>.

*/
besFUNCTION(odbc_close)
  VARIABLE Argument;
  podbcHANDLE q;
  podbcOBJECT p;
  unsigned long lHandle;

  GET_DB_HANDLE
  lHandle = LONGVALUE(Argument);

  SQLDisconnect(q->hConn);
  SQLFreeHandle(SQL_HANDLE_DBC,q->hConn);

  if( q->prev )
    q->prev->next = q->next;
  else
    p->first = q->next;

  if( q->next )
    q->next->prev = q->prev;

  besFREE(q);
  /* this was missing until v1.0b30 and it caused SB to stop when a once closed handle was used */
  besHandleFreeHandle(p->HandleArray,lHandle);
  besRETURNVALUE = NULL;
  return COMMAND_ERROR_SUCCESS;
besEND

/**
=section RealConnect
=H RealConnect

=verbatim
DB = ODBC::RealConnect(DSN,User,Password)
=noverbatim

Use this fucntion to connect to an ODBC data source. The arguments are the T<DSN> data source name as
configured in the ODBC connection manager, the T<User> user name, and the T<Password> password.

The return value is the database handle to be used in consecutive calls to R<Query>, R<FetchArray>,
R<FetchHash> and R<Close>. Note that this handle is a handle that ScriptBasic uses internally and is
not the handle returned by the underlying ODBC functions. ScriptBasic, for security reasons keeps track of
the handles using handle tables and return small integer numbers to the BASIC program to be used as reference.

You should not use any of these handles to perform any calculation other than passing them to the other
functions to identify connections. You can, however freely copy these numbers from variable to the other, 
passing to fucntions and so on as they are simply small integer numbers.

If the connection can not be established for some reason the function raises a module specific error.

See also R<Connect>, R<Error>.

The function calls the underlying ODBC function T<SQLAllocHandle> and T<SQLConnect>.

*/
besFUNCTION(odbc_real_connect)
  podbcHANDLE pH;
  VARIABLE Argument;
  char *pszDataSourceName,*pszUser,*pszPassword;
  podbcOBJECT p;
  SQLRETURN ret;

  p = (podbcOBJECT)besMODULEPOINTER;

  pH = besALLOC(sizeof(odbcHANDLE));
  if( pH == NULL )return COMMAND_ERROR_MEMORY_LOW;

  pH->hStmt = NULL;

  if( besARGNR < 3 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  ret = SQLAllocHandle(SQL_HANDLE_DBC,p->hEnv,&(pH->hConn));
  if( ret == SQL_ERROR )return COMMAND_ERROR_MEMORY_LOW;

  /* Get the data source name */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument ){
    besCONVERT2ZCHAR(Argument,pszDataSourceName);
  }else pszDataSourceName = NULL;

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


  ret = SQLConnect(pH->hConn,
                   pszDataSourceName,(SQLSMALLINT)strlen(pszDataSourceName),
                   pszUser,(SQLSMALLINT)strlen(pszUser),
                   pszPassword,(SQLSMALLINT)strlen(pszPassword));
  pH->hStmt = NULL;
  if( pszDataSourceName )besFREE(pszDataSourceName);
  if( pszUser           )besFREE(pszUser);
  if( pszPassword       )besFREE(pszPassword);

  if( SQL_SUCCEEDED(ret) ) {

    besALLOC_RETURN_LONG;
    if( p->first )p->first->prev = pH;
    pH->next = p->first;
    p->first = pH;
    pH->prev = NULL;
    LONGVALUE(besRETURNVALUE) = besHandleGetHandle(p->HandleArray,pH);
    return COMMAND_ERROR_SUCCESS;
    }else{
    besFREE(pH);
    besRETURNVALUE = NULL;
    return ODBC_ERROR_CREF;
    }

besEND

/**
=section Connect
=H Connect

=verbatim
DB = ODBC::Connect(connection_name)
=noverbatim

Use this fucntion to connect to an ODBC data source. The argument is the name of the connection as
configured in the ScriptBasic configuration file.

Connection to an ODBC database usually requires a user name and password. Coding these into
script source files may not be secure and thus this module allows the system manager to
configure these codes in the ScriptBasic config file. This config file may not be readable by
ordinary user and is binary in the form as read by the ScriptBasic interpreter. This adds some
extra security.


Calling this function instead of R<RealConnect> the programmer may reference a connection configured
in the ScriptBasic configuration file. The key with the name of the connection should be under the
key T<odbc.connections>. For example:

=verbatim
odbc (
  connections (
    test (        ; the name of the connection
	  dsn "odbctest" ; data source name
	  user "" ; user for the connection, to test we use MS-Access thus there is no user or passwd
	  password "" ; password for the connection
	  )
    )
  )
=noverbatim

When configuring such an ODBC connection do not forget to recompile the configuration file.

The name of the connection passed to the function should be the name used in the configuration file
to identify the connection. In the example above this is T<test>. This name need not, howeve may be the same
as the data source name.

If the connection can not be established for some reason the function raises a module specific error.

See also R<RealConnect>, R<Error>.

The function calls the underlying ODBC function T<SQLAllocHandle> and T<SQLConnect>.

*/
besFUNCTION(odbc_config_connect)
  VARIABLE Argument;
  SQLRETURN ret;
  char *pszDataSourceName,*pszUser,*pszPassword;
  podbcOBJECT p;
#define CONFLEN 100
#define CONFROOT "odbc.connections."
#define MAXKL 20 /* the maximum key length used in connection config */
  char szConfigPath[CONFLEN],*pszConf;
  char *pszCname;
  podbcHANDLE pH;

  p = (podbcOBJECT)besMODULEPOINTER;

  pH = besALLOC(sizeof(odbcHANDLE));
  if( pH == NULL )return COMMAND_ERROR_MEMORY_LOW;

  ret = SQLAllocHandle(SQL_HANDLE_DBC,p->hEnv,&(pH->hConn));
  if( ret == SQL_ERROR )return COMMAND_ERROR_MEMORY_LOW;

  /* Get connection name  */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument ){
    besCONVERT2ZCHAR(Argument,pszCname);
  }else return ODBC_ERROR_NOCN;

  strcpy(szConfigPath,CONFROOT);
  if( STRLEN(Argument) > CONFLEN - strlen(CONFROOT) - MAXKL )
    return ODBC_ERROR_BDCN;
  pszConf = szConfigPath + strlen(szConfigPath);

  memcpy(pszConf,STRINGVALUE(Argument),STRLEN(Argument));
  pszConf += STRLEN(Argument);
  *pszConf++ = '.';

  /* Get the data source name */
  strcpy(pszConf,"dsn");
  pszDataSourceName = besCONFIG(szConfigPath);

  /* Get the user */
  strcpy(pszConf,"user");
  pszUser = besCONFIG(szConfigPath);

  /* Get the password */
  strcpy(pszConf,"password");
  pszPassword = besCONFIG(szConfigPath);

  /* if there is no connection defined with that name then return error */
  if( NULL == pszDataSourceName ||
      NULL == pszUser           ||
      NULL == pszPassword       )return ODBC_ERROR_NOCN;

  ret = SQLConnect(pH->hConn,
                   pszDataSourceName,(SQLSMALLINT)strlen(pszDataSourceName),
                   pszUser,(SQLSMALLINT)strlen(pszUser),
                   pszPassword,(SQLSMALLINT)strlen(pszPassword));
  pH->hStmt = NULL;

  if( SQL_SUCCEEDED(ret) ){
    besALLOC_RETURN_LONG;
    if( p->first )p->first->prev = pH;
    pH->next = p->first;
    p->first = pH;
    pH->prev = NULL;
    LONGVALUE(besRETURNVALUE) = besHandleGetHandle(p->HandleArray,pH);
    return COMMAND_ERROR_SUCCESS;
    }else{
    besFREE(pH);
    besRETURNVALUE = NULL;
    return ODBC_ERROR_CREF;
    }

besEND
