/*external.c

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

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../command.h"
#include "../dynlolib.h"
#include "../basext.h"
#include "../builder.h"
#include "../modumana.h"

/*POD
=H EXTERNAL

This file contains the implementation of the commands that handle the load
and execution of external functions. These functions are stored in dynamically
loaded libraries and are loaded on demand when they are first called.

CUT*/



/* structure to store the function entry point and the module structure address for the function */
typedef struct _InstructionData {
  void *FunctionPointer;
  pModule ModulePointer;
  } InstructionData, *pInstructionData;

/**DECLARESUB
=section misc
=display DECLARE SUB
=title DECLARE SUB function ALIAS cfun LIB library

Declare a function implemented in an external library. Note that library and the function implemented in it should be specifically written for ScriptBasic. This command is not able to declare and call just any library function.

The command has three arguments. The argument T<function> is a symbol (a case insensitive string without any space, containing only alphanumeric characters and without double quotes) , the name if the function as it is going to be used in the BASIC program. The BASIC programmer is free to choose this function name. The function later can be used as if it was a BASIC user defined function.

The argument T<cfun> has to be the name of the function as it is defined in the library. This has to be a constant string value. You can not use any expression here but a constant string. This is usually the same as the name of the interface function in the C source file. The programmer writing the module in C has to know this name. If for some reason you do not happen to know it, but you need to declare the function you may be lucky using the name that stands in the C source file between parentheses after the macro T<besFUNCTION>.

The last argument is the name of the library. This also has to be a constant string value. You can not use any expression here but a constant string. This argument has to specify the name of the library file without the extension and the path where the library is located. The extension will automatically be appended to the file name and the path will automatically be prepended to it. The actual extension and the path to be searched for the library is defined in the ScriptBasic configuration file.

You can also specify the full file name containing the full path to the library as well as the file extension. In this case the ScriptBasic configuration file data for the module path and extension is ignored.

=details

DEVELOPER DETAILS!

This command is used to start an external function defined in a dynamic load library.

The dynamically loaded modules are implemented in ScriptBasic via an ordinary command that has the syntax:

=verbatim
'declare' 'sub' * function 'alias' string 'lib' string nl
=noverbatim

For the compiler it generates a user defined function, which is defined on the line that contains the declare statement. The execution system will call itself recursively to execute lines starting at the line where the T<declare> statement is. The command implemented in this file is executed and unlike the T<FUNCTION> or T<SUB> it immediately tells the execution system to return after the line was executed.

This command first checks if the line was already executed. On the first execution it loads the module and gets the address of the function named in the alias string. This entry point is stored in a T<struct> and next time the function is called by the basic pointer it does not need to search for the function and for the module. If a function of an already loaded module is called the program does not reload the module. The program maintains a linked list with the names of the loaded modules and loads modules only when they are first referenced.

The modules are loaded using the operating system T<dll> loading function T<dlopen> or T<LoadLibrary>. These functions search several locations for libraries in case the library is specified without absolute path name.
The module loader can be fouled if the same library is defined with full path and with single name in the basic code.

For example, if the commands

=verbatim
declare sub fun2 alias "mefun" lib "libobas"
declare sub fun3 alias "youfun" lib "/usr/lib/scriba/libobas.so"
=noverbatim

refer to the same module, the code implemented here thinks that they are different libraries.

When the module is loaded the code tries to get the function named T<versmodu> and calls it. This function gets three arguments. The first argument is the interface version that ScriptBasic supports for the external modules. 
The second argument is a pointer to a ZCHAR terminated string that contains the variation of the calling interpreter. Note that this has to be an 8-character (+1 ZCHAR) string. The third argument is a pointer to a T<NULL> initialized T<void *> pointer, which is the module pointer. This pointer is stored by ScriptBasic, and it is guaranteed not been modified. The modules can store "global" variable information using this pointer. Usually this pointer is not used in this function, especially because there are no "safe" memory allocation functions available at this point of execution.

The module should examine the version it gets and it should decide if the interface version is OK for the module. If the interface version is not known the function should return 0 and ScriptBasic will interpret this value as a failure to load the module. If the module does not know if the interface version is good or not it can return the interface version that it can handle. In this case it is the duty of the interpreter to decide if the interface version can be provided or not. ScriptBasic will examine the version and in case it can not handle the version it will generate an error.

This methodology will allow either ScriptBasic to revert its functionality to earlier interface versions in case the higher version interface not only extends the lower version but is incompatible with the former version. On the other hand a module designed for a higher version may be loaded and executed by a lower version of ScriptBasic in case the module is ready to use the lower interface version.

If the function T<versmodu> can not be found in the DLL then ScriptBasic assumes that the module is ready to accept the current interface version. However such a module should not generally be written.

Note that the version we are talking about here is neither the version of ScriptBasic nor the version of the module. This is the version of the interface between the module and ScriptBasic.

After the version negotiation has been successfully done ScriptBasic tries to get the address of the function named T<bootmodu>. If this function exists it is started. This function can perform all the initializations needed for the module. This function gets all the parameters that a usual module implemented function except that there are no parameters and the function should not try to set a result value, because both of these arguments are T<NULL>.

A module function (including T<bootmodu>) is called with four arguments. The four arguments are four pointers.

=verbatim
int my_module_function(pSupportTable pSt,
                       void **ppModuleInternal,
                       pFixSizeMemoryObject pParameters,   // NULL for bootmodu
                       pFixSizeMemoryObject *pReturnValue) // NULL for bootmodu
=noverbatim

The parameter T<pSt> points to a T<struct> that holds the function pointers that the function can call to reach ScriptBasic functionality. These functions are needed to access the arguments and to return a value.

The parameter T<ppModuleInternal> is a pointer pointing to a T<NULL> initialized pointer. This pointer belongs to the module. ScriptBasic guarantees that the value is not modified during the execution (unless the module itself modifies it). This pointer can be used to remember the address space allocated by the module for itself. To store permanent values to remember the state of the module you can either use static variables or this pointer. However using this pointer and allocating memory to store the values is the preferred method. The reason for preferring this method is that global variables are global in the whole process. On the other hand ScriptBasic may run in several threads in a single process executing several different basic programs. This T<ppModuleInternal> pointer will be unique for each thread.

Here is the point to discuss a bit the role of T<bootmodu>. Why not to use T<DllMain> under Windows NT? The reason is in the possibility of threaded execution. T<DllMain> is executed when the process loads the T<dll>. T<bootmodu> is executed when the basic executor loads the module. If there are more threads running then T<DllMain> is not started again. Use T<DllMain> or a similar function under UNIX if you want to initialize some process level data. Use T<bootmodu> to initialize basic program specific data.

The parameter T<pParameters> is a ScriptBasic array containing the values of the arguments. There is no run-time checking about the number of arguments. This is up to the module function.

The last parameter is a pointer to a ScriptBasic variable. The initial value of the pointed pointer is T<NULL>, meaning T<undef> return value. The final return value should be allocated using the macros defined in T<basext.h> and this pointer should be set to point to the value. Note however that T<bootmodu> and T<finimodu> should not to try to dereference this variable, because for both of them the value is T<NULL>.

For further information on how to write module extension functions read the on-line documentation of T<basext.c> in this source documentation.

A module is unloaded when the basic program execution is finished. There is no basic code to unload a module. (Why?)

Before the module is unloaded calling one of the operating system functions T<dlclose> or T<FreeLibrary> the program calls the module function T<finimodu> if it exists. This function gets all the four pointers (the last two are T<NULL>s) and it can perform all the tasks that the module has to do clean up.

Note however that the module need not release the memory it allocated using the T<besALLOCATE> macro defined in the file T<basext.h> (which is generated using T<headerer.pl> from T<basext.c>). The memory is going to be released afterwards by ScriptBasic automatically.

You can have a look at the source code of modules provided by ScriptBasic, for example MySQL module, Berkeley DB module, HASH module, MT module and so on.

*/
void COMMAND_EXTERNAL(pExecuteObject pEo){
#if NOTIMP_EXTERNAL
NOTIMPLEMENTED;
_FunctionFinishLabel:;
#else
  MortalList _ThisCommandMortals=NULL;
  pMortalList _pThisCommandMortals = &_ThisCommandMortals;
  unsigned long _ActualNode=PROGRAMCOUNTER;
  pModule *ThisModule;
  int iErrorCode;
  char *pszLibraryFile;
  char *pszFunctionName;
  long NumberOfArguments;
  NODE nItem,nExpression;
  long i;
  void *FunctionPointer;
  pInstructionData *ppInstPointer;
  pFixSizeMemoryObject ParameterArray,SaveLocalVariables;
  int (*ExternalFunction)(pSupportTable, void **, pFixSizeMemoryObject, pFixSizeMemoryObject *);
  char *(*pfszErrMsg)(pSupportTable, void **, int);
  int iResult;

  /* if the execution is just running through the declaration */
  if( ! pEo->fWeAreCallingFuction )return;

  /* we need this variable, but should be set to NULL in case an error happens and the function returns 
     see more comment at the end of this function */
  SaveLocalVariables = pEo->LocalVariables;
  pEo->LocalVariables = NULL;

  /* The very first thing is to check if there was any external function call before, and
     if this is the first then initialize the data structure, which is needed to call external
     functions. */
  if( iResult = modu_Init(pEo,0) )ERROR(iResult);

  ppInstPointer = (pInstructionData *)&PARAMETEREXEC;
  if( *ppInstPointer ){/* if the function was already called and we already know the entry point */
    FunctionPointer = (*ppInstPointer)->FunctionPointer;
    ThisModule = &((*ppInstPointer)->ModulePointer);
    if( FunctionPointer == NULL )ERROR(COMMAND_ERROR_MODULE_FUNCTION);
    }else{
    /* get the name of the function */
    pszFunctionName = PARAMETERSTRING;
    NEXTPARAMETER;

    /* get the file name of the library where the function is defined */
    pszLibraryFile = PARAMETERSTRING;
    if( iResult =modu_GetFunctionByName(pEo,pszLibraryFile,pszFunctionName,&FunctionPointer,&ThisModule) )ERROR(iResult);

    if( FunctionPointer == NULL )ERROR(COMMAND_ERROR_MODULE_FUNCTION);
    *ppInstPointer = ALLOC(sizeof(InstructionData));
    if( *ppInstPointer == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
    (*ppInstPointer)->ModulePointer = *ThisModule;
    /* now the module is loaded, get the function pointer */
    (*ppInstPointer)->FunctionPointer = FunctionPointer;
    }

  /* count the function arguments */
  NumberOfArguments = 0L;
  for( nItem = pEo->FunctionArgumentsNode ; nItem ; nItem = CDR(nItem) )
    NumberOfArguments ++;

  /* Allocate the array to store the parameters. */
  if( NumberOfArguments ){
    ParameterArray = memory_NewArray(pEo->pMo,1,NumberOfArguments);
    if( ParameterArray == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
    }else ParameterArray = NULL;

  /* evaluate the arguments of the function */
  pEo->LocalVariables = SaveLocalVariables; /* Note that the pEo->LocalVariables pointer was set to NULL,
                                               because it has to be NULL when this command returns, and any
                                               error condition can return. (It took 4 hours to debug.)
                                            */
  nItem = pEo->FunctionArgumentsNode;
  i = 0;
  while( nItem ){
    i++ ;
    nExpression = CAR(nItem);
    switch( OPCODE(nExpression) ){
      case eNTYPE_ARR:
        ParameterArray->Value.aValue[i-1] = memory_NewRef(pEo->pMo);
        memory_SetRef(pEo->pMo,ParameterArray->Value.aValue+i-1,execute_LeftValueArray(pEo,nExpression,_pThisCommandMortals,&iErrorCode));
        break;
      case eNTYPE_SAR:
        ParameterArray->Value.aValue[i-1] = memory_NewRef(pEo->pMo);
        memory_SetRef(pEo->pMo,ParameterArray->Value.aValue+i-1,execute_LeftValueSarray(pEo,nExpression,_pThisCommandMortals,&iErrorCode));
        break;
      case eNTYPE_LVR:
        ParameterArray->Value.aValue[i-1] = memory_NewRef(pEo->pMo);
        memory_SetRef(pEo->pMo,ParameterArray->Value.aValue+i-1,&(pEo->LocalVariables->Value.aValue[pEo->CommandArray[nExpression-1].Parameter.Variable.Serial-1]));
        break;
      case eNTYPE_GVR:
        ParameterArray->Value.aValue[i-1] = memory_NewRef(pEo->pMo);
        memory_SetRef(pEo->pMo,ParameterArray->Value.aValue+i-1,&(pEo->GlobalVariables->Value.aValue[pEo->CommandArray[nExpression-1].Parameter.Variable.Serial-1]));
        break;
      default:
        ParameterArray->Value.aValue[i-1] = EVALUATEEXPRESSION(nExpression);
        ASSERTOKE;
        if( ParameterArray->Value.aValue[i-1] )
          memory_Immortalize(ParameterArray->Value.aValue[i-1],_pThisCommandMortals);
        break;
      }
    nItem = CDR(nItem);
    }
  ExternalFunction = FunctionPointer;
  pEo->pFunctionResult = NULL;
  memory_ReleaseMortals(pEo->pMo,&_ThisCommandMortals);
  /* we do not need the _pThisCommandMortal variable anymore. We use it to save the old value
     of the pGlobalMortalList, which is the mortal list of the caller.
  */
  _pThisCommandMortals = pEo->pGlobalMortalList;
  /* we set the pGlobalMortalList to point to the _ThisCommandMortals. This means that the
     interface function will use the mortal list of this command and not the mortal list
     of the caller. All mortal variables of the interface functionwill be released when this
     function ENDs.
  */
  pEo->pGlobalMortalList = &_ThisCommandMortals;
  /* We have to NULL the list of local variables, because it holds the old value,
     pointing to the array of local variables of the caller. After returning the execution
     code assumes that the pEo->LocalVariables points to the local variables array of the
     subroutine and releases it. Before inserting this line here the code released the caller
     local variables, and calling any external module from a function having local variables
     casued segmentation fault when the code returned from the external function and tried to
     access any local variable. If this variable is set to NULL the execution module assumes
     that there were no local variables of the called function. And we do not indeed have local
     variables in an external function, because they have quite different local variable mechanism,
     simply they have local C variables.
  */
  pEo->LocalVariables = NULL;
  (*ThisModule)->ModuleIsActive = 1;
  iResult = ExternalFunction(pEo->pST,&((*ThisModule)->ModuleInternalParameters),ParameterArray,&(pEo->pFunctionResult));
  pEo->pszModuleError = NULL;
  if( iResult ){
    pfszErrMsg = modu_GetModuleFunctionByName(*ThisModule,MODULE_ERRMSG);
    if( pfszErrMsg ){
     pEo->pszModuleError = pfszErrMsg(pEo->pST,&((*ThisModule)->ModuleInternalParameters),iResult);
     }
    }
  (*ThisModule)->ModuleIsActive = 0;
  /* The interface functions from the external modules return mortal values. We have to immortalize this
     variable otherwise it is freed by the statement hidden in the macro END at the end of this function.
  */
  if( pEo->pFunctionResult && IsMortal(pEo->pFunctionResult) )
      memory_Immortalize(pEo->pFunctionResult,pEo->pGlobalMortalList);

  if( ParameterArray )
    memory_ReleaseVariable(pEo->pMo,ParameterArray);

  /* here we restore the pGlobalMortalList so that the caller can use it as it was */
  pEo->pGlobalMortalList = _pThisCommandMortals;
  if( iResult )ERROR(iResult);
  pEo->fStop = fStopRETURN;

_FunctionFinishLabel:
  memory_ReleaseMortals(pEo->pMo,&_ThisCommandMortals);\
  iErrorCode = 0;
#endif
  }
/**DECLARECOMMAND
=section misc
=display DECLARE COMMAND
=title DECLARE COMMAND function ALIAS cfun LIB library

Declare a command implemented in an external library. Note that library and the command implemented in it should be specifically written for ScriptBasic. This command is not able to declare and call just any library function.

The arguments are similar to that of the command T<DECLARE SUB> and the same restrictions and conditions apply. Look at the documentation of the command R<DECLARESUB>.

=details

DEVELOPER DETAILS!

This command is used to start an external command defined in a dynamic load library. Please read the details of the command T<DECLARE SUB> if you did not do up to know before reading the details of this command.

The major difference between an external function and an external command is that arguments are passed after they are evaluated to external functions, while arguments are not evaluated for external commands.

External commands are a bit more tedious to implement. On the other hand external commands have more freedom handling their arguments. A command is allowed to evaluate or not to evaluate some of its arguments. It can examine the structure of the expression passed as argument and evaluate it partially at its wish.

From the BASIC programmer point of view external commands are called the same way as user defined functions or external functions.
*/

void COMMAND_EXTERNAM(pExecuteObject pEo){
#if NOTIMP_EXTERNAL
NOTIMPLEMENTED;
_FunctionFinishLabel:;
#else
  MortalList _ThisCommandMortals=NULL;
  pMortalList _pThisCommandMortals = &_ThisCommandMortals;
  pMortalList pSaveGlobalMortalList;
  unsigned long _ActualNode=PROGRAMCOUNTER;
  pModule *ThisModule;
  char *pszLibraryFile;
  char *pszFunctionName;
  void *FunctionPointer;
  pInstructionData *ppInstPointer;
  pFixSizeMemoryObject SaveLocalVariables;
  int (*ExternalFunction)(pExecuteObject, void **);
  int iResult;
  char *(*pfszErrMsg)(pSupportTable, void **, int);

  /* if the execution is just running through the declaration */
  if( ! pEo->fWeAreCallingFuction )return;

  pSaveGlobalMortalList = pEo->pGlobalMortalList;
  pEo->pGlobalMortalList = &_ThisCommandMortals;

  /* we need this variable, but should be set to NULL in case an error happens and the function returns 
     see more comment at the end of this function */
  SaveLocalVariables = pEo->LocalVariables;
  pEo->LocalVariables = NULL;

  /* The very first thing is to check if there was any external function call before, and
     if this is the first then initialize the data structure, which is needed to call external
     functions. */
  if( iResult = modu_Init(pEo,0) )ERROR(iResult);

  ppInstPointer = (pInstructionData *)&PARAMETEREXEC;
  if( *ppInstPointer ){/* if the command was already called and we already know the entry point */
    FunctionPointer = (*ppInstPointer)->FunctionPointer;
    ThisModule = &((*ppInstPointer)->ModulePointer);
    if( FunctionPointer == NULL )ERROR(COMMAND_ERROR_MODULE_FUNCTION);
    }else{
    /* get the name of the function */
    pszFunctionName = PARAMETERSTRING;
    NEXTPARAMETER;

    /* get the file name of the library where the function is defined */
    pszLibraryFile = PARAMETERSTRING;

    if( iResult =modu_GetFunctionByName(pEo,pszLibraryFile,pszFunctionName,&FunctionPointer,&ThisModule) )ERROR(iResult);

    if( FunctionPointer == NULL )ERROR(COMMAND_ERROR_MODULE_FUNCTION);
    *ppInstPointer = ALLOC(sizeof(InstructionData));
    if( *ppInstPointer == NULL )ERROR(COMMAND_ERROR_MEMORY_LOW);
    (*ppInstPointer)->ModulePointer = *ThisModule;
    /* now the module is loaded, get the function pointer */
    (*ppInstPointer)->FunctionPointer = FunctionPointer;
    }

  pEo->OperatorNode = pEo->FunctionArgumentsNode;

  pEo->pOpResult = NULL;
  pEo->ErrorCode = 0;
  ExternalFunction = FunctionPointer;
  (*ThisModule)->ModuleIsActive = 1;
  ExternalFunction(pEo,&((*ThisModule)->ModuleInternalParameters));
  if( pEo->ErrorCode ){
    pfszErrMsg = modu_GetModuleFunctionByName(*ThisModule,MODULE_ERRMSG);
    if( pfszErrMsg ){
     pEo->pszModuleError = pfszErrMsg(pEo->pST,&((*ThisModule)->ModuleInternalParameters),pEo->ErrorCode);
     }
    }
  (*ThisModule)->ModuleIsActive = 0;
  /* The interface functions from the external modules return mortal values. We have to immortalize this
     variable otherwise it is freed by the statement hidden in the macro END at the end of this function.
  */
  pEo->pFunctionResult = pEo->pOpResult;
  if( pEo->pFunctionResult && IsMortal(pEo->pFunctionResult) )
      memory_Immortalize(pEo->pFunctionResult,pEo->pGlobalMortalList);

  /* here we restore the pGlobalMortalList so that the caller can use it as it was */
  pEo->pGlobalMortalList = _pThisCommandMortals;
  if( pEo->ErrorCode )ERROR(pEo->ErrorCode);
  pEo->fStop = fStopRETURN;

_FunctionFinishLabel:
  memory_ReleaseMortals(pEo->pMo,&_ThisCommandMortals);
  pEo->pGlobalMortalList = pSaveGlobalMortalList;
#endif
  }
