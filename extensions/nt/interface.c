/* FILE: nt.c
   BAS: nt.bas

   AUTHOR: Peter Verhas
   DATE:   2002

This file is a ScriptBasic interface to NT system calls
that are not available under UNIX and thus are not
implemented in the core of ScriptBasic.

Never try to compile this file under UNIX or MacOS!

This line tells the program setup.pl that this module can only be
installed under Windows.

NTLIBS: advapi32.lib user32.lib kernel32.lib

*/
#include <stdio.h>
#include "../../basext.h"

#include <windows.h>
#include <winbase.h>
#include <winreg.h>
#include <tlhelp32.h>
#include <winsvc.h>

#include <stdio.h>
#include <winnt.h>
#include <winioctl.h>
#include <string.h>
#include <tchar.h>

/**
=H The module NT
=abstract
Windows NT specific functions
=end

This module implements some Win32 system calls that are not implemented in 
the core of ScriptBasic but can be helpful for those who want to write system 
maintenance scripts using ScriptBasic.

The reason that these functions are not implemented inside ScriptBasic is 
that ScriptBasic itself is portable, and whenever a programmer writes 
a program in pure ScriptBasic it should execute the same way under UNIX 
as well as under Windows NT/Win98/W2K.

Programs using the module nt however are Win32 specific and will not run 
unaltered under UNIX.

*/
besVERSION_NEGOTIATE

  return (int)INTERFACE_VERSION;

besEND

besSUB_START

besEND

besSUB_FINISH

besEND

#define NT_ERROR_INVALID_KEY        0x00081001
#define NT_ERROR_NOT_DELETED        0x00081002
#define NT_ERROR_REGISTRY           0x00081003
#define NT_ERROR_NOT_SET            0x00081004

/* Note that this is the maximal length that this module handles
   for registry key length or registry binary or string value.
*/
#define STRING_BUFFER_LENGTH 256

static int ChopHive(VARIABLE Argument,
                    unsigned long *pLen,
                    PHKEY phKey){

#define IFK(Y) if( STRLEN(Argument) >= (*pLen = strlen(#Y)) && ! memcmp(STRINGVALUE(Argument),#Y,*pLen) ){\
                 *phKey = Y;\
                 }else

  IFK(HKEY_CLASSES_ROOT)
  IFK(HKEY_CURRENT_CONFIG)
  IFK(HKEY_CURRENT_USER)
  IFK(HKEY_LOCAL_MACHINE)
  IFK(HKEY_USERS)
  IFK(HKEY_PERFORMANCE_DATA)
#undef IFK
#define IFK(Y,X) if( STRLEN(Argument) >= (*pLen = strlen(#Y)) && ! memcmp(STRINGVALUE(Argument),#Y,*pLen) ){\
                   *phKey = X;\
                   }else
  IFK(HKCR,HKEY_CLASSES_ROOT)
  IFK(HKCC,HKEY_CURRENT_CONFIG)
  IFK(HKCU,HKEY_CURRENT_USER)
  IFK(HKLM,HKEY_LOCAL_MACHINE)
  IFK(HKU ,HKEY_USERS)
  IFK(HKPD,HKEY_PERFORMANCE_DATA)
  /* ELSE */
  { return 0; }
  return 1;
  }

/**
=section registry_handling
=H Handling the registry

These functions handle the registry. 
The argument that specifies a registry element should start with the 
name of the top level registry hives. These are:

=itemize
=item T<HKEY_CLASSES_ROOT> or T<HKCR>
=item T<HKEY_CURRENT_CONFIG> or T<HKCC>
=item T<HKEY_CURRENT_USER> or T<HKCU>
=item T<HKEY_LOCAL_MACHINE> or T<HKLM>
=item T<HKEY_USERS> or T<HKU>
=item T<HKEY_PERFORMANCE_DATA> or T<HKPD>
=noitemize
The registry names in the documentation usually refer to the ScriptBasic configuration file location regis-try element. This is only an example, but you should not alter is from a program without good reason. Generally saying: be reasonable and think twice before altering the registry from a program.

*/
/**
=section RegRead
=H Read the registry

=verbatim
nt::RegRead("HKLM\\Software\\ScriptBasic\\config")
=noverbatim

This function reads a registry entry and returns the value of the entry or 
undef if the entry does not ex-ists or if it is not readable by the 
interpreter process/thread.

If the argument string specifies a key and the last character of the argument 
is \ then the default value for the key is retrieved.  Note that 
HKLM\Software\ScriptBasic can have a subkey named config as well 
as a value named config. The string 
T<"HKLM\\Software\\ScriptBasic\\config"> refers to the value while 
T<"HKLM\\Software\\ScriptBasic\\config\\"> refers to the default value of 
the subkey.
*/
besFUNCTION(regread)
  VARIABLE Argument;
  HKEY hKey,hRes;
  unsigned long sLen;
  char *s,*r,*pszSubKey;
  LONG Ret;
  char *pszData,szDummy;
  DWORD  dwType;
  DWORD  cbData;
  unsigned char swap;

  besRETURNVALUE = NULL;
  if( besARGNR < 1 )return EX_ERROR_TOO_FEW_ARGUMENTS;
  /* get the key */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  Argument = besCONVERT2STRING(Argument);
  if( ! ChopHive(Argument,&sLen,&hKey) )
    return NT_ERROR_INVALID_KEY;

  besCONVERT2ZCHAR(Argument,s);
  pszSubKey = s + sLen + 1; /* +1 to step over the \\ after HKLM*/
  sLen = STRLEN(Argument) - sLen;
  r = s + STRLEN(Argument);
  while( sLen && *r != '\\' ){
    sLen --;
    r--;
    }

  if( sLen ){
    *r = (char)0;
    r++;
    }else{
    besFREE(s);
    return NT_ERROR_INVALID_KEY;
    }

  Ret = RegOpenKeyEx(hKey,pszSubKey,0,KEY_READ,&hRes);

  if( Ret != ERROR_SUCCESS ){
    besFREE(s);
    return COMMAND_ERROR_SUCCESS;
    }

  cbData = 0;
  Ret = RegQueryValueEx(hRes,r,NULL,&dwType,&szDummy,&cbData);
  if( Ret == ERROR_MORE_DATA ){
    pszData = besALLOC(cbData);
    if( pszData == NULL ){
      besFREE(s);
      return COMMAND_ERROR_MEMORY_LOW;
      }
    }else{
    pszData = besALLOC(cbData=1);
    if( pszData == NULL ){
      besFREE(s);
      return COMMAND_ERROR_MEMORY_LOW;
      }
    }
  Ret = RegQueryValueEx(hRes,r,NULL,&dwType,pszData,&cbData);
  RegCloseKey(hRes);
  besFREE(s);

  if( Ret != ERROR_SUCCESS ){
    besFREE(pszData);
    return COMMAND_ERROR_SUCCESS;
    }

  switch( dwType ){
    case REG_DWORD_BIG_ENDIAN:
      if( cbData < 4 ){/* would be strange */
        besFREE(pszData);
        return NT_ERROR_REGISTRY;
        }
      swap = pszData[0]; pszData[0] = pszData[3]; pszData[3] = swap;
      swap = pszData[1]; pszData[1] = pszData[2]; pszData[2] = swap;
      /* ROLL OVER */
    case REG_DWORD:
      besALLOC_RETURN_LONG;
      memcpy(& LONGVALUE(besRETURNVALUE),pszData,sizeof(DWORD));
      besFREE(pszData);
      return COMMAND_ERROR_SUCCESS;
    case REG_BINARY:
    case REG_MULTI_SZ:
    case REG_SZ:
      besALLOC_RETURN_STRING(cbData);
      memcpy(STRINGVALUE(besRETURNVALUE),pszData,cbData);
      besFREE(pszData);
      return COMMAND_ERROR_SUCCESS;
    /* this module does NOT handle these types */
    case REG_LINK:
    case REG_NONE:
    case REG_RESOURCE_LIST:
      besFREE(pszData);
      return COMMAND_ERROR_SUCCESS;
      }
  besFREE(pszData);

besEND

/**
=section RegDel
=H Delete a registry value or key

=verbatim
nt::RegDel "HKLM\\Software\\ScriptBasic\\config"
=noverbatim

This function deletes a value from the registry or deletes a key from the registry.
If the argument specifies a registry key it should not have any subkeys. If the registry 
value or key cannot be deleted for some reason the function raises error that 
the programmer should capture using the T<ON ERROR GOTO> statement.
*/
besFUNCTION(regdel)
  VARIABLE Argument;
  HKEY hKey,hRes;
  unsigned long sLen;
  char *s,*r,*pszSubKey;
  LONG Ret;

  besRETURNVALUE = NULL;
  if( besARGNR < 1 )return EX_ERROR_TOO_FEW_ARGUMENTS;
  /* get the key */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  Argument = besCONVERT2STRING(Argument);
  if( ! ChopHive(Argument,&sLen,&hKey) )
    return NT_ERROR_INVALID_KEY;

  besCONVERT2ZCHAR(Argument,s);
  pszSubKey = s + sLen + 1; /* +1 to step over the \\ after HKLM*/

  /* try to delete it as a key */
  Ret = RegDeleteKey(hKey,pszSubKey);
  if( Ret == ERROR_SUCCESS ){
    besFREE(s);
    return COMMAND_ERROR_SUCCESS;
    }

  /* if this is not a key then delete the value */
  sLen = STRLEN(Argument) - sLen;
  r = s + STRLEN(Argument);
  while( sLen && *r != '\\' ){
    sLen --;
    r--;
    }

  if( sLen ){
    *r = (char)0;
    r++;
    }else{
    besFREE(s);
    return NT_ERROR_INVALID_KEY;
    }

  Ret = RegOpenKeyEx(hKey,pszSubKey,0,KEY_WRITE,&hRes);
  if( Ret != ERROR_SUCCESS ){
    besFREE(s);
    return NT_ERROR_NOT_DELETED;
    }
  Ret = RegDeleteValue(hRes,r);
  besFREE(s);
  RegCloseKey(hRes);
  if( Ret != ERROR_SUCCESS ){
    return NT_ERROR_NOT_DELETED;
    }
  return COMMAND_ERROR_SUCCESS;

besEND

/**
=section RegWrite
=H Write a registry value

=verbatim
nt::RegWrite "HKLM\\Software\\ScriptBasic\\config",value
=noverbatim

This function writes the value of a registry value. The key should already exist. 
If the key is non-existent or the value is not alterable by the program the function 
raises error that the programmer should capture using the T<ON ERROR GOTO> statement.
*/
besFUNCTION(regwrite)
  VARIABLE Argument;
  HKEY hKey,hRes;
  unsigned long sLen;
  char *s,*r,*pszSubKey;
  LONG Ret;

  besRETURNVALUE = NULL;
  if( besARGNR < 2 )return EX_ERROR_TOO_FEW_ARGUMENTS;
  /* get the key */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  Argument = besCONVERT2STRING(Argument);
  if( ! ChopHive(Argument,&sLen,&hKey) )
    return COMMAND_ERROR_SUCCESS;

  besCONVERT2ZCHAR(Argument,s);
  pszSubKey = s + sLen + 1; /* +1 to step over the \\ after HKLM*/
  sLen = STRLEN(Argument) - sLen;
  r = s + STRLEN(Argument);
  while( sLen && *r != '\\' ){
    sLen --;
    r--;
    }

  if( sLen ){
    *r = (char)0;
    r++;
    }else{
    besFREE(s);
    return COMMAND_ERROR_SUCCESS;
    }

  Ret = RegOpenKeyEx(hKey,pszSubKey,0,KEY_WRITE,&hRes);
  if( Ret != ERROR_SUCCESS ){
    besFREE(s);
    return NT_ERROR_INVALID_KEY;
    }

  /* get the value of the registry value */
  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  switch( TYPE(Argument) ){
    case VTYPE_DOUBLE:
      Argument = besCONVERT2LONG(Argument);
    case VTYPE_LONG:
      Ret = RegSetValueEx(hRes,r,0,REG_DWORD, (char *)& LONGVALUE(Argument),sizeof(long));
      break;
    case VTYPE_STRING:
      Ret = RegSetValueEx(hRes,r,0,REG_BINARY,STRINGVALUE(Argument),STRLEN(Argument));
    }
  besFREE(s);
  if( Ret == ERROR_SUCCESS )return COMMAND_ERROR_SUCCESS;
  return NT_ERROR_NOT_SET;
besEND

/**
=section MsgBox
=H Display a message box

=verbatim
i = nt::MsgBox("Text","Caption" [,"buttons","style",defbutton])
=noverbatim

This function displays a Windows message box on the screen and waits for the user to press
one of the buttons of the message box. The function should get two mandatory and three
optional arguments.

The arguments:

=itemize
=item T<"Text"> the first argument to the function is the text that appears on the text box.
=item T<"Caption"> the second argument to the function is the caption of the message
      box. This text appears in the top of the box.

=item T<"buttons"> This optional string specifies what buttons to diplay on the message box. If this
      argument is missing a single OK button will appear on the box. The valid strings for this argument
      are
  =itemize
  =item T<"ARI"> Abort/Retry/Ignore
  =item T<"OK"> OK
  =item T<"O"> OK
  =item T<"OC"> OK/Cancel
  =item T<"RC"> Retry/Cancel
  =item T<"YN"> Yes/No
  =item T<"YNC"> Yes/No/Cancel
  =noitemize
  You can also write the string in a long form for better readability:
  =itemize
  =item T<"AbortRetryIgnore">
  =item T<"Ok">
  =item T<"OkCancel">
  =item T<"RetryCancel">
  =item T<"YesNo">
  =item T<"YesNoCancel">
  =noitemize
=item T<"style"> this optional argument specifies what type of icon should appear on the message box
      left to the text. There are four icons available:
  =itemize
  =item T<"Exclamation">
  =item T<"Ex">
  =item T<"Warning">
  =item T<"W"> An exclamation-point icon appears in the message box.
  =item T<"Information">
  =item T<"Info">
  =item T<"I">
  =item T<"Asterix">
  =item T<"A">
  =item T<"*"> An icon consisting of a lowercase letter i in a circle appears in the message box.
  =item T<"Question">
  =item T<"Q">
  =item T<"?"> A question-mark icon appears in the message box.
  =item T<"Stop">
  =item T<"S">
  =item T<"Error">
  =item T<"E">
  =item T<"Hand">
  =item T<"H"> A stop-sign icon appears in the message box.
  =noitemize
  If this argument is missing or T<undef> no icon appears left to the text.
=item T<defbutton> should be an integer value between 1 to 4, or at most the number of the
      buttons on the actual message box. This argument specifies , which button is the default
      button on the form. The default button is selected as default and in case the user pressed
      the Enter key instead of clickong on one of the buttons the default button is pressed.
=noitemize

The B<return value> of the function is a one character string containing one of the characters
T<A, C, I, N, O, R, Y> for the pressed buttons respectively: Abort, Cancel, Ignore, No, OK, Retry, Yes.

The parameters T<"buttons"> and T<"style"> are not case sensitive. The return value is uppercase
character.

The following example tests all the possible message boxes:

=verbatim
import nt.bas

nt::MsgBox """This program test the various text boxes.

In the coming message boxes press the various keys as requested by the text
and check on the console window that the program reported the correct
button you pressed. (Only initials are printed.)
""","Initial MsgBox"

splita " |ARI|O|RC|YN|YNC" BY "|" TO buttons
splita " |W|I|Q|S" BY "|" TO marks

for j=lbound(marks) to ubound(marks)

for i=lbound(buttons) to ubound(buttons)

for d=1 to len(buttons[i])
print i," ",buttons[i]," ",j," ",marks[j]," ",d,"\n"
R = nt::MsgBox(buttons[i],marks[j],buttons[i],marks[j],d)
print R
print

next d
next i
next j
=noverbatim

*/
besFUNCTION(msgbox)
  char *pszTitle,*pszCaption;
  char *pszButtons,*pszStyle;
  long lDefault;
  int iError;
  int iMsgResult;
  UINT uType;

  iError = besGETARGS "zz[zzi]", &pszTitle, &pszCaption, &pszButtons , &pszStyle , &lDefault besGETARGE;

  if( iError )return iError;

  uType = MB_OK;

  if(      ! strcmp(pszButtons,"ARI") )uType = MB_ABORTRETRYIGNORE;
  else if( ! stricmp(pszButtons,"OK") )uType = MB_OK;
  else if( ! stricmp(pszButtons,"O") )uType = MB_OK;
  else if( ! stricmp(pszButtons,"OC") )uType = MB_OKCANCEL;
  else if( ! stricmp(pszButtons,"RC") )uType = MB_RETRYCANCEL;
  else if( ! stricmp(pszButtons,"YN") )uType = MB_YESNO;
  else if( ! stricmp(pszButtons,"YNC"))uType = MB_YESNOCANCEL;
  else 

  if(      ! stricmp(pszButtons,"AbortRetryIgnore") )uType = MB_ABORTRETRYIGNORE;
  else if( ! stricmp(pszButtons,"Ok")               )uType = MB_OK;
  else if( ! stricmp(pszButtons,"OkCancel")         )uType = MB_OKCANCEL;
  else if( ! stricmp(pszButtons,"RetryCancel")      )uType = MB_RETRYCANCEL;
  else if( ! stricmp(pszButtons,"YesNo")            )uType = MB_YESNO;
  else if( ! stricmp(pszButtons,"YesNoCancel")      )uType = MB_YESNOCANCEL;

  if(      ! stricmp(pszStyle,"Exclamation") )uType |= MB_ICONEXCLAMATION;
  else if( ! stricmp(pszStyle,"Ex")          )uType |= MB_ICONEXCLAMATION;
  else if( ! stricmp(pszStyle,"Warning")     )uType |= MB_ICONWARNING;
  else if( ! stricmp(pszStyle,"W")           )uType |= MB_ICONWARNING;
  else if( ! stricmp(pszStyle,"Information") )uType |= MB_ICONINFORMATION;
  else if( ! stricmp(pszStyle,"Info")        )uType |= MB_ICONINFORMATION;
  else if( ! stricmp(pszStyle,"I")           )uType |= MB_ICONINFORMATION;
  else if( ! stricmp(pszStyle,"Asterix")     )uType |= MB_ICONASTERISK;
  else if( ! stricmp(pszStyle,"A")           )uType |= MB_ICONASTERISK;
  else if( ! stricmp(pszStyle,"*")           )uType |= MB_ICONASTERISK;
  else if( ! stricmp(pszStyle,"Question")    )uType |= MB_ICONQUESTION;
  else if( ! stricmp(pszStyle,"Q")           )uType |= MB_ICONQUESTION;
  else if( ! stricmp(pszStyle,"?")           )uType |= MB_ICONQUESTION;
  else if( ! stricmp(pszStyle,"Stop")        )uType |= MB_ICONSTOP;
  else if( ! stricmp(pszStyle,"S")           )uType |= MB_ICONSTOP;
  else if( ! stricmp(pszStyle,"Error")       )uType |= MB_ICONERROR;
  else if( ! stricmp(pszStyle,"E")           )uType |= MB_ICONERROR;
  else if( ! stricmp(pszStyle,"Hand")        )uType |= MB_ICONHAND;
  else if( ! stricmp(pszStyle,"H")           )uType |= MB_ICONHAND;

  switch( lDefault ){
     case 1: uType |= MB_DEFBUTTON1; break;
     case 2: uType |= MB_DEFBUTTON2; break;
     case 3: uType |= MB_DEFBUTTON3; break;
     case 4: uType |= MB_DEFBUTTON4; break;
     }

  iMsgResult = MessageBoxEx(NULL,pszTitle,pszCaption,uType,
                            MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US));

  besFREE(pszTitle);
  besFREE(pszCaption);
  besFREE(pszButtons);
  besFREE(pszStyle);

  besALLOC_RETURN_STRING(1);
  switch( iMsgResult ){
    case IDABORT  : *STRINGVALUE(besRETURNVALUE) = 'A'; break;
    case IDCANCEL : *STRINGVALUE(besRETURNVALUE) = 'C'; break;
    case IDIGNORE : *STRINGVALUE(besRETURNVALUE) = 'I'; break;
    case IDNO     : *STRINGVALUE(besRETURNVALUE) = 'N'; break;
    case IDOK     : *STRINGVALUE(besRETURNVALUE) = 'O'; break;
    case IDRETRY  : *STRINGVALUE(besRETURNVALUE) = 'R'; break;
    case IDYES    : *STRINGVALUE(besRETURNVALUE) = 'Y'; break;
    default : *STRINGVALUE(besRETURNVALUE) = ' '; break;
    }

besEND

/**
=section ShutDown
=H ShutDown a Windows NT server or workstation

Calling this function shuts down a Windows NT Server, Workstation or
W2K machine. It was tested on a W2K Professional.

=verbatim
nt::ShutDown "machine","message",timeout,force,reboot
=noverbatim

The first argument is the name of the machine to reboot. If this is T<undef>
then the actual machine running is shutting down.

The second argument is the message that the system will display on the screen
before starting the actual shutdown. This argument may also be T<undef> in case
there is no need for such a message.

The argument T<timeout> is the number of seconds that the operating system will wait
before starting the actual shutdown.

T<force> specifies whether applications with unsaved changes are to be forcibly closed.
If this parameter is TRUE, such applications are closed. If this parameter is FALSE,
a dialog box is displayed prompting the user to close the applications.

T<reboot> specifies whether the computer is to restart immediately after shutting 
down. If this parameter is TRUE, the computer is to restart. If this parameter is FALSE,
the system flushes all caches to disk, clears the screen, and displays a message indicating 
that it is safe to power down or switches off automatically.

The function calls the Windows NT T<InitiateSystemShutdown> function. Some sentences of this
ScriptBasic documentation was copied from the original documentation of the Windows NT system
API documentation.
*/
besFUNCTION(shutdown_nt)
  HANDLE hToken;
  TOKEN_PRIVILEGES tkp;
  BOOL fResult;
  int iError;
  char *pszComputerName,*pszMessage;
  long lTimeout,lBruteForce,lReboot;

  iError = besGETARGS "[zz]iii", &pszComputerName, &pszMessage, &lTimeout, &lBruteForce, &lReboot besGETARGE;

  if(!OpenProcessToken(GetCurrentProcess(), 
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    return COMMAND_ERROR_EXTENSION_SPECIFIC;

  LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,&tkp.Privileges[0].Luid); 
 
  tkp.PrivilegeCount = 1;
  tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
 
  AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0); 
 
  if(GetLastError() != ERROR_SUCCESS)
    return COMMAND_ERROR_EXTENSION_SPECIFIC;

  fResult = InitiateSystemShutdown(pszComputerName,
                                   pszMessage,
                                   (DWORD)lTimeout,
                                   (BOOL)(lBruteForce != 0),
                                   (BOOL)(lReboot != 0));
  if( pszComputerName )besFREE(pszComputerName);
  if( pszMessage )besFREE(pszMessage);

  if(!fResult)
    return COMMAND_ERROR_EXTENSION_SPECIFIC;
 
  tkp.Privileges[0].Attributes = 0; 
  AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0); 
 
  if(GetLastError() != ERROR_SUCCESS) 
    return COMMAND_ERROR_EXTENSION_SPECIFIC;
besEND
/**
=section ListProcesses
=H Get the list of the running processes

This procedure gathers the information on the actually executiong processes and returns
the information in an array.

=verbatim
nt::ListProcesses PS
=noverbatim

The result is stored in the variable T<PS>. This will be an array, with each element being also an
array holding information for the process. The number of running processes can be determined
using T<ubound(PS)+1>.

The elements of the array is indexed from 0 to 8 contaiing the following elements:

=itemize
=item 0 Number of references to the process. A process exists as long as its usage count is nonzero. As soon as its usage count becomes zero, a process terminates.
=item 1 Identifier of the process. The contents of this member can be used by Win32 API elements.
=item 2 Identifier of the default heap for the process. The contents of this member has meaning only to the tool help functions. It is not a handle, nor is it usable by Win32 API elements.
=item 3 Module identifier of the process. The contents of this member has meaning only to the tool help functions. It is not a handle, nor is it usable by Win32 API elements.
=item 4 Number of execution threads started by the process.
=item 5 Identifier of the process that created the process being examined. The contents of this member can be used by Win32 API elements.
=item 6 Base priority of any threads created by this process. 
=item 7 Reserved; do not use.
=item 8 Path and filename of the executable file for the process.
=noitemize

*/
besFUNCTION(proclist)
  HANDLE th;
  unsigned long lProcCount;
  PROCESSENTRY32 pe;
  VARIABLE Argument;
  LEFTVALUE Lval;
  VARIABLE vPROC;
  int i;
  unsigned long __refcount_;

  Argument = besARGUMENT(1);
  besLEFTVALUE(Argument,Lval);

  if( Lval == NULL )return COMMAND_ERROR_EXTENSION_SPECIFIC;

  th = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
  if( th == INVALID_HANDLE_VALUE )return COMMAND_ERROR_EXTENSION_SPECIFIC;

  /* Count the processes to allocate the appropriate size array */
  pe.dwSize = sizeof(PROCESSENTRY32);
  lProcCount = 0;
  if( Process32First(th,&pe) ){/* that could be weird not to have any process running */
    lProcCount++;
    while( Process32Next(th,&pe) ){
      lProcCount++;
      pe.dwSize = sizeof(PROCESSENTRY32);
      }
    }

  if( Lval && *Lval ){ besRELEASE(*Lval); }
  if( lProcCount == 0 )return COMMAND_ERROR_SUCCESS;

  *Lval = besNEWARRAY(0,lProcCount-1);
  if( *Lval == NULL )return COMMAND_ERROR_MEMORY_LOW;

  lProcCount = 0;
  Process32First(th,&pe);
  do{
    vPROC = (*Lval)->Value.aValue[lProcCount] = besNEWARRAY(0,8);
    if( vPROC == NULL )return COMMAND_ERROR_MEMORY_LOW;
    for( i=0 ; i < 8 ; i++ ){
      vPROC->Value.aValue[i] = besNEWLONG;
      if( vPROC->Value.aValue[i] == NULL )return COMMAND_ERROR_MEMORY_LOW;
      }
    LONGVALUE(vPROC->Value.aValue[0]) = pe.cntUsage;
    LONGVALUE(vPROC->Value.aValue[1]) = pe.th32ProcessID; 
    LONGVALUE(vPROC->Value.aValue[2]) = pe.th32DefaultHeapID; 
    LONGVALUE(vPROC->Value.aValue[3]) = pe.th32ModuleID; 
    LONGVALUE(vPROC->Value.aValue[4]) = pe.cntThreads; 
    LONGVALUE(vPROC->Value.aValue[5]) = pe.th32ParentProcessID; 
    LONGVALUE(vPROC->Value.aValue[6]) = pe.pcPriClassBase; 
    LONGVALUE(vPROC->Value.aValue[7]) = pe.dwFlags;
    vPROC->Value.aValue[8] = besNEWSTRING(strlen(pe.szExeFile));
    if( vPROC->Value.aValue[8] == NULL )return COMMAND_ERROR_MEMORY_LOW;
    memcpy(STRINGVALUE(vPROC->Value.aValue[8]),pe.szExeFile,STRLEN(vPROC->Value.aValue[8]));

    lProcCount++;
    pe.dwSize = sizeof(PROCESSENTRY32);
    }while( Process32Next(th,&pe) );

  CloseHandle(th);
besEND

/**
=section StartService
=H Start a Windows NT service

This function starts an installed Windows NT service.

=verbatim
nt::StartService "Service name"
=noverbatim

The argument for this subroutine call is the name of the service to start. This is the
name that was used to create the service and you can read this name when you start the
graphical service control manager application.

The subroutine returns T<undef> value. If the service can not be started the function
raises error. Calling this subroutine for an already running service is not an error.

*/
besFUNCTION(nt_startservice)
  char *pszServiceName;
  int iError;
  SC_HANDLE hManager,hService;
  SERVICE_STATUS SSTAT;

  iError = besGETARGS "z", &pszServiceName besGETARGE;
  if( iError )return iError;
  hManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
  if( hManager == NULL )return COMMAND_ERROR_EXTENSION_SPECIFIC;
  hService = OpenService(hManager,pszServiceName,SERVICE_START|SERVICE_QUERY_STATUS);
  besFREE(pszServiceName);
  if( hService == NULL ){
    iError = GetLastError();
    return COMMAND_ERROR_EXTENSION_SPECIFIC;
    }
  memset(&SSTAT,0,sizeof(SSTAT));
  if( ! QueryServiceStatus(hService,&SSTAT) ){
    iError = GetLastError();
    return COMMAND_ERROR_EXTENSION_SPECIFIC;
    }
  /* we have to query the current state of the service because StartService returns
     strange errors instead of ERROR_SERVICE_ALREADY_RUNNING */
  if( SSTAT.dwCurrentState == SERVICE_START_PENDING || 
      SSTAT.dwCurrentState == SERVICE_RUNNING ||
      SSTAT.dwCurrentState == SERVICE_CONTINUE_PENDING )return COMMAND_ERROR_SUCCESS;
  iError = StartService(hService,0,NULL);
  CloseServiceHandle(hService);
  CloseServiceHandle(hManager);
  if( iError == 0 )iError = GetLastError(); else iError = 0;
  if( iError == 0 || iError == ERROR_SERVICE_ALREADY_RUNNING )return COMMAND_ERROR_SUCCESS;
  return COMMAND_ERROR_EXTENSION_SPECIFIC;
besEND

/* This function is used by most of the service control functions
   implemented in this interface file that start stop, pause and continue
   service(s)

   This function gets the same argument as the interface function that calls
   this function and an extra argument dwControl that tells the function what
   to do actually.
*/
static int ControlNtService(pSupportTable pSt,
                            void **ppModuleInternal,
                            pFixSizeMemoryObject pParameters,
                            DWORD dwControl
  ){
  char *pszServiceName;
  int iError;
  SC_HANDLE hManager,hService;
  SERVICE_STATUS SSTAT;

  iError = besGETARGS "z", &pszServiceName besGETARGE;
  if( iError )return iError;
  hManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
  if( hManager == NULL )return COMMAND_ERROR_EXTENSION_SPECIFIC;
  hService = OpenService(hManager,pszServiceName,SERVICE_ALL_ACCESS);
  besFREE(pszServiceName);
  if( hService == NULL )return COMMAND_ERROR_EXTENSION_SPECIFIC;
  memset(&SSTAT,0,sizeof(SSTAT));
  iError = ControlService(hService,dwControl,&SSTAT);

  CloseServiceHandle(hService);
  CloseServiceHandle(hManager);
  if( iError == 0 )iError = GetLastError(); else iError = 0;
  if( iError )return COMMAND_ERROR_EXTENSION_SPECIFIC;
  return COMMAND_ERROR_SUCCESS;
  }

#define CONTROL_NT_SERVICE(X) ControlNtService(pSt,ppModuleInternal,pParameters,X)

/**
=section StopService
=H Stop a Windows NT service

This function stops an already running Windows NT service.

=verbatim
nt::StopService "Service name"
=noverbatim

The argument for this subroutine call is the name of the service to start. This is the
name that was used to create the service and you can read this name when you start the
graphical service control manager application.

The subroutine returns T<undef> value. If the service can not be stopped the function
raises error.

*/
besFUNCTION(nt_stopservice)
  return CONTROL_NT_SERVICE(SERVICE_CONTROL_STOP);
besEND

/**
=section PauseService
=H Pause a Windows NT service

This function pauses an already running Windows NT service.

=verbatim
nt::PauseService "Service name"
=noverbatim

The argument for this subroutine call is the name of the service to start. This is the
name that was used to create the service and you can read this name when you start the
graphical service control manager application.

The subroutine returns T<undef> value. If the service can not be paused the function
raises error.
*/
besFUNCTION(nt_pauseservice)
  return CONTROL_NT_SERVICE(SERVICE_CONTROL_PAUSE);
besEND

/**
=section ContinueService
=H Continue a paused Windows NT service

This function asks a Windows NT service to continue after it was paused

=verbatim
nt::ContinueService "Service name"
=noverbatim

The argument for this subroutine call is the name of the service to start. This is the
name that was used to create the service and you can read this name when you start the
graphical service control manager application.

The subroutine returns T<undef> value. If the service can not be continued the function
raises error.
*/
besFUNCTION(nt_continueservice)
  return CONTROL_NT_SERVICE(SERVICE_CONTROL_CONTINUE);
besEND

/**
=section HardLink
=H Create a hard link on a file or directory

This function function establishes an NTFS hard link between an existing file/directory
and a new file/directory. An NTFS hard link is similar to a POSIX hard link.

=verbatim
nt::HardLink "existing file","new filename"
=noverbatim

Any directory entry for a file, is a hard link to the associated file. Additional hard links, 
created with the HardLink function, allow you to have multiple directory entries for a file, 
that is, multiple hard links to the same file. These may be different names in 
the same directory, or they may be the same (or different) names in different directories.
However, all hard links to a file must be on the same volume.

Because hard links are just directory entries for a file, whenever an application modifies 
a file through any hard link, all applications using any other hard link 
to the file see the changes. Also, all of the directory entries are updated if the file changes. 
For example, if the file's size changes, all of the hard links to the file will show the new size.

When creating hard link to a directory the situation is a bit different. In these cases the original
directory entry and the created links are not equivalent. If you delete the link then the original
directory and its content remains untouched. However if you delete the directory any link to that
directory becomes unusable.

*/
static int lnw(char *,char *);
static int CreateJunction( char *LinkDirectory, char *LinkTarget );

besFUNCTION(nt_hardlink)
  char *pszFileName,*pszNewName;
  int iError;

  iError = besGETARGS "zz", &pszFileName,&pszNewName besGETARGE;
  if( iError )return iError;

  iError = (GetFileAttributes(pszFileName) & FILE_ATTRIBUTE_DIRECTORY)  ?
    CreateJunction(pszNewName,pszFileName)
  :
    lnw(pszFileName,pszNewName);
  
  if( iError )return COMMAND_ERROR_EXTENSION_SPECIFIC;
  return COMMAND_ERROR_SUCCESS;
besEND


void enableprivs(){
  HANDLE hToken;
  byte buf[sizeof(TOKEN_PRIVILEGES) * 2];
  TOKEN_PRIVILEGES *tkp = ( (TOKEN_PRIVILEGES *) buf );

  if ( ! OpenProcessToken( GetCurrentProcess(),
      TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
      return;

  // enable SeBackupPrivilege, SeRestorePrivilege

  if ( !LookupPrivilegeValue( NULL, SE_BACKUP_NAME, &tkp->Privileges[0].Luid ) )
      return;

  if ( !LookupPrivilegeValue( NULL, SE_RESTORE_NAME, &tkp->Privileges[1].Luid ) )
      return;

  tkp->PrivilegeCount = 2;
  tkp->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  tkp->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

  AdjustTokenPrivileges( hToken, FALSE, tkp, sizeof( *tkp ),
      NULL, NULL );
  }



static int lnw(char *pszSource, char *pszDestination){
  HANDLE fh;
  static char buf1[MAX_PATH];
  static wchar_t buf2[MAX_PATH * 2];
  char *p;
  void *ctx = NULL;
  WIN32_STREAM_ID wsi;
  DWORD numwritten;
  BOOL bSourceIsReadOnly;
  DWORD attrib;

  enableprivs(); // in case we aren't admin
  attrib = GetFileAttributes(pszSource);
  if( INVALID_FILE_ATTRIBUTES == attrib ){
    return 1;
    }

  bSourceIsReadOnly = FALSE;
  if( attrib & FILE_ATTRIBUTE_READONLY ){
    bSourceIsReadOnly = TRUE; /* remember that we have altered the attributes, has to be restored when closing the handle */
    /* remove the read only bit from the flags and ... */
    SetFileAttributes(pszSource,attrib & ~FILE_ATTRIBUTE_READONLY);
    }
  if( attrib & FILE_ATTRIBUTE_DIRECTORY ){
    return 1;
    }

  fh = CreateFile( pszSource, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, NULL );
  if ( fh == INVALID_HANDLE_VALUE || fh == NULL ){
      if( bSourceIsReadOnly ){
        SetFileAttributes(pszSource,attrib);
        }
      return 1;
      }

  GetFullPathName( pszDestination, MAX_PATH, buf1, &p );

  wsi.dwStreamId = BACKUP_LINK;
  wsi.dwStreamAttributes = 0;
  wsi.dwStreamNameSize = 0;
  wsi.Size.QuadPart = strlen( buf1 ) * 2 + 2;
  MultiByteToWideChar( CP_ACP, 0, buf1, (int)strlen( buf1 ) + 1, buf2, MAX_PATH );

  if( ! BackupWrite( fh, (byte *) &wsi, 20, &numwritten, FALSE, FALSE, &ctx ) ){
    return 1;
    }
  if( numwritten != 20 ){
    return 1;
    }

  if( ! BackupWrite( fh, (byte *) buf2, wsi.Size.LowPart, &numwritten, FALSE, FALSE, &ctx ) ){
    return 1;
    }
  if( numwritten != wsi.Size.LowPart ){
    return 1;
    }

  // make NT release the context
  if( ! BackupWrite( fh, (byte *) &buf1[0], 0, &numwritten, TRUE, FALSE, &ctx ) ){
    return 1;
    }

  CloseHandle( fh );
  if( bSourceIsReadOnly ){
    SetFileAttributes(pszSource,attrib);
    }
  return 0;
  }

typedef struct _REPARSE_DATA_BUFFER {
    DWORD  ReparseTag;
    WORD   ReparseDataLength;
    WORD   Reserved;
    union {
        struct {
            WORD   SubstituteNameOffset;
            WORD   SubstituteNameLength;
            WORD   PrintNameOffset;
            WORD   PrintNameLength;
            WCHAR PathBuffer[1024];
        } SymbolicLinkReparseBuffer;
        struct {
            WORD   SubstituteNameOffset;
            WORD   SubstituteNameLength;
            WORD   PrintNameOffset;
            WORD   PrintNameLength;
            WCHAR PathBuffer[1024];
        } MountPointReparseBuffer;
        struct {
            BYTE   DataBuffer[1024];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER;

#define REPARSE_MOUNTPOINT_HEADER_SIZE 8

#ifndef FSCTL_SET_REPARSE_POINT
#  define FSCTL_SET_REPARSE_POINT    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 41, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#  define FSCTL_GET_REPARSE_POINT    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 42, METHOD_BUFFERED, FILE_ANY_ACCESS) 
#  define FSCTL_DELETE_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 43, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) 
#endif

void PrintWin32Error( DWORD x){}

#define toUnicode(FROM,TO)                     \
    MultiByteToWideChar(CP_ACP,              \
                        0,                     \
                        (char *)FROM,          \
                        strlen((char *)FROM)+1,\
                        TO,                    \
                        sizeof(TO))

static int CreateJunction(char *LinkDirectory, char *LinkTarget){
  REPARSE_DATA_BUFFER   reparseBuffer;
  char    directoryFileName[MAX_PATH];
  char    targetFileName[MAX_PATH];
  char    targetNativeFileName[MAX_PATH];
  WCHAR   wtargetNativeFileName[MAX_PATH];
  PTCHAR    filePart;
  HANDLE    hFile;
  DWORD   returnedLength;
  size_t slen;
  REPARSE_DATA_BUFFER *reparseInfo = (REPARSE_DATA_BUFFER *) &reparseBuffer;

  // Get the full path referenced by the target
  if( !GetFullPathName( LinkTarget, MAX_PATH, targetFileName, &filePart ) ){
    return 1;
    }

  // Get the full path referenced by the directory
  if( !GetFullPathName( LinkDirectory, MAX_PATH, directoryFileName, &filePart) ){
    return 1;
    }

  // Make the native target name
  sprintf( targetNativeFileName, "\\??\\%s" , targetFileName );
  slen = strlen(targetNativeFileName);
  if ( (targetNativeFileName[slen-1] == _T('\\')) &&
     (targetNativeFileName[slen-2] != _T(':'))) {
      targetNativeFileName[slen-1] = 0;
     }

  // Create the link - ignore errors since it might already exist
  CreateDirectory( LinkDirectory, NULL );
  hFile = CreateFile( LinkDirectory, GENERIC_WRITE, 0,
                       NULL, OPEN_EXISTING, 
                       FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS, NULL );
  if( hFile == INVALID_HANDLE_VALUE ) {
    return 1;
    }

  toUnicode(targetNativeFileName,wtargetNativeFileName);

  // Build the reparse info
  memset( reparseInfo, 0, sizeof( REPARSE_DATA_BUFFER ));
  reparseInfo->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
  reparseInfo->SymbolicLinkReparseBuffer.SubstituteNameLength = 
    wcslen(wtargetNativeFileName) * sizeof(WCHAR);
  reparseInfo->Reserved = 0;
  reparseInfo->SymbolicLinkReparseBuffer.PrintNameLength = 0;
  reparseInfo->SymbolicLinkReparseBuffer.PrintNameOffset = 
    reparseInfo->SymbolicLinkReparseBuffer.SubstituteNameLength 
    + sizeof(WCHAR);
  memcpy(reparseInfo->SymbolicLinkReparseBuffer.PathBuffer, wtargetNativeFileName, 
    sizeof(WCHAR) 
    + reparseInfo->SymbolicLinkReparseBuffer.SubstituteNameLength);
  reparseInfo->ReparseDataLength = 
    reparseInfo->SymbolicLinkReparseBuffer.SubstituteNameLength + 12;

  // Set the link
  if( !DeviceIoControl(hFile, FSCTL_SET_REPARSE_POINT, reparseInfo, 
       reparseInfo->ReparseDataLength 
       + REPARSE_MOUNTPOINT_HEADER_SIZE,
       NULL, 0, &returnedLength, NULL)) { 

    CloseHandle( hFile );
    RemoveDirectory( LinkDirectory );
    return 1;
    }
  return 0;
  }


