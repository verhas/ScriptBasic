/* FILE: conftree.c
   HEADER: conftree.h

--GNU LGPL
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

This file contains the main definitions that are needed to store
configuration information.

A configuration information of a program is generally string values associated
with symbols. The values and the symbols are zero terminated strings.

In this implementation the symbols are hierachically ordered just like in
the Windows NT registry or the UNIX file hierarchy.

The program using this module can

  retrieve information knowing the name of the symbol,
  can traverse the tree of the symbols.

The configuration file is stored in text file or in binary file to speed up
reading. There is API to read text file, to save the read information in
binary format and to read binary format.

There is no API to modify the structure in memory. Configuration information
is changed modifying the text file and then recompiling it. The binary format
is an intermediary compiled format.



TO_HEADER:

typedef long CFT_NODE;

#define CFT_NODE_LEAF    0x00
#define CFT_NODE_BRANCH  0x01
#define CFT_NODE_MASK    0x01

#define CFT_TYPE_STRING  0x02
#define CFT_TYPE_INTEGER 0x04
#define CFT_TYPE_REAL    0x06
#define CFT_TYPE_MASK    0x06

#define CFT_ERROR_FILE   0x00000001
#define CFT_ERROR_SYNTAX 0x00000002
#define CFT_ERROR_MEMORY 0x00000003
#define CFT_ERROR_EMPTY  0x00000004
#define CFT_ERROR_NOTYPE 0x00000005

// this is the node of the compiled structure
// the nodes are stored in an array and index values
// point to each other
typedef struct _tConfigNode {
  long lKey;  // offset of the key string in the string table
  long lNext; // the next element on the same level or zero if this is the last one
  union {
    long lVal;  // the element under the node or
                // the offset of the value string in the string table
                // the value of the long number
    double dVal;// the value of the float number
    }Val;
  unsigned char fFlag; //type of the node
  } tConfigNode, *tpConfigNode;

typedef struct _tConfigTree {
  tpConfigNode Root;
  long cNode; // the number of nodes in the config tree
  char *StringTable;
  unsigned long cbStringTable; // the size of the string table
  void *(*memory_allocating_function)(size_t, void *);
  void (*memory_releasing_function)(void *, void *);
  void *pMemorySegment;
  char *pszConfigFileName;
  char TC;
  } tConfigTree, *ptConfigTree;

#define CFT_ROOT_NODE 1

#define CFT_ERROR_SUCCESS   0x00000000
#define CFT_ERROR_NOT_FOUND 0x00000001

*/
#if _WIN32
#include <windows.h>
#if BCC32 || CYGWIN
extern char *_pgmptr;
#endif
#endif
#ifdef _DEBUG
#include "testalloc.h"
#endif

static char MAGIC[4] = { 0x43, 0x46, 0x47, 0x1A };

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "conftree.h"

#define ALLOC(X) (pCT->memory_allocating_function((X),pCT->pMemorySegment))
#define FREE(X)  (pCT->memory_releasing_function((X),pCT->pMemorySegment))

static void * _mya(size_t x,void *y){
  return malloc(x);
  }
static void _myf(void *x, void *y){
  free(x);
  }

/*POD
=H cft_init()

Before calling any other configuration handling function the caller has to prepare a T<tConfigTree>
structure. To do this it has to call this function.

The first argument has to point to an allocated and uninitialized T<tConfigTree> structure. The second
argument has to point to a memory allocating function. The third argument has to point to the memory releasing
function that is capable releasing the memory allocated by the memory allocating function.

The argument T<pMemorySegment> should be the segment pointer to the memory handling functions. All memory allocation
will be performed calling the T<memory_allocating_function> and passing the T<pMemorySegment> pointer as second argument
to it. All memory releasing will be done via the function T<memory_releasing_function> passing 
T<pMemorySegment> pointer as second argument. This lets the caller to use sophisticated memory handling architecture.

B<On the other hand for the simple use> all these three arguments can be T<NULL>. In this case the configuration
management system will use its own memory allocating and releasing function that simply uses T<malloc> and T<free>.
In this case T<pMemorySegment> is ignored.

For a ready made module that delivers more features see the alloc module of the ScriptBasic project at
T<http://scriptbasic.com>

/*FUNCTION*/
int cft_init(ptConfigTree pCT,
              void *(*memory_allocating_function)(size_t, void *),
              void (*memory_releasing_function)(void *, void *),
              void *pMemorySegment
  ){
/*noverbatim
Note that suggested convention is to use the 'T<.>' character as separator for hierarchical key structures, but
this is only a suggestion. In other words the module writers advice is to use T<key.subkey.subsubkey> as key string
for hierarchical strings. On the other hand you can use any character as separator except the zero character and
except the characters that are used as key characters. You can write

=verbatim
key\subkey\subsubkey
=noverbatim

if you are a windows geek. To do this you have to change the character saying

=verbatim
    pCT->TC = '\\';
=noverbatim

after calling the initialization function. You can change this character any time, this character is not
used in the configuration structure. The only point is that you have to use the actual character when you have
changed it. The best practice is to use the dot  ever.
CUT*/
  pCT->memory_allocating_function = memory_allocating_function ?
                                    memory_allocating_function
                                               :
                                      _mya;
  pCT->memory_releasing_function = memory_releasing_function ?
                                   memory_releasing_function
                                               :
                                     _myf;
  pCT->pMemorySegment = pMemorySegment;
  pCT->Root = NULL;
  pCT->TC = '.';
  return 0;
  }

/*POD
=H cft_GetConfigFileName()

This function tries to locate the configuration file. The working of this function
is system dependant. There are two different implementations: one for UNIX and one for Win32.

B<WIN32>

On Win32 systems the function tries to read the system registry. The value of the key given in the argument
T<env> is used and returned as the config file name. For example if the argument T<env> is
T<Software\myprog\conf> then the registry value of the key T<HKEY_LOCAL_MACHINE\Software\myprog\conf> will
be returned as configuration file name. The program does not check that the file really exists. It only
checks that the registry key exists, it is a string and has some value.

If the registry key does not exists the program tries to locate the system directory getting the environment
variable T<windir>, then T<systemroot> and finally taking T<c:\WINDOWS>. The argument T<DefaultFileName> is
appended to the directory name and is returned.

B<UNIX>

On UNIX it is more simple. The environment variable T<env> is used as a file name.
If this does not exists the T<DefaultFileName> is used and returned.

B<BOTH>

The return value of the function is zero if no error has happened. A pointer to the resulting file name
is returned in the variable T<ppszConfigFile>. The space to hold the resulting file name is allocated
via the allocation function given by the T<tConfigTree> structure pointed by T<pCT>.

/*FUNCTION*/
int cft_GetConfigFileName(ptConfigTree pCT,
                          char **ppszConfigFile,
                          char *env,/* environment variable or registry key on win32 */
                          char *DefaultFileName
  ){
/*noverbatim
This function is T<static> and can not be called from outside of this module.
CUT*/

#if _WIN32
#define STRING_BUFFER_LENGTH 256
  HKEY  hKey ;
  long   Ret;
  unsigned int i;
  char *s;
  CHAR   ValueName[STRING_BUFFER_LENGTH];
  DWORD  cbValueName = STRING_BUFFER_LENGTH;
  DWORD  dwType;
  char *regkey,*svname;

  char   sData[STRING_BUFFER_LENGTH];
  char   xData[STRING_BUFFER_LENGTH];
  DWORD  cbData;
  FILE *fp;

  s = _pgmptr;
  if( strlen(s) < STRING_BUFFER_LENGTH ){
    strcpy(ValueName,s);
    s = ValueName;
    while( *s && ! isspace(*s) )s++;
    *s = (char)0;
    i = GetFullPathName(ValueName,
                        STRING_BUFFER_LENGTH,
                        sData,
                        &s);
    if( i < STRING_BUFFER_LENGTH && /* result is OK and we have*/
        STRING_BUFFER_LENGTH - i + strlen(s) > 12 ){/* space for 'scriba.conf' */
      strcpy(s,"scriba.conf");
      }
    if( fp = fopen(sData,"r") ){
      fclose(fp);
      *ppszConfigFile = ALLOC(strlen(sData)+1);
      if( *ppszConfigFile == NULL )return CFT_ERROR_MEMORY;
      strcpy(*ppszConfigFile,sData);
      return 0;
      }
    }

  /* if env is specified we try to use the file name given in the registry key specified by env */
  svname = NULL;
  if( env ){
    regkey = ALLOC(strlen(env)+1);
    if( regkey == NULL )return CFT_ERROR_MEMORY;
    strcpy(regkey,env);
    for( s = regkey + strlen(env); s > regkey ; s-- ){
      if( *s == '\\' ){
        *s++ = (char)0;
        svname = s;
        break;
        }
      }
    }

  if( svname != NULL ){
    Ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,regkey,0,KEY_EXECUTE,&hKey);
    if( Ret == ERROR_SUCCESS ){
      for( i=0 ; 1 ; i++ ){
        cbValueName = STRING_BUFFER_LENGTH;
        cbData      = STRING_BUFFER_LENGTH;
        Ret = RegEnumValue (hKey, 
                            i,           // Value index, taken from listbox.
                            ValueName,   // Name of value.
                            &cbValueName,// Size of value name.
                            NULL,        // Reserved, dword = NULL.
                            &dwType,     // Type of data.
                            sData,       // Data buffer.
                            &cbData);    // Size of data buffer.
        if( Ret != ERROR_SUCCESS )break;
        if( dwType == REG_EXPAND_SZ ){
          ExpandEnvironmentStrings(sData,xData,cbData);
          strcpy(sData,xData);
          dwType = REG_SZ;
          }
        if( dwType == REG_MULTI_SZ )dwType = REG_SZ;

        if( dwType != REG_SZ )continue;

        if( !strcmp(ValueName,svname) ){
          *ppszConfigFile = ALLOC(strlen(sData)+1);
          if( *ppszConfigFile == NULL ){
            RegCloseKey(hKey);
            return CFT_ERROR_MEMORY;
            }
          strcpy(*ppszConfigFile,sData);
          RegCloseKey(hKey);/*  */
          return 0;
          }
        }
      }
    RegCloseKey(hKey);/* Gavin Jenkins recognized it missing */
    }
  s = getenv("windir");
  if( s == NULL )s = getenv("systemroot");
  if( s == NULL )s = "c:\\WINDOWS";
  *ppszConfigFile = ALLOC(strlen(s)+strlen(DefaultFileName)+2);
  if( *ppszConfigFile == NULL )return CFT_ERROR_MEMORY;
  strcpy(*ppszConfigFile,s);
  strcat(*ppszConfigFile,"\\");
  strcat(*ppszConfigFile,DefaultFileName);
  return 0;
#else
  char *s;

  s = getenv(env);

  if( s == NULL ){
    *ppszConfigFile = ALLOC(strlen(DefaultFileName)+1);
    if( *ppszConfigFile == NULL )return CFT_ERROR_MEMORY;
    strcpy(*ppszConfigFile,DefaultFileName);
    return 0;
    }
  *ppszConfigFile = ALLOC(strlen(s)+1);
  if( *ppszConfigFile == NULL )return CFT_ERROR_MEMORY;
  strcpy(*ppszConfigFile,s);
  return 0;
#endif
  }

/*POD
=H cft_start()

When writing real applications you usually want to call this function. This function initializes the
T<tConfigTree> structure pointed by T<pCT>, searches for the configuration file and reads it.

When trying to allocate the configuration file the static internal function R<GetConfigFileName> is used.

The argument T<Envir> is the registry key under T<HKLM>, eg T<Software\Myprog\conf> under Win32 or
the environment variable to look for the configuration file name. The argument T<pszDefaultFileName>
is the file name searched on WIN32 in the system directories or the full path to the default configuration
file nam eunder UNIX. The argument T<pszForcedFileName> can overrride the file name search or
has to be T<NULL> to let the reader search the environment and registry for file name.
/*FUNCTION*/
int cft_start(ptConfigTree pCT,
              void *(*memory_allocating_function)(size_t, void *),
              void (*memory_releasing_function)(void *, void *),
              void *pMemorySegment,
              char *Envir,
              char *pszDefaultFileName,
              char *pszForcedFileName
  ){
/*noverbatim
CUT*/
  int iError;

  /* First of all we have to initialize the structure */
  if( iError = cft_init(pCT,memory_allocating_function,memory_releasing_function,pMemorySegment) )
    return iError;

#if _DEBUG
testa_Assert0x80();
#endif
  if( pszForcedFileName == NULL )
    if( iError = cft_GetConfigFileName(pCT,&pszForcedFileName,Envir,pszDefaultFileName) )return iError;
#if _DEBUG
testa_Assert0x80();
#endif
  if( strlen(pszForcedFileName) >0 ){
    pCT->pszConfigFileName = ALLOC(strlen(pszForcedFileName)+1);
    if( pCT->pszConfigFileName )
      strcpy(pCT->pszConfigFileName,pszForcedFileName);
    }else pCT->pszConfigFileName = NULL;
  iError = cft_ReadConfig(pCT,pszForcedFileName);
#if _DEBUG
testa_Assert0x80();
#endif
  return iError;
  }


/*POD
=H strmyeq()

This is an internal T<static> function that compares two strings and returns true iff they
are equal. The string terminator is the usual zero character or the dot. Both are legal terminators
for this functions and their difference in the compared strings is not treated as difference in the result.
If one string is terminated by zero character and the other is terminated by a dot but they are the same in any
other character then the return value is true.

This function is used find a sub-key when the caller has specified a dot separated hierarchical key.

Note that the dot is only a convention and the default value for the separator and the caller has 
=verbatim
/**/
static int strmyeq(ptConfigTree pCT,char *a, char *b){
/*noverbatim
This function is T<static> and can not be called from outside of this module.
CUT*/
  while( *a && *a != pCT->TC && *b && *b != pCT->TC ){
    if( *a != *b )return 0;
    a++; b++;
    }
  return   *a == *b ||
         ( *a == pCT->TC && ! *b ) ||
         ( *b == pCT->TC && ! *a );
  }

/*POD
=H cft_FindNode()

Find a node starting from the start node T<lStartNode>
and searching for the T<key>.

The function returns zero if the key is not found in the configuration
information tree T<pCT> or returns the node id of the key. This node
can either be an internal node or leaf.

Note that the string T<key> may contain dot characters. In this case the
key is searched down in the configuration tree. (You can set the separator character
different from the dot character.)

/*FUNCTION*/
CFT_NODE cft_FindNode(ptConfigTree pCT,
                      CFT_NODE lStartNode,
                      char *key
  ){
/*noverbatim
You need this function when you want to iterate over the sub-keys of a node. You get the
node id for the key and then you can call R<cft_EnumFirst> to start the loop and then R<cft_EnumNext> to
iterate the loop over the sub-keys.

If you just want to get the value of a single key you can call the function R<cft_GetEx> that
uses this function.
CUT*/
  long i;

  /* lasy programmers like I use config tree after failed config read. */
  if( pCT == NULL || pCT->Root == NULL )return 0;

RestartFindNode:
  for( i = lStartNode ; i ; i = pCT->Root[i-1].lNext ){
    /* if the key of the node is the same as the first part of the key */
    if( strmyeq(pCT,key,pCT->StringTable+pCT->Root[i-1].lKey) ){
      /* step key to point to the end of the first part */ /*pCT->TC is usually the dot character */
      while( *key && *key != pCT->TC )key++;
      /* if there are no more parts then we have found the node */
      if( !*key )return i;
      /* step over the dot to the next part of the key */
      key++;
      /* the key has sub parts but this is not a brach node. For example you want to
         get the node of key="a.b.c.d" but the node for key="a.b" is already a value
         and not a sub-configuration. */
      if( (pCT->Root[i-1].fFlag&CFT_NODE_MASK) != CFT_NODE_BRANCH )return 0;
      lStartNode = pCT->Root[i-1].Val.lVal;
      /* use a nasty goto instead of tail recursion */
      goto RestartFindNode;
      }
    }
  /* if we went through the list and could not find the key then
     there is no node for the given key */
  return 0;
  }

/*POD
=H cft_GetEx()

Get the value associated with the key T<key> from the configuration
structure T<pCT>, or get the values of a node.

The arguments:

=itemize
=item T<pCT> the configuration information searched.
=item T<key> the key that we search the value for, or NULL if we already
      know the node id where the needed information is.
=item T<plNodeId> the id of the node that we need information from. If the
      key argumentum is not NULL then this argument is overwritten with the
      node id associated with the key. If the argument key is NULL this
      argument should specify the id of the node we need information from.
      If the node id is not needed upon return this argument may point to NULL.
=item T<ppszValue> will return a pointer to a constant ZCHAR string if
      the value associated with T<key> is string. If the argument is T<NULL>
      then the function ignore this argument.
=item T<plValue> will return a T<long> if the value associated with
      T<key> is integer. If the argument is T<NULL>
      then the function ignore this argument.
=item T<pdValue> will return a T<double> if the value associated with
      T<key> is a real number. If the argument is T<NULL>
      then the function ignore this argument.
=item T<type> will return the type of the key. This can be
  =itemize
  =item T<CFT_NODE_BRANCH> if the key is associated with a subtree.
  =item T<CFT_TYPE_STRING> if the key is associated with a string
  =item T<CFT_TYPE_INTEGER> if the key is associated with an integer number
  =item T<CFT_TYPE_REAL> if the key is associated with a real number
  =noitemize
  This argument can also be NULL if the caller is not interested in the
  type of the value.
=noitemize

Note that any of T<ppszValue>, T<plValue>, T<pdValue> can point to a
variable or to T<NULL> in case the caller does not need the actual value.

/*FUNCTION*/
int cft_GetEx(ptConfigTree pCT,
              char *key,
              CFT_NODE *plNodeId,
              char **ppszValue,
              long *plValue,
              double *pdValue,
              int *type
  ){
/*noverbatim

The function returns T<CFT_ERROR_SUCCESS> if no error happens.
The value T<CFT_ERROR_SUCCESS> is zero.

If an error happens the error code is returned. These error codes are:
=itemize
=item T<CFT_ERROR_NOT_FOUND> the key is not present in the table, and
      T<*plNodeId> will also be set to zero.
=item T<CFT_ERROR_NOTYPE> the key is found but has a type that can not
      be returned, because the caller passed NULL as storage location.
      In this case the type of the configuration information is probably
      wrong.
=noitemize

CUT*/
  CFT_NODE lNodeId;

  if( plNodeId )lNodeId = *plNodeId;
  if( key )
    lNodeId = cft_FindNode(pCT,1,key);
  if( plNodeId )*plNodeId = lNodeId;

  if( lNodeId == 0 )return CFT_ERROR_NOT_FOUND;
  if( (pCT->Root[lNodeId-1].fFlag&CFT_NODE_MASK) == CFT_NODE_BRANCH ){
    if( type )
      *type = CFT_NODE_BRANCH;
    return CFT_ERROR_SUCCESS;
    }
  if( type )
    *type = pCT->Root[lNodeId-1].fFlag & CFT_TYPE_MASK;
  switch( pCT->Root[lNodeId-1].fFlag & CFT_TYPE_MASK ){
    default: return CFT_ERROR_NOT_FOUND;
    case CFT_TYPE_STRING:
      if( ppszValue )
        *ppszValue = pCT->StringTable + pCT->Root[lNodeId-1].Val.lVal;
      else
        return CFT_ERROR_NOTYPE;
      break;
    case CFT_TYPE_INTEGER:
      if( plValue )
        *plValue = pCT->Root[lNodeId-1].Val.lVal;
      else
        return CFT_ERROR_NOTYPE;
      break;
    case CFT_TYPE_REAL:
      if( pdValue )
        *pdValue = pCT->Root[lNodeId-1].Val.dVal;
      else
        return CFT_ERROR_NOTYPE;
      break;
    }
  return CFT_ERROR_SUCCESS;
  }

/*POD
=H cft_GetString()

This is the simplest interface function to retrieve a configuration
string. This assumes that you exactly know the name of the key and
you are sure that the value is a string. The function returns the pointer
to the constant string or returns NULL if the configuration key is not
present in the tree or the value is not a string.

The use of this function is not recommended. This function is present
in this package to ease porting of programs that use simpler configuration
information management software.
/*FUNCTION*/
char *cft_GetString(ptConfigTree pCT,
                    char *key
  ){
/*noverbatim
This function calls R<cft_GetEx>.

CUT*/
  char *pszReturn;
  int type;
  long dummy;/* Store the node id, though we do not need it. */

  if( cft_GetEx(pCT,key,&dummy,&pszReturn,NULL,NULL,&type) )return NULL;
  if( type != CFT_TYPE_STRING )return NULL;
  return pszReturn;
  }

/*POD
=H cft_EnumFirst()

Whenever you need to enumerate the sub-keys of a key you have to
get the node associated with the key (see R<cft_GetEx> or R<cft_FindNode>).
When you have the node associated with the key you can get the node of the
first sub-key calling this function.

The function needs the node id T<lNodeId> of the key for which
we need to enumerate the sub keys and returns the node id of the
first sub key.

If the key is associated with a leaf node the function returns zero.

If the key is associated with a branch node that has no sub-keys the
function returns zero.
/*FUNCTION*/
CFT_NODE cft_EnumFirst(ptConfigTree pCT,
                       CFT_NODE lNodeId
  ){
/*noverbatim
CUT*/

  if( lNodeId == 0 )return 1;
  if( (pCT->Root[lNodeId-1].fFlag&CFT_NODE_MASK) != CFT_NODE_BRANCH )return 0;
  return pCT->Root[lNodeId-1].Val.lVal;
  }

/*POD
=H cft_EnumNext()

Whenever you need to enumerate the sub-keys of a key you have to
get the node associated with the key (see R<cft_GetEx> or R<cft_FindNode>).
When you have the node associated with the key you can get the node of the
first sub-key calling the function R<cft_EnumFirst>. Later on you can enumerate
the sub keys stepping from node to node calling this function.

The function needs the node id T<lNodeId> returned by R<cft_EnumFirst> or
by previous call of this function.

The function returns the node id of the next sub key.

If the enumeration has ended, in other words there is no next sub-key the
function returns zero.
/*FUNCTION*/
long cft_EnumNext(ptConfigTree pCT,
                  long lNodeId
  ){
/*noverbatim
CUT*/
  return pCT->Root[lNodeId-1].lNext;
  }

/*POD
=H cft_GetKey()

This function returns a pointer to the constant zchar string that
holds the key of the node defined by the id T<lNodeId>.
/*FUNCTION*/
char *cft_GetKey(ptConfigTree pCT,
                 CFT_NODE lNodeId
  ){
/*noverbatim
CUT*/
  if( lNodeId == 0 )return NULL;
  return pCT->StringTable + pCT->Root[lNodeId-1].lKey;
  }

/*POD
=H cft_ReadConfig()

/*FUNCTION*/
int cft_ReadConfig(ptConfigTree pCT,
                   char *pszFileName
  ){
/*noverbatim
CUT*/
  FILE *fp;
  size_t cbRead;
  char magic[4];
  unsigned long lNodeSize;

  fp = fopen(pszFileName,"rb");
  if( fp == NULL )return CFT_ERROR_FILE;
  cbRead = fread(magic,1,4,fp);
  if( cbRead != 4 || memcmp(magic,MAGIC,4) ){
    fclose(fp);
    return CFT_ERROR_FILE;
    }

  cbRead = fread(&lNodeSize,1,sizeof(long),fp);
  if( cbRead != sizeof(long) || lNodeSize != sizeof(tConfigNode) ){
    fclose(fp);
    return CFT_ERROR_FILE;
    }

  cbRead = fread((void *)&(pCT->cNode),1,sizeof(long),fp);
  if( cbRead != sizeof(long) ){
    fclose(fp);
    return CFT_ERROR_FILE;
    }
  cbRead = fread((void *)&(pCT->cbStringTable),1,sizeof(long),fp);
  if( cbRead != sizeof(long) ){
    fclose(fp);
    return CFT_ERROR_FILE;
    }
  if( pCT->cNode == 0 ){
    fclose(fp);
    return CFT_ERROR_EMPTY;
    }
  pCT->Root = ALLOC(pCT->cNode*sizeof(tConfigNode));
  if( pCT->Root == NULL ){
    fclose(fp);
    return CFT_ERROR_MEMORY;
    }
  pCT->StringTable = ALLOC(pCT->cbStringTable);
  if( pCT->StringTable == NULL ){
    fclose(fp);
    FREE(pCT->Root);
    return CFT_ERROR_MEMORY;
    }
  cbRead = fread((void *)pCT->Root,1,pCT->cNode*sizeof(tConfigNode),fp);
  if( cbRead != pCT->cNode*sizeof(tConfigNode) ){
    fclose(fp);
    return CFT_ERROR_FILE;
    }
  cbRead = fread((void *)pCT->StringTable,1,pCT->cbStringTable,fp);
  fclose(fp);
  if( cbRead != pCT->cbStringTable )return CFT_ERROR_FILE;
  return 0;
  }

/*POD
=H cft_WriteConfig()

/*FUNCTION*/
int cft_WriteConfig(ptConfigTree pCT,
                    char *pszFileName
  ){
/*noverbatim
CUT*/
  FILE *fp;
  size_t cbWrite;
  unsigned long lNodeSize;

  if( pCT->cNode == 0 )return CFT_ERROR_EMPTY;

  fp = fopen(pszFileName,"wb");
  if( fp == NULL )return CFT_ERROR_FILE;
  cbWrite = fwrite(MAGIC,1,4,fp);
  if( cbWrite != 4 ){
    fclose(fp);
    return CFT_ERROR_FILE;
    }

  lNodeSize = sizeof(tConfigNode);
  cbWrite = fwrite(&lNodeSize,1,sizeof(long),fp);
  if( cbWrite != sizeof(long) ){
    fclose(fp);
    return CFT_ERROR_FILE;
    }

  cbWrite = fwrite((void *)&(pCT->cNode),1,sizeof(long),fp);
  if( cbWrite != sizeof(long) ){
    fclose(fp);
    return CFT_ERROR_FILE;
    }

  cbWrite = fwrite((void *)&(pCT->cbStringTable),1,sizeof(long),fp);
  if( cbWrite != sizeof(long) ){
    fclose(fp);
    return CFT_ERROR_FILE;
    }

  cbWrite = fwrite((void *)pCT->Root,1,pCT->cNode*sizeof(tConfigNode),fp);
  if( cbWrite != pCT->cNode*sizeof(tConfigNode) ){
    fclose(fp);
    return CFT_ERROR_FILE;
    }
  cbWrite = fwrite((void *)pCT->StringTable,1,pCT->cbStringTable,fp);
  fclose(fp);
  if( cbWrite != pCT->cbStringTable )return CFT_ERROR_FILE;
  return 0;
  }

/*POD
=H cft_DropConfig()

/*FUNCTION*/
void cft_DropConfig(ptConfigTree pCT
  ){
/*noverbatim
CUT*/

  FREE(pCT->Root);
  pCT->Root = NULL;
  FREE(pCT->StringTable);
  pCT->StringTable = NULL;
  pCT->cNode = 0;
  pCT->cbStringTable = 0;
  }
