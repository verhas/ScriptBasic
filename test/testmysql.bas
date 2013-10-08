import mysql.bas

dbh = mysql::Connect("test")
print "The data base handle is: ",dbh,"\n"
' mysql::Shutdown dbh
' print mysql::ErrorMessage(),"\n"
print mysql::Stat(dbh)
print
mysql::query dbh,"delete from users where name='Kakukk'"
print "Affected rows after delete is: ",mysql::AffectedRows(dbh)
print
mysql::query dbh,"insert into users values ('Kakukk',52)"
print "Affected rows after inserting kakukk: ",mysql::AffectedRows(dbh)
print
print "Info is: ",mysql::Info(dbh)
print
mysql::query dbh,"select * from users order by name desc"
print "Affected rows after select: ",mysql::AffectedRows(dbh)
print
' mysql::DataSeek dbh,1
' print
i=0
while mysql::fetcharray(dbh,q)
i=i+1
print i,". ",q[0]," ",q[1]
print
wend
print "Character set name is: ",mysql::CharacterSetName(dbh)
print

mysql::query dbh,"select * from users order by name desc"
print "Affected rows after select: ",mysql::AffectedRows(dbh)
print
' mysql::DataSeek dbh,1
' print
i=0
while mysql::FetchHash(dbh,q)
i=i+1
print i,". "
print
print "name=",q{"name"}
print
print "age=",q{"age"}
print
wend


' mysql::DataSeek dbh,0
on error resume next
mysql::query dbh,"select * from user"
print "Last error is: ",mysql::ErrorMessage(dbh)
print
print "Client info is: ",mysql::GetClientInfo()
print
print "Host info is: ",mysql::GetHostInfo(dbh)
print
print "Proto info is: ",mysql::GetProtoInfo(dbh)
print
print "Server info is: ",mysql::GetServerInfo(dbh)
print

mysql::query dbh,"SHOW PROCESSLIST"
print "Affected rows after show processlistselect: ",mysql::AffectedRows(dbh)
print
i=0
while mysql::fetcharray(dbh,q)
i=i+1
print i,". ",q[0]
' mysql::kill dbh,q[0]
print
wend
print "ping result: ",mysql::Ping(dbh)
print "haho"
print
on error resume next
mysql::Query dbh,"INSERT INTO autoinc values ('huuuh',null)"
print mysql::ErrorMessage(dbh)
print
print mysql::InsertId(dbh)
print

print "Thread id=",mysql::ThreadId(dbh)
print
mysql::kill dbh, mysql::ThreadId(dbh)

mysql::Close(dbh)

