#! /usr/bin/scriba
import mysql.bas
import t.bas

print """This program creates the tables for the SIBAWA application.
Running this program destroys all data that may have been previously
stored in the tables. If you really want that type 'yes' (all lower
case letters without the quote characters) and press enter.
"""
line input yesorno$
yesorno$ = chomp(yesorno$)
if yesorno$ <> "yes" then
  stop
end if
'
' this keyword is not known by older MySQL
' if that does not work, then we try to create the tables without it
'
UNIQUE$ = "UNIQUE"

on error goto ErrorConnectingToAuth
dbh = mysql::Connect("auth")

query = "DROP TABLE IF EXISTS USERS"
call mysql::query dbh,query
print query
print

'
' USERS contains the data of the users using the application
'
CreatingUsersTable:
on error goto ErrorCreatingUsersTable
mysql::query dbh,"""CREATE TABLE USERS (id INTEGER UNSIGNED AUTO_INCREMENT PRIMARY KEY,
                                        name CHAR(64) NOT NULL """ & UNIQUE$ & """,
                                        password CHAR(64) NOT NULL,
                                        realname CHAR(80),
                                        acl INTEGER UNSIGNED,
                                        owner INTEGER UNSIGNED,
                                        priv CHAR(255),
                                        INDEX i_name (name)
                                       )"""
print "Table USERS was successfully created\n"

on error goto GeneralDatabaseError
query = "DROP TABLE IF EXISTS GROUPS"
call mysql::query dbh,query
print query
print

'
' GROUPS contain the data of the user groups
' this table does not list the members of the groups
'
mysql::query dbh,"""CREATE TABLE GROUPS (id INTEGER UNSIGNED AUTO_INCREMENT PRIMARY KEY,
                                         name CHAR(64) NOT NULL """ & UNIQUE$ & """,
                                         description CHAR(255),
                                         acl INTEGER UNSIGNED,
                                         owner INTEGER UNSIGNED,
                                         INDEX i_name (name)
                                        )"""
print "Table GROUPS was successfully created\n"

query = "DROP TABLE IF EXISTS GROUPMEMBERS"
call mysql::query dbh,query
print query
print

'
' GROUPMEMBERS join the groups and the users
'
mysql::query dbh,"""CREATE TABLE GROUPMEMBERS (user_id INTEGER UNSIGNED NOT NULL,
                                               group_id INTEGER UNSIGNED NOT NULL,
                                               INDEX i_ug (user_id,group_id)
                                               )"""
print "Table GROUPMEMBERS was successfully created\n"

'
' ROLES contain the data of the user roles
'
query = "DROP TABLE IF EXISTS ROLES"
call mysql::query dbh,query
print query
print
mysql::query dbh,"""CREATE TABLE ROLES (id INTEGER UNSIGNED AUTO_INCREMENT PRIMARY KEY,
                                        name CHAR(64) NOT NULL """ & UNIQUE$ & """,
                                        description CHAR(255),
                                        INDEX i_name (name)
                                        )"""
print "Table ROLES was successfully created\n"

'
' Insert the general roles that are used by the ACL objects
'
query = "INSERT INTO ROLES (NAME,DESCRIPTION) VALUES ('owner','owner of any object')"
call mysql::query dbh,query
print query
print

AdministratorPassword = "12345678"
print AdministratorPassword ,"\n"
AdministratorPrivileges = "admin"
mysql::query dbh,"""INSERT INTO USERS (name,password,realname,priv) VALUES
                                      ('Administrator','""" & AdministratorPassword & """','Administrator User',
                                       '""" & AdministratorPrivileges & """'
                                      )"""
print "Initial user named \"Administrator\" was successfully created\n"

query = "DROP TABLE IF EXISTS ACES"
call mysql::query dbh,query
print query
print

mysql::query dbh,"""CREATE TABLE ACES (ID INTEGER UNSIGNED AUTO_INCREMENT NOT NULL PRIMARY KEY,
                                       GUID INTEGER UNSIGNED NOT NULL,
                                       ACLID INTEGER UNSIGNED NOT NULL,
                                       ACETYPE CHAR(1),
                                       ACEVAL CHAR(32)
                                       )"""
print "Table ACES was successfully created\n"

query = "DROP TABLE IF EXISTS ACLS"
call mysql::query dbh,query
print query
print

mysql::query dbh,"""CREATE TABLE ACLS (ID INTEGER UNSIGNED AUTO_INCREMENT NOT NULL PRIMARY KEY,
                                       ACLID INTEGER UNSIGNED,
                                       OWNER INTEGER UNSIGNED,
                                       ACLTYPE INTEGER UNSIGNED,
                                       NAME CHAR(64),
                                       DESCRIPTION CHAR(255)
                                       )"""
print "Table ACLS was successfully created\n"

query = "DROP TABLE IF EXISTS ACLTYPES"
call mysql::query dbh,query
print query
print

query = """CREATE TABLE ACLTYPES (ID INTEGER UNSIGNED AUTO_INCREMENT NOT NULL PRIMARY KEY,
                                  NAME CHAR(64),
                                  ACL INTEGER,
                                  DESCRIPTION CHAR(255),
"""
for i=1 to 32
  query = query & "BIT" & i & " CHAR(16), DESCRIPTION" & i & " CHAR(255)"
  if i < 32 then query = query & ","
next i
query = query & ")"

print query

mysql::query dbh,query
print "Table ACLTYPES was successfully created\n"

query = "INSERT INTO ACLTYPES (NAME,ACL,DESCRIPTION,BIT1,BIT2) VALUES ('ACL',NULL,'controll access to ACLs','read','write')"
mysql::query dbh,query
print query


'
' DEFINING FORMS
'

'
' this table contains the core information of all forms of any type
'
query = "DROP TABLE IF EXISTS ALLFORMS"
call mysql::query dbh,query
print query
print
query = """CREATE TABLE ALLFORMS (ID INTEGER UNSIGNED AUTO_INCREMENT NOT NULL PRIMARY KEY,
                                  ACL INTEGER,                 # acl that governs access to this form
                                  OWNER INTEGER,               # user id of the owner of the form
                                  PARENT INTEGER,              # parent form id
                                  FORMTYPE CHAR(64)            # name of the table that contains the form specific data
                                 )"""
mysql::query dbh,query
print query


query = """CREATE TABLE FORMTYPES (ID INTEGER UNSIGNED AUTO_INCREMENT NOT NULL PRIMARY KEY,
                                   NAME CHAR(64),           # the name of the type, table name
                                   DEFSTRING CHAR(255)      # defintion of the form, name of the columns comma separated
                                  )"""
mysql::query dbh,query
print query

end

ErrorConnectingToAuth:

print """***ERROR***
The program can not connect to the database 'auth'. Check the followings:

1. MySQL is installed and running on the database server machibe.
2. MySQL ScriptBasic module is installed.
3. The connection named 'auth' is configured in theScriptBasic
   configuration file, for example:

    auth  (
	  host "127.0.0.1" ; the host for the connection
	  db "auth"   ; database for the connection
	  user "root" ; user for the connection
	  password "" ; password for the connection
	  port 0      ; the port to use
	  socket ""   ; the name of the socket or ""
	  flag 0      ; the client flag
	  clients 10  ; how many clients to serve before really closing the connections
          )

4. The database 'auth' exists."""
stop
ErrorCreatingUsersTable:
if UNIQUE$ = "UNIQUE" then
  REM may be that the problem is the UNIQUE keyword
  REM give it a try without the keyword
  UNIQUE$ = ""
  print """It seems that you have an older version of MySQL that can not handle
UNIQUE fields. I will try to create the table without the UNIQUE keyword."""
  resume CreatingUsersTable
end if
print """***ERROR***
The program can not create the table USERS. I have no idea why.
Check the permissions of the DB user configured in the connection.
"""
print mysql::ErrorMessage(dbh)
stop

GeneralDatabaseError:
print "***ERROR***\n"
print mysql::ErrorMessage(dbh)
stop
