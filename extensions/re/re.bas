'
' FILE: re.bas
'
' This is the module declaration of the ScriptBasic external module re
'
' To use this module you have to have re.dll or re.so installed in the
' modules directory.
'
' These implement the interface to the hsregex regular expression package

module re

declare sub ::match   alias "match"   lib "re"
declare sub ::m       alias "match"   lib "re"
declare sub ::n       alias "nmatch"  lib "re"
declare sub ::dollar  alias "dollar"  lib "re"
declare sub ::$       alias "dollar"  lib "re"
declare sub ::format  alias "format"  lib "re"
declare sub ::reset   alias "reset"   lib "re"
declare sub ::replace alias "replace" lib "re"
declare sub ::s       alias "replace" lib "re"

Const CaseInsensitive = &HFFFFFFFE
Const MultiLine       = &HFFFFFFFD

Const ErrorNoMatch   = &H00080001
Const ErrorBadPat    = &H00080002
Const ErrorCollate   = &H00080003
Const ErrorCType     = &H00080004
Const ErrorEscape    = &H00080005
Const ErrorSubReg    = &H00080006
Const ErrorBrack     = &H00080007
Const ErrorParen     = &H00080008
Const ErrorBrace     = &H00080009
Const ErrorBadBr     = &H0008000A
Const ErrorRange     = &H0008000B
Const ErrorSpace     = &H0008000C
Const ErrorBadRpt    = &H0008000D
Const ErrorEmpty     = &H0008000E
Const ErrorAssert    = &H0008000F
Const ErrorInvArg    = &H00080010

'     REG_NOMATCH    regexec() failed to match
'     REG_BADPAT     invalid regular expression
'     REG_ECOLLATE   invalid collating element
'     REG_ECTYPE     invalid character class
'     REG_EESCAPE    \ applied to unescapable character
'     REG_ESUBREG    invalid backreference number
'     REG_EBRACK     brackets [ ] not balanced
'     REG_EPAREN     parentheses ( ) not balanced
'     REG_EBRACE     braces { } not balanced
'     REG_BADBR      invalid repetition count(s) in { }
'     REG_ERANGE     invalid character range in [ ]
'     REG_ESPACE     ran out of memory
'     REG_BADRPT     ?, *, or + operand invalid
'     REG_EMPTY      empty (sub)expression
'     REG_ASSERT     ``can't happen''-you found a bug
'     REG_INVARG     invalid argument, e.g. negative-length string


end module
