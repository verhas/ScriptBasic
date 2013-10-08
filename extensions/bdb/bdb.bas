'
' FILE: bdb.bas
'
' This is the module declaration of the ScriptBasic external module bdb
'
' To use this module you have to have bdb.dll or bdb.so installed in the
' modules directory.
'
' These implement the interface to the Berkeley Data Base library

module bdb

' Error codes
Const ErrorInvalidFileName      = &H80001
Const ErrorInvalidDbHandle      = &H80002
Const ErrorTransactionnotClosed = &H80003
Const ErrorTransactionNotOpened = &H80004
Const ErrorKeyNotFound          = &H80005
Const ErrorIncomplete           = &H80006
Const ErrorKeyEmpty             = &H80007
Const ErrorKeyExist             = &H80008
Const ErrorLockDeadLock         = &H80009
Const ErrorLockNotGranted       = &H8000A
Const ErrorNotFound             = &H8000B
Const ErrorOldVersion           = &H8000C
Const ErrorRunRecovery          = &H8000D
Const ErrorDeleted              = &H8000E
Const ErrorNeedSplit            = &H8000F
Const ErrorSwapBytes            = &H80010
Const ErrorTxnCkp               = &H80011


' Table types
Const BTree   = &H01
Const Hash    = &H02
Const Recno   = &H04
Const Queue   = &H08
Const Unknown = &H10

' put flags
Const Append      = &HFFFFFFFE
Const NoOverWrite = &HFFFFFFFD

' Table opening flags
Const Create   = &HFFFFFFFE
Const NoMap    = &HFFFFFFFD
Const RdOnly   = &HFFFFFFFB
Const Thread   = &HFFFFFFF7
Const Trunc    = &HFFFFFFEF
Const New      = &HFFFFFFDF

' open the database
' DB = bdb::Open(DataBase,type,flags,unixmode)
declare sub ::Open alias "sb_db_open" lib "bdb"

' close the DB
' bdb::Close DB
declare sub ::Close alias "sb_db_close" lib "bdb"

' put a new, possibly duplicated key/value pairinto DB
' bdb::Put DB,key,value
declare sub ::Put alias "sb_db_put" lib "bdb"

' update the last accessed record
' bdb::Update DB,value
declare sub ::Update alias "sb_db_update" lib "bdb"
' delete the last accessed record
' bdb::DeleteRecord DB
declare sub ::DeleteRecord alias "sb_db_eracrec" lib "bdb"

' get value matching the key from DB
' if there are more than one values for the same key return the first one
' value = bdb::Get(DB,key) 
declare sub ::Get alias "sb_db_get" lib "bdb"
' value = bdb::First(DB,key) or value = bdb::First(DB)
declare sub ::First alias "sb_db_get" lib "bdb"

' get the last value from DB
' value = bdb::Last(DB)
declare sub ::Last alias "sb_db_last" lib "bdb"

' get the next value for the key or undef if there are no more
' value = bdb::Next(DB,key) 
declare sub ::Next alias "sb_db_next" lib "bdb"
' get the previous value for the key or undef if there are no more
' value = bdb::Previous(DB,key) 
declare sub ::Previous alias "sb_db_prev" lib "bdb"

' delete all records for the given key
' bdb::DeleteAll DB,key
declare sub ::DeleteAll alias "sb_db_del" lib "bdb"

' get the key of the last accessed record
' bdb::Key(DB)
declare sub ::Key alias "sb_db_key" lib "bdb"

' transaction handling, no arguments
declare sub ::BeginTransaction alias "sb_db_transact" lib "bdb"
declare sub ::CommitTransaction alias "sb_db_trcommit" lib "bdb"
declare sub ::EndTransaction alias "sb_db_trcommit" lib "bdb"
declare sub ::AbortTransaction alias "sb_db_trabort" lib "bdb"

' delete a database table
' bdb::Drop "databasename"
declare sub ::Drop alias "sb_db_remove" lib "bdb"

end module

