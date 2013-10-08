module cio

declare sub ::gotoxy        alias "gotoxy"     lib "cio"
declare sub ::kbhit         alias "sbkbhit"    lib "cio"
declare sub ::getch         alias "sbgetch"    lib "cio"
declare sub ::getche        alias "sbgetche"   lib "cio"
declare sub ::detach        alias "sbdetach"   lib "cio"
declare sub ::GetTitle      alias "sbgettitle" lib "cio"
declare sub ::SetTitle      alias "sbsettitle" lib "cio"
declare sub ::break         alias "sbbreak"    lib "cio"
declare sub ::nobreak       alias "sbnobreak"  lib "cio"

declare sub ::SizeX         alias "sbscrx"     lib "cio"
declare sub ::SizeY         alias "sbscry"     lib "cio"
declare sub ::CursorX       alias "sbcurzx"     lib "cio"
declare sub ::CursorY       alias "sbcury"     lib "cio"
declare sub ::BufferSizeX   alias "sbsizx"     lib "cio"
declare sub ::BufferSizeY   alias "sbsizy"     lib "cio"
declare sub ::PossibleMaxX  alias "sbmaxx"     lib "cio"
declare sub ::PossibleMaxY  alias "sbmaxy"     lib "cio"

declare sub ::SetWindow     alias "sbsetcw"    lib "cio"
declare sub ::SetCursor     alias "sbsetcur"   lib "cio"
declare sub ::SetColor      alias "sbsetcol"   lib "cio"

global Const FBlue      = 0x0001
global Const FGreen     = 0x0002
global Const FRed       = 0x0004
global Const FIntense   = 0x0008
global Const BBlue      = 0x0010
global Const BGreen     = 0x0020
global Const BRed       = 0x0040
global Const BIntense   = 0x0080
global Const FGrey      = 0x0007
global Const FWhite     = 0x000F
global Const BGrey      = 0x0070
global Const BWhite     = 0x00F0

declare sub ::Cls           alias "sbcls"      lib "cio"

end module
