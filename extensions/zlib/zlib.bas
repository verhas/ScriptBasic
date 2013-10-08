'
' Module zlib
'
' This module lets you use the functions provided by the library zlib
'
' In the current version there are only two functions:
'
'  compress()
'  uncompress()
'
' Both accept a single argument, which is a string. Compress() converts
' the argument to string if needed, but uncompress is strict accepting only
' strings. The argument should not be undef.
'
' Compress compresses the string.
' Uncompress restores the compressed string.
'
module zlib

Const ErrorNoCompress      = &H80100
Const ErrorArgument        = &H80101
Const ErrorCorrupt         = &H80102
Const ErrorCorrupt1        = &H80103

declare sub ::Compress   alias "zlbcmprs" lib "zlib"
declare sub ::UnCompress alias "zlbucprs" lib "zlib"


' compress a file (remove the original by default)
declare sub ::gzip       alias "gzipfunc" lib "zlib"
' decompress a file (remove the original by default)
declare sub ::gunzip     alias "gunzpfnc" lib "zlib"
Const RemoveOriginal = 1
Const LeaveOriginal  = 0

end module
