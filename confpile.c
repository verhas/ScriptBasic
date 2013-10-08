/* FILE: confpile.c
   HEADER: confpile.h

TO_HEADER:

*/

/*POD
=H Compile Configuration Information Text File

The functions in this file implement the features that let a program to
read the configuration information from a text file.

The text file reading is performed using the lexical analyzer of the module R<lsp/index>

CUT*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "conftree.h"
#include "lsp.h"
#include "confpile.h"

#define ALLOC(X) (pCT->memory_allocating_function((X),pCT->pMemorySegment))
#define FREE(X)  (pCT->memory_releasing_function((X),pCT->pMemorySegment))

/*POD
=section BuildSubTree
=H Build a sub-tree from the lisp structure

This function recursively builds up the subtrees.
The arguments:

=itemize
=item T<pCT> is the configuration tree class object pointer
=item T<pLSP> is the LISP structure handling functions class object pointer
=item T<plNodeIndex> is the actual first free node id
=item T<plSindex> is the actual string index (first free character location in the string table)
=item T<q> is the root of the LISP structure to convert
=noitemize

=verbatim
/**/
static int BuildSubTree(ptConfigTree pCT,
                        tLspObject *pLSP,
                        CFT_NODE *plNindex, /* node index   */
                        long *plSindex, /* string index */
                        LVAL q){
/*noverbatim
B<This is a static internal function!>
CUT*/
  LVAL r;
  long prev;
  CFT_NODE lThisNode;

  prev = 0; /* when we start there is no previous element where we have to fill the 'next' pointer */
  
  while( q ){/* go through the LISP list  */

    /* store the location where we will copy the key */
    pCT->Root[(*plNindex)-1].lKey = *plSindex;
    /* if there is a previous node then we are the next */
    if( prev )pCT->Root[prev-1].lNext = *plNindex;
    /* we dont have a next currently, the next iteration may overwrite this */
    pCT->Root[(*plNindex)-1].lNext = 0;
    /* and for the next iteration we are going to be the previous */
    prev = *plNindex;

    /* now copy the key to the string table at the location we remembered in the node */
    strcpy(pCT->StringTable+*plSindex,getsymbol(car(q)));
    /* and step the string index after the copied string */
    *plSindex += strlen(getsymbol(car(q))) +1;

    /* r becomes the atom that follows the key string. aka (keystring r) */
    r = cadr(q);
    if( consp(r) ){/* if this is a compound node with sub-keys */
      lThisNode = *plNindex;
      /* the first sub node will be placed on the first free node, that is :   */
      ++(*plNindex);
      pCT->Root[lThisNode-1].Val.lVal = (*plNindex);
      /* this is a compound node */
      pCT->Root[lThisNode-1].fFlag = CFT_NODE_BRANCH;
      /* call itself recursively to build up the node
         note that this may increase the node and string index */
      BuildSubTree(pCT,
                   pLSP,
                   plNindex,
                   plSindex,
                   r);
      }else{
      /* this is not a compound node */
      if( stringp(r) || symbolp(r) ){
        strcpy(pCT->StringTable+*plSindex,getstring(r));
        pCT->Root[(*plNindex)-1].Val.lVal = *plSindex;
        pCT->Root[(*plNindex)-1].fFlag = CFT_NODE_LEAF | CFT_TYPE_STRING;
        *plSindex += strlen(getstring(r)) +1;
        }else
      if( floatp(r) ){
        pCT->Root[(*plNindex)-1].Val.dVal = getfloat(r);
        pCT->Root[(*plNindex)-1].fFlag = CFT_NODE_LEAF | CFT_TYPE_REAL;
        }else
      if( integerp(r) ){
        pCT->Root[(*plNindex)-1].Val.lVal = getint(r);
        pCT->Root[(*plNindex)-1].fFlag = CFT_NODE_LEAF | CFT_TYPE_INTEGER;
        }
      /* step to the next node */
      (*plNindex)++;
      }
    /* step for the next node on the same level */
    q = cddr(q);
    }
  return 0;
  }

extern int GlobalDebugDisplayFlag;

/*POD
=section RemoveNil
=H Remove symbols that have empty value

This function is used to remove the symbols and their values that have T<NIL> value. This
is sometimes, like in cases:

=verbatim
run (
 ; start "runthis.bas"
 )
=noverbatim

is hard to debug.

=verbatim
/**/
static void RemoveNil(ptConfigTree pCT,
                     tLspObject *pLSP,
                     LVAL *q){
/*noverbatim
B<This is a static internal function!>
CUT*/
  LVAL r,*p;

  if( NULL == q )return;

  p = q;
  while( *q ){
    r = cadr(*q);
    if( NULL == r ){
      *q = cddr(*q);
      continue;
      }
    if( consp(r) ){
      RemoveNil(pCT,pLSP,&((*q)->n_value.n_cons._cdr->n_value.n_cons._car));
      }
    if( *q )
      q = & ( (*q)->n_value.n_cons._cdr);
    if( *q )
      q = & ( (*q)->n_value.n_cons._cdr);
    }
  return;
  }

/*POD
=section CountSubTree
=H Count the size of the sub-tree

This function is called to calculate the number of nodes and the size of the string table that is needed
to store the configuration information.

=verbatim
/**/
static int CountSubTree(ptConfigTree pCT,
                        tLspObject *pLSP,
                        long *plNodeCounter,
                        long *plStringCounter,
                        LVAL q){
/*noverbatim
B<This is a static internal function!>
CUT*/
  LVAL r;
  int iError;
  while( q ){
    (*plNodeCounter)++;
    if( ! symbolp(car(q)) ){
      if( GlobalDebugDisplayFlag ){
        fprintf(stderr,"The node should have been a symbol.\n");
        pprint(q,stderr);
        }
      return CFT_ERROR_SYNTAX;
      }
    *plStringCounter += strlen(getsymbol(car(q))) + 1;
    r = cadr(q);
    if( consp(r) ){
      iError = CountSubTree(pCT,pLSP,plNodeCounter,plStringCounter,r);
      if( iError )
        return CFT_ERROR_SYNTAX;
      }else{
      if( stringp(r) || symbolp(r) ){
        *plStringCounter += strlen( getstring(r) ) + 1;
        }else{
        if( (!floatp(r)) && (!integerp(r)) ){
          if( GlobalDebugDisplayFlag ){
            fprintf(stderr,"The node should have been an integer, float or integer.\n");
            pprint(r,stderr);
            fprintf(stderr,"This is the value of the symbol ");
            pprint(q,stderr);
            }
          return CFT_ERROR_SYNTAX;
          }
        }
      }
    q = cddr(q);
    }
  return 0;
  }

/*POD
=section cft_ReadTextConfig
=H Read configuration information from a text file

This function opens and reads the configuration information from a text file. The argumentum
T<pCT> should point to an intialized T<tConfigTree> structure. Initialization has to be done
calling R<conftree/cft_init>.

The built up structure is stored in the structure pointed by T<pCT> and values can be extracted
from it calling the functions of R<conftree/index>
/*FUNCTION*/
int cft_ReadTextConfig(ptConfigTree pCT,
                       char *pszFileName
  ){
/*noverbatim
CUT*/
  FILE *fp;
  tLspObject MyLSP,*pLSP;
  LVAL q;
  int iError;
  long lNindex,lSindex;

  pLSP = &MyLSP;
  lsp_init(pLSP,-1,1,pCT->memory_allocating_function,
                    pCT->memory_releasing_function,
                    pCT->pMemorySegment);
  fp = fopen(pszFileName,"r");
  if( fp == NULL )return CFT_ERROR_FILE;
  q = readlist(fp);
  fclose(fp);
  pCT->cNode = 0;
  pCT->cbStringTable = 0;
  RemoveNil(pCT,pLSP,&q);
  if( iError = CountSubTree(pCT,
                            pLSP,
                            &(pCT->cNode),
                            &(pCT->cbStringTable),
                            q)                       )return iError;
  if( pCT->cNode == 0 )return CFT_ERROR_EMPTY;
  pCT->Root = ALLOC(pCT->cNode*sizeof(tConfigNode));
  if( pCT->Root == NULL )return CFT_ERROR_MEMORY;
  pCT->StringTable = ALLOC(pCT->cbStringTable);
  if( pCT->StringTable == NULL ){
    FREE(pCT->Root);
    return CFT_ERROR_MEMORY;
    }
  lNindex = 1;
  lSindex = 0;
  BuildSubTree(pCT,pLSP,&lNindex,&lSindex,q);
  freelist(q);
  return 0;
  }

/*POD
=section DumpTree
=H Recursive function to dump a subtree

This fucntion dumps a subtree of the configuration to the output file.
The printing is formatted according to the LISP notation of the text
version of the configuration. Thus a file dumped using this function
(invokable using the option T<-D> with scriba) can later be compiled
back to binary format.

The printing is also indented and the basic indentation should be
specified in the argument T<offset>. When a subtree is printed using
recursive call this argument is increased by two and thus the branches
are written each time two characters to the right as they are nested.

Arguments:

=itemize
=item T<pCT> is the whole configuration information structure
=item T<fp> the opened file where the information is to be sent
=item T<hNode> is the first node of the subtree. This is not the BRANCH node
      but alread the first node in the subtree
=item T<offset> the number of initial spaces to print in front of each line (except
      inside multi-line string literals)
=noitemize

/*FUNCTION*/
static int DumpTree(ptConfigTree pCT,
                    FILE *fp,
                    CFT_NODE hNode,
                    int offset
  ){
/*noverbatim
Note that this function is T<static> and is not meant to be called from outside. The "head"
function that initializes the recursive printing is R<cft_DumpConfig>.
CUT*/
  CFT_NODE hNodeIter;
  char *s;
  char *t;

  hNodeIter = hNode;
  while( hNodeIter ){
    if( (pCT->Root[hNodeIter-1].fFlag&CFT_NODE_MASK) == CFT_NODE_BRANCH ){
      fprintf(fp,"%*s%s (\n",offset,"",pCT->StringTable+pCT->Root[hNodeIter-1].lKey);
      DumpTree(pCT,fp,cft_EnumFirst(pCT,hNodeIter),offset+2);
      fprintf(fp,"%*s )\n",offset,"");
      }else
    if( (pCT->Root[hNodeIter-1].fFlag&CFT_TYPE_MASK) == CFT_TYPE_STRING ){
      fprintf(fp,"%*s%s ",offset,"",pCT->StringTable+pCT->Root[hNodeIter-1].lKey);
      t = "\"";
      for( s = pCT->StringTable+pCT->Root[hNodeIter-1].Val.lVal ; *s ; s++ ){
         if( *s == '\n' || *s == '\r' ){
           t = "\"\"\"";
           break;
           }
         }
      fprintf(fp,t);
      for( s = pCT->StringTable+pCT->Root[hNodeIter-1].Val.lVal ; *s ; s++ ){
        if( *s == '"' ){
          fprintf(fp,"\\\"");
          continue;
          }
        if( *s == '\\' ){
          fprintf(fp,"\\\\");
          continue;
          }
        fprintf(fp,"%c",*s);
        }
      fprintf(fp,"%s\n",t);
      }else
    if( (pCT->Root[hNodeIter-1].fFlag&CFT_TYPE_MASK) == CFT_TYPE_INTEGER ){
      fprintf(fp,"%*s%s %d\n",offset,"",pCT->StringTable+pCT->Root[hNodeIter-1].lKey,
                                         pCT->Root[hNodeIter-1].Val.lVal);
      }else
    if( (pCT->Root[hNodeIter-1].fFlag&CFT_TYPE_MASK) == CFT_TYPE_REAL ){
      fprintf(fp,"%*s%s %f\n",offset,"",pCT->StringTable+pCT->Root[hNodeIter-1].lKey,
                                         pCT->Root[hNodeIter-1].Val.dVal);
      }
    hNodeIter = cft_EnumNext(pCT,hNodeIter);
    }
  return 0;
  }

/*POD
=section cft_DumpConfig
=H Dump the configuration information into a text file

/*FUNCTION*/
int cft_DumpConfig(ptConfigTree pCT,
                   char *pszFileName
  ){
/*noverbatim
CUT*/
  FILE *fp;
  int iError;
  int iShouldClose = 0;

  if( !strcmp(pszFileName,"STDOUT") )
    fp = stdout;
  else
  if( !strcmp(pszFileName,"STDERR") )
    fp = stderr;
  else{
    fp = fopen(pszFileName,"w");
    iShouldClose = 1;
    }
  if( fp == NULL )return CFT_ERROR_FILE;
  iError = DumpTree(pCT,fp,CFT_ROOT_NODE,0);
  if( iShouldClose )fclose(fp);
  return iError;
  }


/*POD
=section cft_DumbConfig
=H Dump the configuration information into a text file in dumb mode

/*FUNCTION*/
int cft_DumbConfig(ptConfigTree pCT,
                   char *pszFileName
  ){
/*noverbatim
CUT*/
  int i;
  FILE *fp;
  int iShouldClose = 0;

  if( !strcmp(pszFileName,"STDOUT") )
    fp = stdout;
  else
  if( !strcmp(pszFileName,"STDERR") )
    fp = stderr;
  else{
    fp = fopen(pszFileName,"w");
    iShouldClose = 1;
    }
  if( fp == NULL )return CFT_ERROR_FILE;
  for( i=0 ; i < pCT->cNode ; i++ ){
    fprintf(fp,"Node %d:",i+1);
    fprintf(fp,"(%s)",pCT->StringTable+pCT->Root[i].lKey);
    switch( pCT->Root[i].fFlag ){
      case CFT_NODE_BRANCH:  fprintf(fp," BRANCH %d",pCT->Root[i].Val.lVal); break;
      case CFT_TYPE_STRING:  fprintf(fp," \"%s\"",pCT->StringTable+pCT->Root[i].Val.lVal); break;
      case CFT_TYPE_INTEGER: fprintf(fp," %d",pCT->Root[i].Val.lVal); break;
      case CFT_TYPE_REAL:    fprintf(fp," %lf",pCT->Root[i].Val.dVal); break;
      default: fprintf(fp,"Unknown node type: %d",pCT->Root[i].fFlag);
      }
    fprintf(fp," NEXT->%d",pCT->Root[i].lNext);
    fprintf(fp,"\n");
    }

  if( iShouldClose )fclose(fp);
  return 0;
  }
