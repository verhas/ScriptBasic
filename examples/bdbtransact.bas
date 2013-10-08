include bdb.bas
Const nl = "\n"

DB = bdb::open("alma.db",bdb::BTree,bdb::Create,0)

bdb::BeginTransaction
print "transaction started\n"
Counter = bdb::Get(DB,"COUNTER")
print "Counter is ",Counter,nl

If IsDefined(Counter) Then
  Counter = Counter + 1
  bdb::Update DB,Counter
  print "updated\n "
Else
  Counter = 1
  bdb::Put DB,"COUNTER",Counter
  print "put\n "
End If
bdb::EndTransaction
print "transaction has finished\n"

bdb::close(DB)

' bdb::Drop "alma.db"
