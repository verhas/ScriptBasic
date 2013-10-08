/*
FILE:   options.c
HEADER: options.h

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

Handling options

TO_HEADER:

*/


#include <stdio.h>
#include <stdlib.h>

#include "sym.h"
#include "errcodes.h"
#include "report.h"
#include "lexer.h"
#include "expression.h"
#include "builder.h"
#include "memory.h"
#include "syntax.h"
#include "execute.h"
#include "myalloc.h"
#include "command.h"

/*POD
@c Setting and getting option values

Each BASIC interpreter maintains a symbol table holding option values.
These option values can be set using the BASIC command T<OPTION> and an
option value can be retrieved using the function T<OPTION()>.

An option has an integer value (T<long>). Options are usually used to
alter the behaviour of some commands or modules, altough BASIC programs
are free to use any string to name an option. For example the option
T<compare> may alter the behavior of the string comparision function
to be case sensitive or insensitive:

=verbatim
OPTION compare 1
=noverbatim

Unitialized options are treated as being zero. There is no special option value for uninitialized 
options. In other words BASIC programs can not distinguish between unitialized options and
options having the value zero.

This file contains the functions that handle the option symbol table. The option
symbol tableis pointed by the field T<OptionsTable> of the execution object. This
pointer is initialized to be T<NULL>, which means no options are available, or in other
words all options are zero.
CUT*/

/*POD
=H options_Reset
@c Clear an option data

Calling this function resets an option. This means that the memory holding
the T<long> value is released and the pointer that was pointing to it is set T<NULL>.

/*FUNCTION*/
int options_Reset(pExecuteObject pEo,
                  char *name
  ){
/*noverbatim
CUT*/
  void **p;

  /* If there is no any option then the actual option need not be reset. */
  if( pEo->OptionsTable == NULL )return 0;
  p = sym_LookupSymbol(name,pEo->OptionsTable,0,alloc_Alloc,alloc_Free,pEo->pMemorySegment);
  /* If the option is not in the symbol table it need not be reset. */
  if( p == NULL )return 0;
  /* If the option is in the symbol table with NULL value then it IS already reset. */
  if( *p == NULL )return 0;
  /* release the memory (a long) */
  alloc_Free(*p,pEo->pMemorySegment);
  /* do not point to the released long */
  *p = NULL;
  return 0;
  }

/*POD
=H options_Set
@c Set option data

This function sets a long value for an option. If the option
did not exist before in the symbol table it is inserted. If the
symbol table was empty (aka T<OptionsTable> pointed T<NULL>) the
symbol table is also created.

If the symbol already existed with some T<long> value then the new value
is stored in the already allocated place and thus the caller
may store the pointer to the long returned by R<GetR> and access 
possibly updated data without searching the table again and again.

/*FUNCTION*/
int options_Set(pExecuteObject pEo,
                char *name,
                long value
  ){
/*noverbatim
The function returns zero if the option was set or T<1> if there was a memory failure.
CUT*/
  void **p;

  /* create the symbol table if it does not exist */
  if( pEo->OptionsTable == NULL )
    pEo->OptionsTable = sym_NewSymbolTable(alloc_Alloc,pEo->pMemorySegment);
  if( pEo->OptionsTable == NULL )return 1; /* In case of memory failure. */

  /* lookup the option and insert it into the table in case it is not there yet */
  p = sym_LookupSymbol(name,pEo->OptionsTable,1,alloc_Alloc,alloc_Free,pEo->pMemorySegment);
  if( p == NULL )return 1; /* In case of memory failure. */

  /* If the option was not defined before. */
  if( *p == NULL ){
    *p = alloc_Alloc(sizeof(long),pEo->pMemorySegment);
    if( *p == NULL )return 1; /* In case of memory failure. */
    }
  /* store the value */
  *((long *)*p) = value;
  return 0;/*OK*/
  }

/*POD
=H options_Get
@c Get option data

This function retrieves and returns the value of an option data.

/*FUNCTION*/
long options_Get(pExecuteObject pEo,
                 char *name
  ){
/*noverbatim
The return value is the option value or zero in case the option is not set.
CUT*/
  void **p;

  if( pEo->OptionsTable == NULL )return 0L;
  p = sym_LookupSymbol(name,pEo->OptionsTable,0,alloc_Alloc,alloc_Free,pEo->pMemorySegment);
  if( p == NULL )return 0L;
  if( *p == NULL )return 0L;

  return *((long *)*p);
  }

/*POD
=H options_GetR
@c Get option data

This function retrieves and returns the value of an option data.

/*FUNCTION*/
long *options_GetR(pExecuteObject pEo,
                 char *name
  ){
/*noverbatim
The return value is a T<long *> pointer to the option value or T<NULL> if the option is
not set. If the caller sets the T<long> variable pointed by the returned pointer the value of the
option is changed directly.
CUT*/
  void **p;

  if( pEo->OptionsTable == NULL )return NULL;
  p = sym_LookupSymbol(name,pEo->OptionsTable,0,alloc_Alloc,alloc_Free,pEo->pMemorySegment);
  if( p == NULL )return NULL;
  if( *p == NULL )return NULL;

  return ((long *)*p);
  }
