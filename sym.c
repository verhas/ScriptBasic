/* 
FILE:   sym.c
HEADER: sym.h

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

TO_HEADER:
#include <stddef.h>
typedef struct _symbol {
  char  *name;
  void  *value;
  struct _symbol *small_son, *big_son;
  } Symbol, *pSymbol, **SymbolTable;

#define SymbolValue(x) (x->value)
// get the pointer to the stored string from the value pointer 
#define SymbolName(x) ((char *)*((char **)( ((unsigned char *)(x)) - offsetof(struct _symbol,value) + offsetof(struct _symbol,name) )))

*/
#include <string.h>
#include <ctype.h>
#include "sym.h"

static void _to_lower(char *s){
  /* return */ /* uncomment this return to make variable names in scriba case sensitive */
  while( *s ){
    if( isupper(*s) )*s = tolower(*s);
    s++;
    }
  }

/*
** HASH function
**
** I found this function on the page 436. of the dragon book.
**
** The dragon book:
** Aho-Sethi-Ulman : Compilers
**      Principles, techniques, and Tools
** Addison-Wesley Publishing Company 1986
**
** The only modification what was made by me is the letter 'l'
** following the 0xf0000000 constant for the specifying it long
** explicitly. ( The constant PRIME can be found in the file defines.h
** either, but the redefinition of it is identical. )
*/
#define PRIME 211
#define MASK  0xf0000000l


static int hashpjw(s)
char   *s;
{
  char *p;
  unsigned long h = 0, g;
  for ( p = s; *p != '\0'; p = p+1 ) {
    h = (h << 4) + (*p);
    if (g = h&MASK) {
      h = h ^ (g >> 24);
      h = h ^ g;
    }
  }
  return h % PRIME;
}

/*POD
=H sym_NewSymbolTable()

This function creates a new symbol table. Later this symbol table should be used
to store and retrieve symbol information.
/*FUNCTION*/
SymbolTable sym_NewSymbolTable(
  void* (*memory_allocating_function)(size_t,void *),
  void *pMemorySegment
  ){
/*noverbatim
The second argument should point to the memory allocating function that the
symbol table creation process should use. The last argument is an pointer to a memory
segment which is passed to the memory allocation function. The actual arguments of the
memory allocation function fits the allocation function from the package alloc. However the
defintion is general enough to use any other function.
CUT*/
  SymbolTable t;
  int i;

  if( ! (t = (SymbolTable)memory_allocating_function(PRIME*sizeof(pSymbol),pMemorySegment)) )return NULL;
  for( i=0 ; i<PRIME ; i++ )t[i]=NULL;
  return t;
  }


static void sym_FreeSymbolSub(
  pSymbol table,
  void (*memory_releasing_function)(void *, void *),
  void *pMemorySegment
  ){
  if( ! table )return;
  if( table->small_son )
    sym_FreeSymbolSub(table->small_son,memory_releasing_function,pMemorySegment);
  if( table->big_son )
    sym_FreeSymbolSub(table->big_son,memory_releasing_function,pMemorySegment);
  if( table->name )
    memory_releasing_function(table->name,pMemorySegment);
  memory_releasing_function(table,pMemorySegment);
  }

/*POD
=H sym_FreeSymbolTable()

This function should be used to release the memory allocated for a symbol table.
This function releases all the memory that was allocated during symbol table creation
and during symbol insertion.

Note that the memory allocated outside the symbol table handling routines is not
released. This means that it is the caller responsibility to relase all memory
that holds the actual values associated with the symbols.
/*FUNCTION*/
void sym_FreeSymbolTable(
  SymbolTable table,
  void (*memory_releasing_function)(void *,void *),
  void *pMemorySegment
  ){
/*noverbatim
CUT*/
  int i;

  for( i=0 ; i<PRIME ; i++ )
    sym_FreeSymbolSub(table[i],memory_releasing_function,pMemorySegment);

  memory_releasing_function(table,pMemorySegment);
  }

static void sym_TraverseSymbolTableSub(
  pSymbol table,
  void (*call_back_function)(char *SymbolName, void *SymbolValue, void *f),
  void *f
  ){

  if( ! table )return;
  if( table->small_son )
    sym_TraverseSymbolTableSub(table->small_son,call_back_function,f);
  if( table->big_son )
    sym_TraverseSymbolTableSub(table->big_son,call_back_function,f);
  if( table->name )
    call_back_function(table->name,table->value,f);
  }

/*POD
=H sym_TraverseSymbolTable()

This function can be used to traverse through all the symbols stored
in a symbol table. The function starts to go through the symbols and
for each symbol calls the function T<call_back_function>.
/*FUNCTION*/
void sym_TraverseSymbolTable(
  SymbolTable table,
  void (*call_back_function)(char *SymbolName, void *SymbolValue, void *f),
  void *f
  ){
/*noverbatim
The first argument is the symbol table to traverse. The second argument is the
function to be called for each symbol. This function gets three arguments. The
first is a pointer to the symbol string. The second is the pointer to the symbol
arguments. The third argument is a general pointer which is passed to the
function T<sym_TraverseSymbolTable>.

Note that the call back function gets the pointer to the symbol arguments and
not the pointer to the pointer to the symbol arguments, and therefore call back
function can not change the actual symbol value pointer.
CUT*/
  int i;

  if( !table ) return;
  for( i=0 ; i<PRIME ; i++ )
    sym_TraverseSymbolTableSub(table[i],call_back_function,f);
  }

/*POD
=H sym_LookupSymbol()

This function should be used to search a symbol or to insert a new symbol.

/*FUNCTION*/
void **sym_LookupSymbol(
  char *s,                 /* zero terminated string containing the symbol                 */
  SymbolTable hashtable,   /* the symbol table                                             */
  int insert,              /* should a new empty symbol inserted, or return NULL instead   */
  void* (*memory_allocating_function)(size_t, void *),
  void (*memory_releasing_function)(void *, void *),
  void *pMemorySegment
  ){
/*noverbatim

This function usually returns a pointer to the T<void *> pointer which is
supposed to point to the structure, which actually holds the parameters for
the symbol. When a symbol is not found in the symbol table the parameter T<insert>
is used to decide what to do. If this parameter is zero the function returns T<NULL>.
If this parameter is 1 the function creates a new symbol and returns a pointer to the
T<void *> pointer associated with the symbol.

If a new symbol is to be inserted and the function returns T<NULL> means that the memory 
allocation function has failed.

If the new symbol was created and a pointer to the T<void *> pointer is returned the value of
the pointer is T<NULL>. In other words:

=verbatim

void **a;

a = sym_LookupSymbol(s,table,1,mymema,mymemr,p);

if( a == NULL )error("memory releasing error");
if( *a == NULL )error("symbol not found");

=noverbatim

CUT*/
  pSymbol *work_pointer;
  int k;

  /* convert the string to lower case to make it case insensitive */
  _to_lower(s);

  k = hashpjw( s );
  work_pointer = &(hashtable[ k ]);
  while( *work_pointer && (k=strcmp(s,(*work_pointer)->name)) )
    work_pointer= k > 0 ? &((*work_pointer)->big_son)  :
        &((*work_pointer)->small_son);

  if( *work_pointer )return &((*work_pointer)->value);

  /* if there is no such symbol and we need not insert it then return NULL */
  if( ! insert )return NULL;

  /* there is no such symbol and we have to create it */
  /* allocate memory for the symbol entry and the copy of the name */
  *work_pointer = (pSymbol)memory_allocating_function( sizeof(Symbol),pMemorySegment);
  if( !*work_pointer )return NULL;

  (*work_pointer)->name = (char *)memory_allocating_function( (strlen( s )+1)*sizeof(char),pMemorySegment );
  if( !(*work_pointer)->name ) {
    memory_releasing_function(*work_pointer,pMemorySegment);
    return NULL;
    }
  strcpy((*work_pointer)->name , s);

  /* value is always NULL for a newly allocated symbol */
  (*work_pointer)->value = NULL;

  /* no sons */
  (*work_pointer)->big_son =
      (*work_pointer)->small_son = NULL;

  return &((*work_pointer)->value);
}

/*POD
=H sym_DeleteSymbol()

This function should be used to delete a symbol from the symbol table

/*FUNCTION*/
int sym_DeleteSymbol(
  char *s,                 /* zero terminated string containing the symbol                 */
  SymbolTable hashtable,   /* the symbol table                                             */
  void (*memory_releasing_function)(void *, void *),
  void *pMemorySegment
  ){
/*noverbatim

This function searches the given symbol and if the symbol is found it deletes it from the
symbol table. If the symbol was found in the symbol table the return value is zero. If
the symbol was not found the return value is 1. This may be interpreted by the caller as
an error or as a warning.

Note that this function only deletes the memory that was part of the symbol table. The
memory allocated by the caller and handled via the pointer T<value> usually returned by
R<sym_LookupSymbol()> should be released by the caller.

CUT*/
  pSymbol *work_pointer,*swp;
  pSymbol sym;
  int k;

  /* convert the string to lower case to make it case insensitive */
  _to_lower(s);

  k = hashpjw( s );
  work_pointer = swp = &(hashtable[ k ]);
  while( *work_pointer && (k=strcmp(s,(*work_pointer)->name)) )
    work_pointer= k > 0 ? &((*work_pointer)->big_son)  :
        &((*work_pointer)->small_son);

  /* if the symbol is not in the table there is nothing to do */
  if( *work_pointer == NULL )return 1;
  sym = *work_pointer;
  /* hook off this element */
  *work_pointer = NULL;

  if( sym->big_son ){
    *work_pointer = sym->big_son;
    if(  sym->small_son == NULL ){
      memory_releasing_function(sym->name,pMemorySegment);  
      memory_releasing_function(sym,pMemorySegment);
      return 0;/* there is big_son, but no small_son */
      }
    }else{
    /* if there is no big_son */
    if( sym->small_son ){
      *work_pointer = sym->small_son;
      memory_releasing_function(sym->name,pMemorySegment);  
      memory_releasing_function(sym,pMemorySegment);
      return 0;/* there is no big_son, small_son hooked */
      }
    /* there are no sons at all */
    memory_releasing_function(sym->name,pMemorySegment);  
    memory_releasing_function(sym,pMemorySegment);
    return 0;
    }

  /* start from the root again and find the new place for the small_son */
  work_pointer = swp;
  while( *work_pointer && (k=strcmp(sym->small_son->name,(*work_pointer)->name)) )
    work_pointer= k > 0 ? &((*work_pointer)->big_son)  :
        &((*work_pointer)->small_son);

  /* hook the small son on it */
  *work_pointer = sym->small_son;

  /* finally erase the symbol */
  memory_releasing_function(sym->name,pMemorySegment);  
  memory_releasing_function(sym,pMemorySegment);  
  
  return 0;
}
