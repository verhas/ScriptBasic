/*
FILE:   command.c
HEADER: command.h

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

This is a C file that contains only header information. Use headerer.pl to
create command.h

We do not created command.h using an editor because all .h files may be deleted
in this project during cleanup and we may loose this file accidentally.

TO_HEADER:
#include "errcodes.h"
#include "report.h"
#include "sym.h"
#include "lexer.h"
#include "expression.h"
#include "builder.h"
#include "memory.h"
#include "syntax.h"
#include "execute.h"
#include "syntax.h"
#include "myalloc.h"
#include "filesys.h"
#include "options.h"
#include "hookers.h"

#include "notimp.h"

#define COMMAND(x) void COMMAND_##x(pExecuteObject pEo){\
                     MortalList _ThisCommandMortals=NULL;\
                     pMortalList _pThisCommandMortals = &_ThisCommandMortals;\
                     unsigned long _ActualNode=PROGRAMCOUNTER;\
                     int iErrorCode;

#define IDENTICAL_COMMAND(x) COMMAND_##x(pEo);

#define USE_CALLER_MORTALS (_pThisCommandMortals = pEo->pGlobalMortalList)

#define END goto _FunctionFinishLabel;\ // this is to avoid warnings on unrefereced labels
            _FunctionFinishLabel: \
            memory_ReleaseMortals(pEo->pMo,&_ThisCommandMortals);\
            iErrorCode = 0;\ // this is to avoid warnings on unreferenced variable
            FINISH;\
            }
// some commands may redefine this macro to execute some finishing code
#define FINISH

#define RETURN goto _FunctionFinishLabel
#ifdef ERROR
#undef ERROR
#endif
#define ERROR(x) do{ pEo->ErrorCode = x; RETURN; }while(0)

#define NOTIMPLEMENTED ERROR(COMMAND_ERROR_NOTIMP)

#define DEREFERENCE(X)   refcount = pEo->pMo->maxderef;\
  while( *(X) && TYPE( *(X) ) == VTYPE_REF ){\
    (X) = (*(X))->Value.aValue;\
    if( ! refcount -- )ERROR(COMMAND_ERROR_CIRCULAR);\
    }

// Raise error or return undef based on option setting.
#define ERRORUNDEF if( (*RaiseError(pEo))&1 ){\
                     ERROR(COMMAND_ERROR_DIV);\
                     }\
                   RESULT = NULL; RETURN;

// Raise error or return undef if macro argument is NULL
#define NONULOP(x) ASSERTOKE;if( memory_IsUndef(x) ){if((*RaiseError(pEo))&2 ){\
                     ERROR(COMMAND_ERROR_UNDEFOP);\
                     }\
                   RESULT = NULL; RETURN;}

// Raise error if macro argument is NULL for compare operators
#define NONULOPE(x) ASSERTOKE;if( memory_IsUndef(x) ){if( (*RaiseError(pEo))&4 )ERROR(COMMAND_ERROR_UNDEFOP);}

// Return argument as a double or if this is a long value then as a long.
#define RETURN_DOUBLE_VALUE_OR_LONG(x) \
                   dResult = (x);\
                   if( dResult == floor(dResult) && fabs(dResult) < LONG_MAX ){\
                     RESULT= NEWMORTALLONG;\
                     ASSERTNULL(RESULT);\
                     LONGVALUE(RESULT) = ((long)dResult);\
                     RETURN;\
                     }\
                   RESULT = NEWMORTALDOUBLE;\
                   ASSERTNULL(RESULT);\
                   DOUBLEVALUE(RESULT) = dResult;\
                   RETURN;

#define RETURN_DOUBLE_VALUE(x) \
                   RESULT = NEWMORTALDOUBLE;\
                   ASSERTNULL(RESULT);\
                   DOUBLEVALUE(RESULT) = (x);\
                   RETURN;

// Return argument as a long. 
#define RETURN_LONG_VALUE(x) \
                   RESULT= NEWMORTALLONG;\
                   ASSERTNULL(RESULT);\
                   LONGVALUE(RESULT) = (x);\
                   RETURN;

#define RETURN_UNDEF RESULT = NULL; RETURN

#define RETURN_TRUE \
                   RESULT= NEWMORTALLONG;\
                   ASSERTNULL(RESULT);\
                   LONGVALUE(RESULT) = -1L;\
                   RETURN;

#define RETURN_FALSE \
                   RESULT= NEWMORTALLONG;\
                   ASSERTNULL(RESULT);\
                   LONGVALUE(RESULT) = 0L;\
                   RETURN;

#define PROGRAMCOUNTER (pEo->CommandArray[pEo->ProgramCounter-1].Parameter.NodeList.actualm)
#define SETPROGRAMCOUNTER(x) ( pEo->fNextPC=1 , pEo->NextProgramCounter = (x) )

#define NEXTPARAMETER (_ActualNode = pEo->CommandArray[_ActualNode-1].Parameter.CommandArgument.next)
#define PARAMETERNODE (pEo->CommandArray[_ActualNode-1].Parameter.CommandArgument.Argument.pNode)
#define PARAMETERLONG (pEo->CommandArray[_ActualNode-1].Parameter.CommandArgument.Argument.lLongValue)
#define PARAMETERDOUBLE (pEo->CommandArray[_ActualNode-1].Parameter.CommandArgument.Argument.dDoubleValue)
#define PARAMETERSTRING (pEo->StringTable+pEo->CommandArray[_ActualNode-1].Parameter.CommandArgument.Argument.szStringValue)
#define PARAMETERSTRLEN (*((long *)(pEo->StringTable+pEo->CommandArray[_ActualNode-1].Parameter.CommandArgument.Argument.szStringValue-sizeof(long))))
#define PARAMETEREXEC   (pEo->InstructionParameter[_ActualNode-1])

#define OPCODE(x) (pEo->CommandArray[x-1].OpCode)
#define THISPARAMPTR (pEo->CommandParameter[pEo->CommandArray[_ActualNode-1].OpCode - START_CMD])
#define PARAMPTR(x)  (pEo->CommandParameter[(x) - START_CMD])
#define FINALPTR(x)  (pEo->Finaliser[(x) - START_CMD])
#define ALLOC(x) alloc_Alloc((x),pEo->pMemorySegment)
#define FREE(x)  alloc_Free((x),pEo->pMemorySegment)

#define PARAMETERLIST (pEo->CommandArray[pEo->OperatorNode-1].Parameter.Arguments.Argument)

#define NEWLONG   memory_NewLong(pEo->pMo)
#define NEWDOUBLE memory_NewDouble(pEo->pMo)
#define NEWSTRING(length) memory_NewString(pEo->pMo,length)
#define NEWARRAY(low,high) memory_NewArray(pEo->pMo,low,high)

#define NEWMORTALLONG   memory_NewMortalLong(pEo->pMo,_pThisCommandMortals)
#define NEWMORTALDOUBLE memory_NewMortalDouble(pEo->pMo,_pThisCommandMortals)
#define NEWMORTALSTRING(length) memory_NewMortalString(pEo->pMo,length,_pThisCommandMortals)
#define NEWMORTALARRAY(low,high) memory_NewArray(pEo->pMo,_pThisCommandMortals,low,high)

#define CONVERT2DOUBLE(x)  execute_Convert2Double(pEo,x,_pThisCommandMortals)
#define CONVERT2LONG(x)    execute_Convert2Long(pEo,x,_pThisCommandMortals)
#define CONVERT2STRING(x)  execute_Convert2String(pEo,x,_pThisCommandMortals)
#define CONVERT2NUMERIC(x) execute_Convert2Numeric(pEo,x,_pThisCommandMortals)

#define CONVERT2ZCHAR(x,y)   (y) = ALLOC(STRLEN( (x) )+1);\
                             if( (y) == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);\
                             memcpy((y),STRINGVALUE( (x) ),STRLEN( (x) ));\
                             (y)[STRLEN( (x) )] = (char)0;


#define ISSTRINGINTEGER(x) execute_IsStringInteger(x)
#define ISINTEGER(x) execute_IsInteger(x)

#define RESULT pEo->pOpResult

#define CAR(x) (((x)>0) ? pEo->CommandArray[(x)-1].Parameter.NodeList.actualm : 0)
#define CDR(x) (((x)>0) ? pEo->CommandArray[(x)-1].Parameter.NodeList.rest : 0)

#define EVALUATEEXPRESSION(x) memory_DupMortalize(pEo->pMo,\
                                            execute_Dereference(pEo,\
                                            execute_Evaluate(pEo,x,_pThisCommandMortals,&iErrorCode,0),&iErrorCode),\
                                            _pThisCommandMortals,\
                                            &iErrorCode)
#define _EVALUATEEXPRESSION(x) execute_Dereference(pEo,execute_Evaluate(pEo,x,_pThisCommandMortals,&iErrorCode,0),&iErrorCode)
#define _EVALUATEEXPRESSION_A(x) execute_Dereference(pEo,execute_Evaluate(pEo,x,_pThisCommandMortals,&iErrorCode,1),&iErrorCode)
#define EVALUATELEFTVALUE(x) execute_LeftValue(pEo,x,_pThisCommandMortals,&iErrorCode,0)
#define EVALUATELEFTVALUE_A(x) execute_LeftValue(pEo,x,_pThisCommandMortals,&iErrorCode,1)

#define ASSERTOKE if( iErrorCode )ERROR(iErrorCode); // use this macro after each EVAL to check that no error occured
#define ASSERTNULL(X) if( (X) == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);

#define IMMORTALIZE(x) memory_Immortalize(x,_pThisCommandMortals)

typedef unsigned long NODE;
typedef pFixSizeMemoryObject VARIABLE, *LEFTVALUE;
#define TYPE(x) ((x)->vType)

#define OPTION(x) options_Get(pEo,(x))
#define OPTIONR(x) options_GetR(pEo,(x))
*/

/*POD
=H Header file for command building

This file contains macros that help the command implementators to write easy, readable and
compact code. The macros on one hand assume some variable naming, but on the other hand
hide some details of the function calls.

For examples how to use these macros see the files in the T<commands> directory of the
source files. Note that some command implementation do not use these macros, because they
do sophisticated and low level operations that have to deal with the interpreted
code in more detail. However such a programming should not be neccesary to implement
a new command or function for extending the language.

To implement a new command or function do the following:

=itemize
=item Read the definitions here. 
=item Read the macros and see the implemented functions and commands in the directory T<commands>. 
      Try to get some understanding how it works.
=item Take an already implemented function which is similar to the one that you want to implement. Copy
      it to a new file, give it a new name. 
=item Edit the T<syntax.def> file to include the new command or file and run T<syntaxer.pl syntax.def>
      to generate the files T<syntax.h> and T<syntax.def>
=item Compile ScriptBasic to see that the function with the new name or the command has the same
      functionality as the old one that you have copied.
=item Start modifying the code step by step. At each step compile the interpreter and check that the modified
      functionality exists.
=item Debug, crosscheck the final code and document your new command or function for your project.
=item Announce the new functionality in your project and be proud.
=item Be happy.
=noitemize

B<NOTE:>

This file is actually a header file. This is maintained in T<command.c> to avoid
accidental deletion of the T<command.h> file. The file T<command.h> is an intermediate file
created from T<command.c> using the Perl utility T<headerer.pl>. Because all T<*.h> files
are intermediate it would have been dangerous to have T<command.h> to be a source file.



The macros and types defined in this file:

CUT*/

/*POD
=section COMMAND
=H Start a command implementation

T<COMMAND>

This macro should be used to start a function that implements a command or built-in function.
This actually generates the function header with some local variable declarations and some 
variable setting.

=verbatim
COMMAND(FUNCTIONNAME)
=noverbatim

in the current implementation generates:

=verbatim
void COMMAND_FUNCTIONNAME(pExecuteObject pEo){
  MortalList _ThisCommandMortals=NULL;
  pMortalList _pThisCommandMortals = &_ThisCommandMortals;
  unsigned long _ActualNode=PROGRAMCOUNTER;
  int iErrorCode;
=noverbatim

Note that further implemenation changes may change the actual code generated not followed in this
documentation. However the actual use of the macro should not change.

The function should be finished using the macro R<END> documented also in this documentation.

CUT*/

/*POD
=section END
=H Finish a command implementation

This macro generates the finishing code that a function impementing a BASIC command or built-in function
should have.

=verbatim
END 
=noverbatim

in the current implementation generates the following code:

=verbatim
goto _FunctionFinishLabel;
_FunctionFinishLabel:
memory_ReleaseMortals(pEo->pMo,&_ThisCommandMortals);
iErrorCode = 0;
}
=noverbatim

Note that further implemenation changes may change the actual code generated not followed in this
documentation. However the actual use of the macro should not change.

Some part of the code may seem unneccesary. The T<goto> just before the label, or the final assignment
to a local variable. These are present to avoid some compiler warning and clever compilers should
optimize out these constructs.

CUT*/

/*POD
=section IDENTICAL_COMMAND
=H Implement a command that has identical functionality

This macro helps to implement a command that has identical functionality as
another command. You can see examples for it in the looping construct. There is a wide
variety of looping construct to implement all looping facilities that BASIC programmers
got used to. However the loop closing commands more or less behave the same. For example
the command T<next> behaves exactly the same as the command T<while>

Also note that the identical behaviour does not mean that one command can be used instead
of the other. There are conventions that the BASIC language requires and the syntactical
analyzer does nto allow to close a T<FOR> loop using a T<WEND> command.

To present an example on how to use this macro we have copied the code of the comman T<NEXT>:

=verbatim
COMMAND(NEXT)

  IDENTICAL_COMMAND(WEND)

END
=noverbatim

CUT*/

/*POD
=section USE_CALLER_MORTALS
=H Use the mortals of the caller

=verbatim
USE_CALLER_MORTALS
=noverbatim

You should use this macro when impementing a built-in function. The implementation
of the commands use their own mortal list to collect mortal variables storing
intermediate results. Built-in function implementations do NOT maintain their own
collection of mortal variables. This macro sets some variables to collect mortal
variables into the list of the calling modules.

To get a deeper understanding of mortals and variable handling see the documentation
for the source file T<memory.c>

CUT*/

/*POD
=section RETURN
=H Return from the function

=verbatim
RETURN
=noverbatim

When implementing a built-in function or command you should never ever T<return> from
the function because that may avoid release of mortal variables and may not execute
the final code which is needed to properly finish the function. Use the macro T<RETURN>
instead.
CUT*/

/*POD
=section ERROR
=H Terminate a function with error

=verbatim
ERROR(x)
=noverbatim

Use this macro to terminate the execution of a commans or built-in function with some
error. T<ERROR(0)> means no error, but this construct is not advisable, use R<RETURN> instead.
Any other code value can be used to specify a special error.

CUT*/

/*POD
=section PROGRAMCOUNTER
=H The value of the programcounter

=verbatim
PROGRAMCOUNTER
=noverbatim

This macro results the node id of the command, which is currently executed. Note that this is
already the node that contains the command code and not the code that the class variable T<ProgramCounter>
points. T<ProgramCounter> points to a list node. This list node points to the node returned by
T<PROGRAMCOUNTER> and to the next command node.

CUT*/

/*POD
=section SETPROGRAMCOUNTER
=H Implement jump instructions

=verbatim
SETPROGRAMCOUNTER(x)
=noverbatim

Use this macro when a command decides that the code interpretation should
continue at a different location. The simplest example on how to use this
macro is the implementation of the command T<goto>:

=verbatim
COMMAND(GOTO)

  SETPROGRAMCOUNTER(PARAMETERNODE);

END
=noverbatim

See also R<PARAMETERNODE>.

CUT*/

/*POD
=section NEXTPARAMETER
=H Get the next command parameter

=verbatim
NEXTPARAMETER
=noverbatim

This macro should be used to get access to the next command parameter. This macro should NOT be
used in built-in function implemenation. The functions have only a single parameter, which is indeed
an expression list. To access the parameters of a function use the macros R<PARAMETERLIST>, R<CAR> and
R<CDR>.

When you implement a command you can get the first parameter of a command using the macro
T<PARAMETERNODE>, T<PARAMETERLONG>, T<PARAMETERDOUBLE> or T<PARAMETERSTRING> (see R<PARAMETERXXX>). 
If the command has more than one parameters you should use the macro T<NEXTPARAMETER>
to step to the next parameter.

CUT*/
/*POD
=section PARAMETERXXX
=H Access a command parameter

=verbatim
PARAMETERNODE
PARAMETERLONG
PARAMETERDOUBLE
PARAMETERSTRING
=noverbatim

You should use these macros to get access to the command parameters. Usually these parameters are
presented as "nodes". Syntax definition usually allows you to use expressions whenever a long, double
or string is expected. Expressions are converted to "nodes" and therefore only a few commands may
use the macros T<PARAMETERLONG>, T<PARAMETERDOUBLE> or T<PARAMETERSTRING>. These can be used when the
parameter is a long number, double number or a contant string and NOT an expression or expression list.

When a command has more than one parameters you can access each, step by step using the macro R<NEXTPARAMETER>,
which steps onto the next parameter of the command.

Do B<NOT> use any of the T<PARAMETERXXX> macro or the macro R<NEXTPARAMETER> in the implementation
of a built-in function.
CUT*/
/*POD
=section OPCODE
=H Get the opcode of a node

=verbatim
OPCODE(x)
=noverbatim

This macro results the opcode of the node T<x>. This macro can rarely be used by your extension.

CUT*/

/*POD
=section PARAMETERLIST
=H Get the parameter list node for a function

=verbatim
PARAMETERLIST
=noverbatim

You should use this macro to get the parameter list for a built-in function or operator.
Both built-in functions and operators get their parameter list as and expression list. This
macro gets the first list node of the expression list.

The parameter is presented as an expression list even if there is only a single parameter
for the function.

To access the parameters use the macros R<CAR> and R<CDR>.
CUT*/

/*POD
=section CAR
=H Get the car node of a list node

Expression lists and commands are stored using list nodes. A list node has an R<OPCODE> value
T<eNTYPE_LST> defined in T<expression.c>, and has two node pointers. One points to the node
that belongs to the list member and other points to the next list node.

If T<nItem> is a list node for an expression list then T<CAR(nItem)> is the root node of the
expression, and T<CDR(nItem)> is the list node for the next expression. T<CAR(CDR(nItem))> is
the root node of the second expression.

The nodes are indexed with T<unsigned long> values. T<NULL> pointer is a T<0L> value and list
node lists are terminated with a node that has T<CDR(nItem)=0L>.

See also R<CDR>.
CUT*/

/*POD
=section CDR
=H Get the cdr node of a list node

Expression lists and commands are stored using list nodes. A list node has an R<OPCODE> value
T<eNTYPE_LST> defined in T<expression.c>, and has two node pointers. One points to the node
that belongs to the list member and other points to the next list node.

If T<nItem> is a list node for an expression list then T<CAR(nItem)> is the root node of the
expression, and T<CDR(nItem)> is the list node for the next expression. T<CAR(CDR(nItem))> is
the root node of the second expression.

The nodes are indexed with T<unsigned long> values. T<NULL> pointer is a T<0L> value and list
node lists are terminated with a node that has T<CDR(nItem)=0L>.

See also R<CAR>.
CUT*/

/*POD
=section RESULT
=H Special variable to store the result

=verbatim
RESULT
=noverbatim

Use this macro to store the result of the operation. Usually a new mortal value should be
allocated using R<NEWMORTALXXX> and the appropriate value of T<RESULT> should be then set.

See also R<NEWMORTALXXX>, R<XXXVALUE>

CUT*/

/*POD
=section XXXVALUE
=H Access certain values of a memory object

=verbatim
STRINGVALUE(x)
LONGVALUE(x)
DOUBLEVALUE(x)
=noverbatim

These macros are actually defined in T<memory.c>, but we document them here because they play
an important role when writing implementation code for functions and operators.

These macros get the string (car*), long or double value of a variable. The macros can also be
used to assign value a long or double value to a variable. Do not forget to change the type of the
variable. You usually should call the macro R<CONVERT2XXX>.

Note that you should NOT change the string value of a variable. The T<STRINGVALUE(x)> is a (char *)
pointer to a string. You have to change the characters in this string, or you should allocate a new
string with longer or shorter length and copy the characters, but never change the (char *) pointer.

CUT*/

/*POD
=section NEWMORTALXXX
=H Create a new mortal value

=verbatim
NEWMORTALLONG
NEWMORTALDOUBLE
NEWMORTALSTRING(length)
=noverbatim

Use these macros to allocate a new mortal variable. In case of a string you have to give
the length of the string. 

=center
INCLUDE THE TERMINATING ZERO IN THE LENGTH!!!
=nocenter

Never allocate non-mortal values when implemenating operators or functions.

CUT*/

/*POD
=section EVALUATEEXPRESSION
=H Evaluate an expression

=verbatim
EVALUATEEXPRESSION(x)
_EVALUATEEXPRESSION(x)
=noverbatim

Use these macros to evaluate an expression. The argument is the root node of the expression.
When a command has a parameter, which is an expression you should write:

=verbatim
  VARIABLE Param;

  Param = EVALUATEEXPRESSION(PARAMETERNODE);
=noverbatim

Implementing a function or operator you should write

=verbatim
  VARIABLE Param;

  Param = EVALUATEEXPRESSION(CAR(PARAMETERLIST));
=noverbatim

For further details see examples in the source files T<commands/let.c>, T<commands/mathops.c>.

B<NOTE:>

When an expression is evaluated the returned pointer points to a struct which contains the value
of the expression. This is usually a mortal variable which was created during expression evaluation.
However in some cases this pointer points to a nonmortal variable. If the expression is a single 
global or local variable the result of the expression is the pointer to the variable value.

This helps writing for more efficient code. On ther other hand operators tend to convert the operands
to long or double in case they expect a long or double but get a different type. The conversions, or
other manipulations then change the original variable value, which is a side effect. For this reason
the macro T<EVALUATEEXPRESSION> also calls T<memory_DupMortalize> which creates a new variable, and 
copies the content of the variable passed as argument if the variable is not a mortal.

T<_EVALUATEEXPRESSION> does not do this and therefore you can use it for more efficient memory
handling avoiding the creation of unneccesary copies of variables.

If you are not sure whichone to use use the first one without the leading underscore.

CUT*/
/*POD
=section EVALUATELEFTVALUE
=H Evaluate a left value
 
=verbatim
EVALUATELEFTVALUE(x)
=noverbatim

Use this macro to evaluate a left value. This is done in the function T<commands/let.c> that implements
the command LET. This command wasprogrammed with comments to help the learning how this works.

CUT*/

/*POD
=section IMMORTALIZE
=H Immortalize a variable

=verbatim
IMMORTALIZE(x)
=noverbatim

Use this macro to immortalize a variable.You usually should avoid immortalizing values when implementing
operators of functions. Immortalize a value when it is assigned to a variable.

CUT*/

/*POD
=section NEWXXX
=H Create a new immortal value

=verbatim
NEWLONG
NEWDOUBLE
NEWSTRING(length)
=noverbatim

Use these macros to create a new long, double or string value. The created value is NOT mortal.
You usually should avoid this when implementing functions or operators. However you may need in
some command implementation.

CUT*/

/*POD
=section CONVERT2XXX
=H Convert a value to other type

=verbatim
CONVERT2DOUBLE(x)
CONVERT2LONG(x)
CONVERT2STRING(x)
=noverbatim

Use these macros to convert a value to long, double or string. The macros return the pointer to the
converted type. Note that conversion between long and double does not generate a new value. In such
a conversion the argument pointer is returned and the value itself is converted from one type tp the
other. This was programmed this way to avoid unneccesary creation of mortal values. However be sure
that either the argument is a mortal value or you do not mind the conversion of the value and so the value
of the variable that the value was assigned to. You need not worry about this when you use the macro
T<EVALUATEEXPRESSION> and not T<_EVALUATEEXPRESSION>.

Also note that a conversion does not duplicate the value if the value already has the desired type. In such
a case the argument pointer is returned.

On the other hand there is no guarantee that the conversion is done in-place. When conversion from
string to anything else is done a new mortal variable is allocated and the pointer to that
value is returned.

CUT*/

/*POD
=section PARAMPTR
=H Parameter pointer

=verbatim
PARAMPTR(x)
THISPARAMPTR
=noverbatim

Each command may need some parameters that are persistent during a program execution. For example
the file handling routines need an array that associates the opened file handles with the integer
values that the basic language uses. Each command may allocate storage and assign a pointer to
T<THISPARAMPTR> to point to the allocated space. This is a void pointer initialized to T<NULL>.

A command may access a pointer of another command using the macro T<PARAMPTR(x)> supplying
I<x> as the command code. This is usually T<CMD_XXX> with I<XXX> the name of the command, function or
operator.

See also R<ALLOC> and R<FREE>
CUT*/

/*POD
=section ALLOC
=H Allocate memory

The T<ALLOC> and R<FREE> macros are provided to allocate general memory. They are not intended to create
new variable storage. For that purpose the R<NEWXXX> and R<NEWMORTALXXX> macros should be
used.

The memory allocated should usually be assigned to the T<THISPARAMPTR> pointer (see R<PARAMPTR>).

The macro T<ALLOC> behaves similar to the system function T<malloc> accepting the size of the required memory 
in bytes and returning a void pointer.

The macro R<FREE> accepts the pointer to the allocated memory and returns nothing.

There is no need to release the allocated memory. The memory allocated using T<ALLOC> is automatically
release upon program termination.

CUT*/

/*POD
=section FREE
=H Release memory


The R<ALLOC> and T<FREE> macros are provided to allocate general memory. They are not intended to create
new variable storage. For that purpose the R<NEWXXX> and R<NEWMORTALXXX> macros should be
used.

The memory allocated should usually be assigned to the T<THISPARAMPTR> pointer (see R<PARAMPTR>).

The macro R<ALLOC> behaves similar to the system function T<malloc> accepting the size of the required memory 
in bytes and returning a void pointer.

The macro T<FREE> accepts the pointer to the allocated memory and returns nothing.

There is no need to release the allocated memory. The memory allocated using R<ALLOC> is automatically
release upon program termination.
CUT*/

/*POD
=section ISSTRINGINTEGER
=H Decide if a string is integer or not

=verbatim
ISSTRINGINTEGER(x)
=noverbatim

Use this macro to decide that a string contains integer value or not. This can beuseful implementing
operators that work with doubles as well as longs. When one of the operators is a string hopefully
containing the decimal form of a number you have to convert the string to a long or double. This
macro calls a function that decides whether the string contains an integer number convertible to
long or contains a double.

For an example how to use it see the source file  T<commands/mathops.c>
CUT*/

/*POD
=section NODE
=H Basic C variable types to be used

=verbatim
NODE nNode;
VARIABLE Variable;
LEFTVALUE Lval;
=noverbatim

These T<typedef>s can be used to declare C variables that are to hold value associated with nodes,
variables (or values) and left values. For example how to use these T<typedef>s see the files
T<commands/let.c>, T<commands/mathops.c>

CUT*/

/*POD
=section TYPE
=H Get the actual type of a value

=verbatim
TYPE(x)
=noverbatim

Use this macro to access the type of a value. Values can hold long, double, string or reference types.
This macro returns the type of the value. For comparision use the constants:

=itemize
=item T<VTYPE_LONG> long value
=item T<VTYPE_DOUBLE> double value
=item T<VTYPE_STRING> string value
=item T<VTYPE_REF> reference value
=noitemize

CUT*/
