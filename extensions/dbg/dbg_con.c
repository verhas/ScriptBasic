/*
FILE: dbg_con.c
HEADER: dbg_comm.h

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

This program implements a simple debugger "preprocessor" for ScriptBasic.

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../conftree.h"
#include "../../report.h"
#include "../../reader.h"
#include "../../basext.h"
#include "../../prepext.h"

#include "dbg.h"

/*POD
=H Debugger communication module

This file implements the functions that are used by the debugger module and,
which communicate with the debugger station. This sample implementation is
the possible simplest example implementation using T<getchar()> to get characters
and T<printf()> to send characters to the user.

Other implementations should implement the same functions but using more sophisticated
methods, like connecting to a socket where a graphical debugger client application is
accepting connection and wants to communincate with the debugger module.

CUT*/

#ifdef WIN32
static HANDLE GetConsoleHandle(){
  HANDLE hConsole;

  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  if( hConsole == NULL ){
    AllocConsole();
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    }
  return hConsole;
  }

static void gotoxy(short x, short y){
  COORD lp;
  HANDLE hConsole;

  lp.X = x; lp.Y = y;
  hConsole = GetConsoleHandle();
  if( hConsole == NULL )return;
  SetConsoleCursorPosition(hConsole,lp);
  }

static int sizeY(){
  HANDLE hConsole;
  CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;

  hConsole = GetConsoleHandle();
  if( hConsole == NULL )return 0;
  GetConsoleScreenBufferInfo(hConsole,&ConsoleScreenBufferInfo);

  return ConsoleScreenBufferInfo.srWindow.Bottom - ConsoleScreenBufferInfo.srWindow.Top + 1;
  }

static int sizeX(){
  HANDLE hConsole;
  CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;

  hConsole = GetConsoleHandle();
  if( hConsole == NULL )return 0;
  GetConsoleScreenBufferInfo(hConsole,&ConsoleScreenBufferInfo);

  return ConsoleScreenBufferInfo.srWindow.Right - ConsoleScreenBufferInfo.srWindow.Left + 1;
  }

static void cls(){
    COORD coordScreen = { 0, 0 };    /* here's where we'll home the
                                        cursor */
    BOOL bSuccess;
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
    DWORD dwConSize;                 /* number of character cells in
                                        the current buffer */
    HANDLE hConsole;

    hConsole = GetConsoleHandle();
    if( hConsole == NULL )return;
    /* get the number of character cells in the current buffer */

    bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

    /* fill the entire screen with blanks */

    bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) ' ',
       dwConSize, coordScreen, &cCharsWritten );

    /* get the current text attribute */

    bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );

    /* now set the buffer's attributes accordingly */

    bSuccess = FillConsoleOutputAttribute( hConsole, csbi.wAttributes,
       dwConSize, coordScreen, &cCharsWritten );

    /* put the cursor at (0, 0) */

    bSuccess = SetConsoleCursorPosition( hConsole, coordScreen );
    return;
 }


#endif

/*POD
=section Init
=H Initiate communication with the debugger station

This function is called by the debugger when the execution of the program starts.
This function has to set up the debugger environment with the client. Connecting to
the listening socket, clearing screen and so on.

/*FUNCTION*/
void comm_Init(pDebuggerObject pDO
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  HANDLE hConsole;

  SetConsoleTitle("ScriptBasic debugger executing");
  hConsole = GetConsoleHandle();
  if( hConsole )SetConsoleTextAttribute(hConsole,240);
  cls();

  gotoxy(1,sizeY()-2);
  printf("For help type 'h'\n");

#else
  long i;
  printf("ScriptBasic debugger, executing\n");
  for( i=0 ; i < pDO->cFileNames ; i++ )
    printf("  %s\n",pDO->ppszFileNames[i]);
  printf("For help type 'h'");
  getche();
#endif
  }

#define SEPARATORLINE   printf("\n-----------------------------------------------------\n");

/*POD
=section WeAreAt
=H Send prompt to the debugger station

This function is called by the debugger when it stops before executing
a BASIC line. This function can be used to give some information to the
client, displaying lines around the actual one, values of variables and so on.

/*FUNCTION*/
void comm_WeAreAt(pDebuggerObject pDO,
                  long i
  ){
/*noverbatim
CUT*/
  int iThis;

#ifdef WIN32
  short y;
  HANDLE hConsole;
  void comm_List();

  hConsole = GetConsoleHandle();
  if( hConsole )SetConsoleTextAttribute(hConsole,240);
  iThis = i;
  /* the number of lines available for the program code display */
  y = sizeY() - pDO->cFileNames - 2 - 3;
  /* position the actual line in the middle */
  y /= 2;
  /* list some lines before */
  if( i > y )i -= y; else i = 0;
  comm_List(pDO,i,9999999,iThis);

#else
  int j;
  iThis = i;
  /* list some lines before */
  if( i > 2 )i -= 2; else i = 0;

  SEPARATORLINE
  for( j = 0 ; j < 5 ; j++ ){
    if( i >= pDO->cSourceLines )break;
    if( pDO->SourceLines[i].BreakPoint )printf("*"); else printf(" ");
    if( i == iThis )printf(">");else printf(" ");
    printf("%03d. %s",i+1,pDO->SourceLines[i].line);
    i++;
    }
  SEPARATORLINE
#endif
  }

/*POD
=section List
=H List code lines

List the source lines from T<lStart> to T<lEnd>.

The optional T<lThis> may show the caret where the actual execution
context is.

/*FUNCTION*/
void comm_List(pDebuggerObject pDO,
               long lStart,
               long lEnd,
               long lThis
  ){
/*noverbatim
CUT*/
  long j;

#ifdef WIN32
  HANDLE hConsole;
  short y;

  hConsole = GetConsoleHandle();

  if( lStart < 1 )lStart = 1;
  if( lEnd   < 1 )lEnd   = 1;

  SetConsoleTitle("ScriptBasic debugger executing");
  cls();
  for( j=0 ; j < pDO->cFileNames ; j++ )
    printf("  %s\n",pDO->ppszFileNames[j]);

  y = (short) pDO->cFileNames+1;
  for( j = lStart-1 ; j < lEnd ; j++ ){
    if( j >= pDO->cSourceLines )break;
    if( y >= sizeY()-3 )break;
    gotoxy(1,y);
    y++;
    SetConsoleTextAttribute(hConsole,240);
    if( pDO->SourceLines[j].BreakPoint ){
      SetConsoleTextAttribute(hConsole,252);
      printf("*");
      }else{
      printf(" ");
      }
    if( j == lThis ){
      SetConsoleTextAttribute(hConsole,249);
      printf(">");
    }else{
      printf(" ");
    }
    printf("%03d. %s",j+1,pDO->SourceLines[j].line);
    }

  gotoxy(1,sizeY()-3);
  printf("For help type 'h'\n");

#else

  if( lStart < 1 )lStart = 1;
  if( lEnd   < 1 )lEnd   = 1;
  printf("\n");
  SEPARATORLINE
  for( j = lStart-1 ; j < lEnd ; j++ ){
    if( j >= pDO->cSourceLines )break;
    if( pDO->SourceLines[j].BreakPoint )printf("*"); else printf(" ");
    if( j == lThis )printf(">");else printf(" ");
    printf("%03d. %s",j+1,pDO->SourceLines[j].line);
    }
  SEPARATORLINE
#endif
  }

/*POD
=section GetRange
=H get line number range from a string

This is an auxilliary function, which is used by the debugger.
This simply gets the two numbers from the debugger command and returns
them in the variables pointed by T<plStart> and T<plEnd>.

For example the command T<B 2-5> removes breakpoints from lines 2,3,4 and 5.
In this case this function will return the numbers 2 and 5.

If the first number is missing it is returned as 0. If there is first number
but the last one is missing it is returned 999999999.

If there is first number but it is not followed by '-' then the T<*plEnd> will
be set to zero.

Finally if there are no numbers on the command line then bot variables are set zero.
/*FUNCTION*/
void GetRange(char *pszBuffer,
              long *plStart,
              long *plEnd
  ){
/*noverbatim
Arguments:
=itemize
=item T<pszBuffer> the debugger command argument string to get the numbers from
=item T<plStart> pointer to the long that will hold the value of the first number
=item T<plEnd> pointer to the long that will hold the value of the second number following the dash character
=noitemize
CUT*/
  *plStart = *plEnd = 0;
  while( isspace(*pszBuffer) )pszBuffer++;
  if( !*pszBuffer )return;
  *plStart = atol(pszBuffer);
  while( isdigit(*pszBuffer))pszBuffer++;
  while( isspace(*pszBuffer) )pszBuffer++;
  if( *pszBuffer == '-' ){
    pszBuffer++;
    *plEnd = 999999999;/* something large, very large */
    }
  while( isspace(*pszBuffer) )pszBuffer++;
  if( !*pszBuffer )return;
  *plEnd = atol(pszBuffer);
  return;
  }

static void print_help(){
#ifdef WIN32
  cls();
  SetConsoleTitle("ScriptBasic debugger HELP");

  printf(
"                                                                                  "
"              ScriptBasic Console Debugger Help\n"
"h help\n"
"s step one line, or just press return on the line\n"
"S step one line, do not step into functions or subs\n"
"o step until getting out of the current function\n"
"  (if you stepped into but changed your mind)\n"
"? var  print the value of a variable\n"
"u step one level up in the stack\n"
"d step one level down in the stack (for variable printing)\n"
"D step down in the stack to current execution depth\n"
"G list all global variables\n"
"L list all local variables\n"
"l [n-m] list the source lines\n"
"r [n] run to line n\n"
"R [n] run to line n but do not stop in recursive function call\n"
"b [n] set breakpoint on the line n or the current line\n"
"B [n-m] remove breakpoints from lines\n"
"q quit the program\n"

"Press any key to return to debugger..."
);
_getch();
#else
  printf(
"h help\n"
"s step one line, or just press return on the line\n"
"S step one line, do not step into functions or subs\n"
"o step until getting out of the current function\n"
"  (if you stepped into but changed your mind)\n"
"? var  print the value of a variable\n"
"u step one level up in the stack\n"
"d step one level down in the stack (for variable printing)\n"
"D step down in the stack to current execution depth\n"
"G list all global variables\n"
"L list all local variables\n"
"l [n-m] list the source lines\n"
"r [n] run to line n\n"
"R [n] run to line n but do not stop in recursive function call\n"
"b [n] set breakpoint on the line n or the current line\n"
"B [n-m] remove breakpoints from lines\n"
"q quit the program\n"
);
#endif
  }

/*POD
=section Message
=H Report success of some command

This function is called when a command that results no output is executed.
The message is an informal message to the client that either tells that the
command was executed successfully or that the command failed and why.

/*FUNCTION*/
void comm_Message(pDebuggerObject pDO,
                  char *pszMessage
  ){
/*noverbatim
CUT*/

#ifdef WIN32

  MessageBoxEx(NULL,pszMessage,"ScriptBasic debugger message",MB_OK|MB_ICONINFORMATION,
                            MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US));

#else
  printf("%s\n",pszMessage);
#endif
  }

/*POD
=section GetCommand
=H Prompt the debugger station

This function should send the prompt to the client and get the client
input. The function should return a single character that represents the
command what the debugger is supposed to do and the possible string argument
in T<pszBuffer>. The available space for the argument is given T<cbBuffer>.

/*FUNCTION*/
int comm_GetCommand(pDebuggerObject pDO,
                    char *pszBuffer,
                    long cbBuffer
  ){
/*noverbatim
The commands that the debugger accepts: (see help function printout above).

The function may also implement some printing commands itself, like printing
a help screen.
CUT*/
  int i,j;
  int ch,cmd;
  char pszPrintBuff[1024];
  long cbPrintBuff;
  pUserFunction_t pUF;
  pExecuteObject pEo;
  long lStart,lEnd,lThis;

  pEo = pDO->pEo;
  while( 1 ){
    lThis = GetCurrentDebugLine(pDO);
    comm_WeAreAt(pDO,lThis);
    printf("#");

    cmd = getchar();
    while( isspace(cmd) && cmd != '\n' )cmd = getchar();
    if( cmd == '\n' ){
      *pszBuffer = (char)0;
      return 's';
      }
    ch = getchar();
    while( isspace(ch) && ch != '\n' )ch = getchar();
  
    for( i=0 ; i < cbBuffer ; i++ ){
      if( ch == '\n' )break;
      pszBuffer[i] = ch;
      ch = getchar();
      }
    pszBuffer[i] = (char)0;

    switch( cmd ){
      case 'l':/*list lines*/
        lThis = GetCurrentDebugLine(pDO);

        if( *pszBuffer ){/*if there are arguments*/
          GetRange(pszBuffer,&lStart,&lEnd);
          comm_List(pDO,lStart,lEnd,lThis);
          }else comm_WeAreAt(pDO,lThis);

        continue;
      case 'h':  
        print_help();
        continue;
      case '?':
        cbPrintBuff = 1204;
        i = SPrintVarByName(pDO,pDO->pEo,pszBuffer,pszPrintBuff,&cbPrintBuff);
        switch( i ){
          case 2:
            printf("variable is non-existent\n");
            continue;
          case 1:
            printf("variable is too long to print\n");
            continue;
          default:
            printf("%s\n",pszPrintBuff);
          }
#ifdef WIN32
        printf("Press any key to continue...");_getch();
#endif
      continue;
    case 'L': /* list local variables */
      if( pDO->StackListPointer == NULL ){
        comm_Message(pDO,"program is not local");
        continue;
        }
      pUF = pDO->StackListPointer->pUF;
      for( i=0 ; i < pUF->cLocalVariables ; i++ ){
        printf("%s=",pUF->ppszLocalVariables[i]);
        if( pDO->StackListPointer->LocalVariables ){
          j = SPrintVariable(pDO,ARRAYVALUE(pDO->StackListPointer->LocalVariables,i+1),pszPrintBuff,&cbPrintBuff);
          switch( j ){
            case 2:
              printf("variable is non-existent\n");
              continue;
            case 1:
              printf("variable is too long to print\n");
              continue;
            default:
              printf("%s\n",pszPrintBuff);
            }
          }else{
          printf("undef\n");
          }
        }
#ifdef WIN32
        printf("Press any key to continue...");_getch();
#endif
      continue;
    case 'G':/* list global variables */
      for( i=0 ; i < pDO->cGlobalVariables ; i++ ){
        printf("%s=",pDO->ppszGlobalVariables[i]);
        if( pEo->GlobalVariables ){
          j = SPrintVariable(pDO,ARRAYVALUE(pEo->GlobalVariables,i+1),pszPrintBuff,&cbPrintBuff);
          switch( j ){
            case 2:
              printf("variable is non-existent\n");
              continue;
            case 1:
              printf("variable is too long to print\n");
              continue;
            default:
              printf("%s\n",pszPrintBuff);
            }
          }else
          printf("undef\n");
        }
#ifdef WIN32
        printf("Press any key to continue...");_getch();
#endif
      continue;
      }

    break;
    }
  return cmd;
  }
