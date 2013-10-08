include bdb.bas
Const nl = "\n"

DB = bdb::open("alma.db",bdb::BTree,bdb::Create,0)

bdb::BeginTransaction
print "transaction started\n"
Counter = bdb::Get(DB,"COUNTER")
print "Counter is ",Counter,nl
print "Press enter to continue...\n"
line input wait$

If IsDefined(Counter) Then
  Counter = Counter + 1
  bdb::Update DB,Counter
  print "updated\nPress enter to continue...\n"
  line input wait$
Else
 Counter = 1
 bdb::Put DB,"COUNTER",Counter
  print "put\nPress enter to continue...\n"
  line input wait$
End If
bdb::EndTransaction
print "transaction has finished\n"

bdb::close(DB)

' bdb::Drop "alma.db"


