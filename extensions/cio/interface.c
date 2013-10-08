/* FILE: cio.c

This file implements the cio module for ScriptBasic. This module is
to handle console input and output directly.

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

This line tells the Perl script setup.pl that this file is to be compiled
under NT or other WIN32 flavours and should not try to compile it under
UNIX.

NTLIBS:
*/
#ifdef WIN32
#include <conio.h>
#include <stdio.h>
#include <windows.h>
#include <winbase.h>
#include <winuser.h>
#endif
#include "../../basext.h"

/**
Console handling low level routines for Win32 environment.

This module implements routines that can handle the Windows character
console on low level. This is like getting a character without echo, or
checking that a key was pressed without stopping the program, disabling
control-c and so on.

*/

besSUB_SHUTDOWN
  return 0;
besEND

besSUB_START
  return COMMAND_ERROR_SUCCESS;
besEND

static HANDLE GetConsoleHandle(){
  HANDLE hConsole;

  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  if( hConsole == NULL ){
    AllocConsole();
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    }
  return hConsole;
  }

#define GET_CONSOLE_HANDLE   hConsole = GetConsoleHandle();\
  if( hConsole == NULL )return 0;

/**
=section gotoxy
=H cio::GotoXY(x,y)

Position the console cursor at the coordinate (X,Y). The
next PRINT statement sending characters to the screen will
print to this position.
*/
besFUNCTION(gotoxy)
#ifdef WIN32
  COORD lp;
  long x,y;
  HANDLE hConsole;
  int iError;

  if( besARGNR < 2 )return COMMAND_ERROR_FEW_ARGS;

  iError = besGETARGS "ii",&x,&y besGETARGE
  lp.X = (short)x; lp.Y = (short)y;
  if( iError )return iError;
  GET_CONSOLE_HANDLE
  SetConsoleCursorPosition(hConsole,lp);
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

/**
=section kbhit
=H cio::KbHit()

Returns T<TRUE> if there is any keyboard pressed and waiting in the keyboard buffer and
returns T<FALSE> if there is no key in the keyboard buffer.
*/
besFUNCTION(sbkbhit)
#ifdef WIN32
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = _kbhit() ? -1 : 0 ;
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

/**
=section getch
=H cio::GetCh()

Get a single character from the keyboard buffer without echo. If there
is no key pressed in the keyboard buffer then the function waits for the
user to press a key.
*/
besFUNCTION(sbgetch)
#ifdef WIN32
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = _getch();
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

/**
=section getche
=H cio::GetChE()

Get a single character from the keyboard buffer with echo. If there
is no key pressed in the keyboard buffer then the function waits for the
user to press a key.
*/
besFUNCTION(sbgetche)
#ifdef WIN32
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = _getche();
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

/**
=section detach
=H cio::Detach()

Close the console window. There is no way to open a new console after it has been closed.
If the program was started from the command line it does not close the console window, but
further print commands go into nowhere.
*/
besFUNCTION(sbdetach)
#ifdef WIN32
  FreeConsole();
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

/**
=section gettitle
=H cio::GetTitle()

The function returns the title text of the console window.
*/
besFUNCTION(sbgettitle)
#ifdef WIN32
  char szB[256];
  long i;

  i = GetConsoleTitle(szB,256);
  if( i > 0 ){
    besALLOC_RETURN_STRING(i);
    memcpy(STRINGVALUE(besRETURNVALUE),szB,i);
    }else
    besRETURNVALUE = NULL;
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

/**
=section settitle
=H cio::SetTitle()

Set the title of the console window.
*/
besFUNCTION(sbsettitle)
#ifdef WIN32
  char *pszT;
  int iError;

  if( besARGNR < 1 )return COMMAND_ERROR_FEW_ARGS;

  iError = besGETARGS "z",&pszT besGETARGE
  if( iError )return iError;
  SetConsoleTitle(pszT);
  besFREE(pszT);
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

#ifdef WIN32
static int SetConBreak(int i){
  HANDLE hConsole;
  DWORD Mode;

  GET_CONSOLE_HANDLE
  if( GetConsoleMode(hConsole,&Mode) ){
    if( i ){
      Mode |= ENABLE_PROCESSED_INPUT;
      SetConsoleCtrlHandler(NULL,TRUE);
      }
    else{
      Mode &= ~ ENABLE_PROCESSED_INPUT;
      SetConsoleCtrlHandler(NULL,FALSE);
      }
    SetConsoleMode(hConsole,Mode);
    }
  return 0;
  }
#endif

/**
=section nobreak
=H cio::NoBreak()

After calling this function the ScriptBasic interpreter will ignore
the user pressing control-c.
*/
besFUNCTION(sbnobreak)
#ifdef WIN32
  SetConBreak(1);
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

/**
=section break
=H cio::Break()

After calling this function if the user presses control-c the program terminates.
This is the default behaviour. This function should be used to return to the default
behaviour after calling R<nobreak>.
*/
besFUNCTION(sbbreak)
#ifdef WIN32
  SetConBreak(0);
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

static int GetCSBI(PCONSOLE_SCREEN_BUFFER_INFO q){
  HANDLE hConsole;

  GET_CONSOLE_HANDLE
  GetConsoleScreenBufferInfo(hConsole,q);
  return 0;
  }

#define GET_CSBI(PAR) \
  CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;\
  GetCSBI(&ConsoleScreenBufferInfo);\
  besALLOC_RETURN_LONG;\
  LONGVALUE(besRETURNVALUE) = ConsoleScreenBufferInfo.PAR;

/**
=section BufferSizeX
=H cio::BufferSizeX

This function returns the console buffer horizontal size.

The console buffer is the character buffer that holds all characters
that are visible on the console or that can be scrolled to the console
using the scroll bar.
*/
besFUNCTION(sbsizx)
#ifdef WIN32
  GET_CSBI(dwSize.X)
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

/**
=section BufferSizeY
=H cio::BufferSizeY

This function returns the console buffer vertical size.

The console buffer is the character buffer that holds all characters
that are visible on the console or that can be scrolled to the console
using the scroll bar.
*/
besFUNCTION(sbsizy)
#ifdef WIN32
  GET_CSBI(dwSize.Y)
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

/**
=section CursorX
=H cio::CursorX

Get the actual cursor position. The function returns the horizontal
position of the console cursor.
*/
besFUNCTION(sbcurx)
#ifdef WIN32
  GET_CSBI(dwCursorPosition.X)
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

/**
=section CursorY
=H cio::CursorY

Get the actual cursor position. The function returns the vertical
position of the console cursor.
*/
besFUNCTION(sbcury)
#ifdef WIN32
  GET_CSBI(dwCursorPosition.Y)
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

/**
=section SizeX
=H cio::SizeX

This function returns the horizontal size of the console window in terms
of characters.

*/
besFUNCTION(sbscrx)
#ifdef WIN32
  CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
  GetCSBI(&ConsoleScreenBufferInfo);
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = ConsoleScreenBufferInfo.srWindow.Right -
                              ConsoleScreenBufferInfo.srWindow.Left + 1 ;
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

/**
=section SizeY
=H cio::SizeY

This function returns the vertical size of the console window in terms
of characters.

*/
besFUNCTION(sbscry)
#ifdef WIN32
  CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
  GetCSBI(&ConsoleScreenBufferInfo);
  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = ConsoleScreenBufferInfo.srWindow.Bottom -
                              ConsoleScreenBufferInfo.srWindow.Top + 1;
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

/**
=section PossibleMaxX
=H cio::PossibleMaxX

This function returns the possible maximal horizontal size of the console
that could fit on the screen using the actual character set and screen
resolution.
*/
besFUNCTION(sbmaxx)
#ifdef WIN32
  HANDLE hConsole;
  COORD crd;

  GET_CONSOLE_HANDLE
  crd = GetLargestConsoleWindowSize(hConsole);

  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = crd.X;
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

/**
=section PossibleMaxY
=H cio::PossibleMaxY

This function returns the possible maximal vertical size of the console
that could fit on the screen using the actual character set and screen
resolution.
*/
besFUNCTION(sbmaxy)
#ifdef WIN32
  HANDLE hConsole;
  COORD crd;

  GET_CONSOLE_HANDLE
  crd = GetLargestConsoleWindowSize(hConsole);

  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = crd.Y;
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

/**
=section setwindow
=H cio::SetWindow

This function is supposed to set the actual size of the console. However
this does not work properly and I do not know why.
*/
besFUNCTION(sbsetcw)
#ifdef WIN32
  HANDLE hConsole;
  int iError;
  long x,y,yb;
  SMALL_RECT sr;
  COORD crd;

  if( besARGNR < 3 )return COMMAND_ERROR_FEW_ARGS;

  iError = besGETARGS "iii",&x,&y,&yb besGETARGE
  if( iError )return iError;
  crd.X = (short)x;
  crd.Y = (short)yb;
  
  GET_CONSOLE_HANDLE
  SetConsoleScreenBufferSize(hConsole,crd);

  sr.Left   = 0;
  sr.Top    = 0;
  sr.Right  = (short)x;
  sr.Bottom = (short)yb;
  SetConsoleWindowInfo(hConsole,1,&sr);
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

/**
=section setcursor
=H cio::SetCursor

Use this subroutine to set the cursor size. The argument value gives
the percentages of the height of the cursor relative to the total 
character tallness.
*/
besFUNCTION(sbsetcur)
#ifdef WIN32
  HANDLE hConsole;
  int iError;
  long size;
  CONSOLE_CURSOR_INFO  cci;

  if( besARGNR < 1 )return COMMAND_ERROR_FEW_ARGS;

  iError = besGETARGS "i",&size besGETARGE
  if( iError )return iError;
  cci.dwSize = size;
  cci.bVisible = size ? 1 :0;
  GET_CONSOLE_HANDLE
  SetConsoleCursorInfo(hConsole,&cci);
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND
/**
=section color
=H cio::SetColor col

The the color of the consecutive printings or clear screeen R<cls>.

The argument should be an integer value defining the actual color.
To create this value the constants defined in T<cio.bas> should be used.
These are:

=itemize
=item T<FBlue> the foreground in blue
=item T<FGreen> the foreground is green
=item T<FRed> the foreground in red
=item T<FIntense> the foreground is intense
=item T<BBlue> the background in blue
=item T<BGreen> the background is green
=item T<BRed> the background is red
=item T<BIntense> the background is intense
=noitemize

To set the actual color of the foreground and the background the
programmer may use and expression that T<or> connects these values.
For example

=verbatim
cio::SetColor FBlue or BIntense
=noverbatim

sets the foreground blue and the backgound black with intense bit set (a kind
of light grey). To get the actual colors play a bit with the sample program
T<testconcol.bas> in the directory examples.

To ease different color settings the header file T<cio.bas> defines the following
constants:
=itemize
=item T<FGrey>
=item T<FWhite>
=item T<BGrey>
=item T<BWhite>
=noitemize

*/
besFUNCTION(sbsetcol)
#ifdef WIN32
  HANDLE hConsole;
  int iError;
  long color;

  if( besARGNR < 1 )return COMMAND_ERROR_FEW_ARGS;

  iError = besGETARGS "i",&color besGETARGE
  if( iError )return iError;
  GET_CONSOLE_HANDLE
  SetConsoleTextAttribute(hConsole,(WORD)color);
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND

#define PERR(X,Y) if( !X )return 0
static int cls( HANDLE hConsole ){
    COORD coordScreen = { 0, 0 };    /* here's where we'll home the
                                        cursor */
    BOOL bSuccess;
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
    DWORD dwConSize;                 /* number of character cells in
                                        the current buffer */

    /* get the number of character cells in the current buffer */

    bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );
    PERR( bSuccess, "GetConsoleScreenBufferInfo" );
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

    /* fill the entire screen with blanks */

    bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) ' ',
       dwConSize, coordScreen, &cCharsWritten );
    PERR( bSuccess, "FillConsoleOutputCharacter" );

    /* get the current text attribute */

    bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );
    PERR( bSuccess, "ConsoleScreenBufferInfo" );

    /* now set the buffer's attributes accordingly */

    bSuccess = FillConsoleOutputAttribute( hConsole, csbi.wAttributes,
       dwConSize, coordScreen, &cCharsWritten );
    PERR( bSuccess, "FillConsoleOutputAttribute" );

    /* put the cursor at (0, 0) */

    bSuccess = SetConsoleCursorPosition( hConsole, coordScreen );
    PERR( bSuccess, "SetConsoleCursorPosition" );
    return 0;
 }

/**
=section cls
=H cio::Cls

Clear the console screen and set all character cells to the 
last color settings. See also R<color>.
*/
besFUNCTION(sbcls)
#ifdef WIN32
  HANDLE hConsole;

  GET_CONSOLE_HANDLE
  return cls(hConsole);
#else
  return COMMAND_ERROR_NOTIMP;
#endif
besEND



besVERSION_NEGOTIATE

  return (int)INTERFACE_VERSION;

besEND

besSUB_FINISH

besEND

SLFST CIO_SLFST[] ={

{ "shutmodu" , shutmodu },
{ "bootmodu" , bootmodu },
{ "gotoxy" , gotoxy },
{ "sbkbhit" , sbkbhit },
{ "sbgetch" , sbgetch },
{ "sbgetche" , sbgetche },
{ "sbdetach" , sbdetach },
{ "sbgettitle" , sbgettitle },
{ "sbsettitle" , sbsettitle },
{ "sbnobreak" , sbnobreak },
{ "sbbreak" , sbbreak },
{ "sbsizx" , sbsizx },
{ "sbsizy" , sbsizy },
{ "sbcurx" , sbcurx },
{ "sbcury" , sbcury },
{ "sbscrx" , sbscrx },
{ "sbscry" , sbscry },
{ "sbmaxx" , sbmaxx },
{ "sbmaxy" , sbmaxy },
{ "sbsetcw" , sbsetcw },
{ "sbsetcur" , sbsetcur },
{ "sbsetcol" , sbsetcol },
{ "sbcls" , sbcls },
{ "versmodu" , versmodu },
{ "finimodu" , finimodu },
{ NULL , NULL }
  };
