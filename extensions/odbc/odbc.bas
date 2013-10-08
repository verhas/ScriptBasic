' FILE_ odbc.bas
'
' This is an include file to define the ODBC functions for the ScriptBasic module ODBC
'

module odbc

declare sub ::Connect alias "odbc_config_connect" lib "odbc"
declare sub ::RealConnect alias "odbc_real_connect" lib "odbc"
declare sub ::Close alias "odbc_close" lib "odbc"
declare sub ::Query alias "odbc_query" lib "odbc"
declare sub ::FetchArray alias "odbc_fetcharray" lib "odbc"
declare sub ::FetchHash alias "odbc_fetchhash" lib "odbc"
declare sub ::AffectedRows alias "odbc_affected_rows" lib "odbc"
declare sub ::Error alias "odbc_error" lib "odbc"
end module
