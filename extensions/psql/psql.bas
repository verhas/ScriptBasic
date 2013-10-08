'
' include file for pgsqlinterf.c
' by pts@fazekas.hu at Sat Jun  8 13:57:20 CEST 2002
' Mon Jun 10 12:14:57 CEST 2002
'

module PGSQL

declare sub PGopen alias "PGopen" lib "pgsql"
declare sub PGclose alias "PGclose" lib "pgsql"
declare sub PGconndefaults alias "PGconndefaults" lib "pgsql"
declare sub PGconnget alias "PGconnget" lib "pgsql"
declare sub PGok alias "PGok" lib "pgsql"
declare sub PGnotified alias "PGnotified" lib "pgsql"
declare sub PGdumpNotices alias "PGdumpNotices" lib "pgsql"
declare sub PGexec alias "PGexec" lib "pgsql"
declare sub PGresultStatus alias "PGresultStatus" lib "pgsql"
declare sub PGmakeEmptyPGresult alias "PGmakeEmptyPGresult" lib "pgsql"
declare sub PGoid alias "PGoid" lib "pgsql"
declare sub PGescapeString alias "PGescapeString" lib "pgsql"
declare sub PGescapeBytea  alias "PGescapeBytea" lib "pgsql"
declare sub PGnrows alias "PGnrows" lib "pgsql"
declare sub PGntuples alias "PGrows" lib "pgsql"
declare sub PGncols alias "PGncols" lib "pgsql"
declare sub PGnfields alias "PGcols" lib "pgsql"
declare sub PGcol alias "PGcol" lib "pgsql"
declare sub PGcoltype alias "PGcoltype" lib "pgsql"
declare sub PGcolmod alias "PGcolmod" lib "pgsql"
declare sub PGcolsize alias "PGcolsize" lib "pgsql"
declare sub PGgetvalue alias "PGgetvalue" lib "pgsql"
declare sub PGgetlength alias "PGgetlength" lib "pgsql"
declare sub PGgetisnull alias "PGgetisnull" lib "pgsql"
declare sub PGbinaryTuples alias "PGbinaryTuples" lib "pgsql"
declare sub PGcmdStatus alias "PGcmdStatus" lib "pgsql"
declare sub PGcmdTuples alias "PGcmdTuples" lib "pgsql"

end module
