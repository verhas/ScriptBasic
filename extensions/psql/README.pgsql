README for the pgsql ScriptBasic extension
by pts@fazekas.hu at Wed May  8 14:45:42 CEST 2002
user manual Mon Jun 10 17:41:30 CEST 2002

Scriba PGSQL:: documentation
""""""""""""""""""""""""""""
Scriba PGSQL:: is an extension for ScriptBasic (http://www.scriptbasic.com/)
that lets you access PostgreSQL (>=7.0) databases from your ScriptBasic
program. You can connect to a PostgreSQL database server running on the
local host or any other machine on the internet, you can run SQL commands
on the server, and retrieve resulting data rows.

Scriba PGSQL:: is written by pts@fazekas.hu in Early June 2002. The program
is free software, and it is licensed under the GNU GPL >=2.0.

Files
~~~~~
README.pgsql: this documentation
extensions/pgsql/pgsqlinterf.c: C source
include/pgsql.bas: .bas module interface
Makefile.sample.pgsql: sample Makefile, provided as-is

Compilation
~~~~~~~~~~~
1. Download the Scriba PGSQL:: distribution from

	http://www.inf.bme.hu/~pts/scriba-pgsql-latest.tar.gz

2. Download the ScriptBasic sources from http://www.scriptbasic.com/,
   and extract it.

3. Extract the Scriba PGSLL:: distribution to the same directory, i.e the
   extensions/ directory should overlap.

4. Download and install the _development_ package of PostgreSQL. You'll need
   a package with the file libpq-fe.h in it. For example, in Debian systems,
   package postgresql-dev should work.

5. Modify the Makefile: add `pgsql.so' to the line containing `all:'. Add
   the following lines (verbatim, with spaces and tabs correct):

pgsql.so : pgsqlinterf.o
        $(LD) $(LDOPTIONS) -o $@ pgsqlinterf.o -lc -lpq
pgsqlinterf.o   : extensions/pgsql/pgsqlinterf.c
        $(CC) $(CFLAGS)  -c -o $@ extensions/pgsql/pgsqlinterf.c -I/usr/include/postgresql
s_pgsqlinterf.o : extensions/pgsql/pgsqlinterf.c
        $(CC) $(CFLAGSS) -c -o $@ extensions/pgsql/pgsqlinterf.c -I/usr/include/postgresql

   If you don't have the file /usr/include/postgresql/libpq-fe.h, modify
   the lines above, and specify the location of libpq-fe.h accordingly.

6. Compile and install ScriptBasic as usual (see
   http://www.scriptbasic.com). The extra files that belong to the installed 
   Scriba PGSQL:: are: `pgsql.so' and `include/pgsql.bas'.

7. Verify that the following .nas example program compiles and runs and
   prints three lines with -1 in them:

	include pgsql.bas
	PGSQL::PGconndefaults(cd)
	print "conndefaults=(", isarray(cd), ")\n"
	print "escapeBytea: ", PGSQL::PGescapeBytea("a'\000b")="a\\'\\\\000b", "\n"
	print "escapeString: ", PGSQL::PGescapeString("a'\000b")="a''", "\n"

Tutorial
~~~~~~~~
Start your ScriptBasic program with:

	include pgsql.bas

All functions and subroutines belonging tho Scriba PGSQL:: begin with
`PGSQL::PG'.

There are two kinds of PostgreSQL objects: connections and resultsets. You
can refer to these objects with handles (small integer numbers), similar to
the way you use files in ScriptBasic. To issue SQL commands to the
PostgreSQL database, you have to connect the database first (and thus you'll
get the handle of the connection object). To retrieve data rows from a query
to the database, you have to use resultset objects. Connection objects are
created with the PGSQL::PGopen() function, resultset objects are created
with the PGSQL::PGexec() function, and both kinds of objects must be
released by calling PGSQL::PGclose().

Please refer to the PostgreSQL documentation for all non-ScriptBasic
specific aspects of the PostgreSQL RDBMS on the following URL:

	http://www.postgresql.org/users-lounge/docs/7.2/postgres/

This documentation assumes that you are quite familiar with PostgreSQL, you
understand the concepts and you've already succesively used the `psql'
command line client.

To connect to the database, you have to obtain a connection string from
your database administrator, which contains the following information:

-- server hostname
-- database name
-- username
-- password

An example connection string:

	"host='x.y.org' dbname='suicide' user='john' password='doe'"

Connecting example:

	c = PGSQL::PGopen("host='x.y.org' dbname='suicide' user='john' password='doe'")
	if isstring(c) then
	  print "Error connecting: (", rtrim(c), ")\n"
	  stop
	end if

After that, you can issue SQL commands. Example:

	r=PGSQL::PGexec(c, "UPDATE people SET death=NOW() WHERE name=", "john doe", " AND death IS NULL")
	if isstring(r) then
	  print "Error querying: (", rtrim(c), ")\n"
	  stop
	end if
	print "cmdStatus=(", PGSQL::PGcmdStatus(r), ")\n"
	print "cmdTuples=(", PGSQL::PGcmdTuples(r), ")\n"
	PGclose(r)

Please note that string in SQL commands are delimited by single quotes, and
you must escape special characters inside strings (such as the backspace and
the single quote mark). This is automatically done when you call
PGSQL::PGexec. The simple rule is: just supply the string to be quoted as a
separate argument to PGSQL::PGexec(). For example,

	PGSQL::PGexec(c, a, b, s, undef, d, e)

is equilvalent to:

	PGSQL::PGexec(c, a+PGSQL::PGescapeString(b)+s+PGSQLL::PGescapeBytea(d)+e

And an example for SQL queries that return data:

	r=PGSQL::PGexec(c, "SELECT * FROM people")
	if isstring(r) then
	  print "Error querying: (", rtrim(c), ")\n"
	  stop
	end if
	print "cmdStatus=(", PGSQL::PGcmdStatus(r), ")\n"
	print "cmdTuples=(", PGSQL::PGcmdTuples(r), ")\n"

	print "result fields:"
	nf=PGSQL::PGncols(r)  
	' vvv SUXX: Scribe loops must be multiple lines
	for i=0 to nf-1
	  print " ", PGSQL::PGcol(r,i)
	next
	print ".\n"

	nrows=PGSQL::PGnrows(r)
	for row=0 to nrows-1   
	  print "row ", row, ":"
	  for col=0 to nf-1
	    print " "
	    if PGSQL::PGgetisnull(r,row,col) then
	      print "-"
	    end if
	    print PGSQL::PGgetlength(r,row,col)
	    print "(", PGSQL::PGgetvalue(r,row,col), ")"
	  next
	  print ".\n"
	next
	PGclose(r)

Finally, you should close the database connection. Example:

	PGclose(c)

You can guarantee the atomicity of your operations using transactions. Just
issue the queries "BEGIN", "COMMIT" or "ROLLBACK" with PGexec().

User function overview
~~~~~~~~~~~~~~~~~~~~~~
The following functions are provided by Scriba PGSQL:: :

-- PGSQL::PGopen(): open a new database connection; reconnect after
   broken connection (PGSQL::PGok() returned false)
-- PGSQL::PGclose(): close a connection or resultset
-- PGSQL::PGconndefaults(): get default connection properties
-- PGSQL::PGconnget(): get overall properties of an existing connection
-- PGSQL::PGok(): verify that a connection or resultset is still functional
-- PGSQL::PGnotified(): recieve asynchronous notification. See the SQL
   commands LISTEN, UNLISTEN and NOTIFY.
-- PGSQL::PGdumpNotices(): enable or disable dumping notice (not
   notification!) messages to stderr
-- PGSQL::PGexec(): execute a query and return a resultset
-- PGSQL::PGresultStatus(): query the status of the query execution
-- PGSQL::PGmakeEmptyPGresult(): create and return an empty resultset
-- PGSQL::PGoid(): return the OID of the last SQL INSERT operation
-- PGSQL::PGescapeString(): escape a non-binary string to be included into
   an SQL command
-- PGSQL::PGescapeBytea(): escape a bytea (binary 8-bit string) to be
   included to an SQL command
-- PGSQL::PGnrows(): get the number of rows of a resultset
-- PGSQL::PGncols(): get the number of rows of a resultset
-- PGSQL::PGcol(): get a column index by its name and vice versa
-- PGSQL::PGcoltype(): get a column's data type
-- PGSQL::PGcolmod(): get a column's data type modifier
-- PGSQL::PGcolsize(): get a column's storage size on the server
-- PGSQL::PGgetvalue(): get a cell value (always a string)
-- PGSQL::PGgetlength(): get a cell's length
-- PGSQL::PGgetisnull(): return whether a cell is null
-- PGSQL::PGbinaryTuples(): return whether cells are returned as binary
-- PGSQL::PGcmdStatus(): get the status message of the completed query
-- PGSQL::PGcmdTuples(): get the number of rows affected (not _returned_)
   by a query

libpq is a PostgreSQL client library that provides almost the same
functionality as Scriba PGSQL::, but for the C language. Scriba PGSQL:: uses
libpq to do all operations.

PostgreSQL doc says:

> libpq is thread-safe as of PostgreSQL 7.0, so long as no two threads attempt
> to manipulate the same PGconn object at the same time. In particular, you
> cannot issue concurrent queries from different threads through the same
> connection object. (If you need to run concurrent queries, start up multiple
> connections.)

Scriba PGSQL:: is written to be thread-safe, but this hasn't been tested
yet. You don't have to worry about using the same connection object in
multiple threads, since it is inherently impossible due to the ScriptBasic
architecture.

Comparison to libpq
~~~~~~~~~~~~~~~~~~~
libpq and Scriba PGSQL:: provide essentially the same functionality with
very similar functions. libpq function begin with `PQ', Scriba PGSQL::
functions begin with `PGSQL::PG'.

The following libpq functions are missing from Scriba PGSQL::, because they
are related to asynchronous, nonblocking operations: PQconnectStart(),
PQconnectPoll(), PQresetStart(), PQsetnonblocking(), PQisnonblocking(),
PQsendQuery(), PQgetResult(), PQconsumeInput(), PQisBusy(), PQflush(),
PQsocket(), PQrequestCancel().

The following libpq functions are obsolete/deprecated, thus they are not
supported in Scriba PGSQL: PQsetdblogin(): obsoleted by PQconnectdb(),
PQsetdb(): obsoleted by PQconnectdb(), PQoidStatus(): obsoleted by
PQoidValue()

The SQL COPY command not supported in Scriba PGSQL::, thus the following
libpq functions have no Scriba PGSQL:: counterpart: PQgetline(),
PQgetlineAsync(), PQputline(), PQputnbytes(), PQendcopy().

Tracing is not supported in Scriba PGSQL::, thus the following
libpq functions have no Scriba PGSQL:: counterpart:
PQtrace(), PQuntrace().

Large objects are not supported in Scriba PGSQL::, thus the following
libpq functions have no Scriba PGSQL:: counterpart: lo_creat(), lo_import(),
lo_export(), lo_open(), lo_write(), lo_read(), lo_lseek(), lo_close(),
lo_unlink().

The libpq PQsetNoticeProcessor() function has been simplified to
PGSQL::PGdumpNotices().

All other libpq functions have equivalent counterpart in Scriba PGSQL::. See
the function reference for function-specific details.

For more information about libpq, please visit the URL:

	http://www.postgresql.org/users-lounge/docs/7.2/postgres/libpq-connect.html

Function reference
~~~~~~~~~~~~~~~~~~
Parameter and return value data types are significant in Scriba PGSQL::.
You should use the ScriptBasic isstring() and isundef() functions to
distinguish between different data types.

For more details, please read the documentation of the corresponding libpq
function, available from the URL:

	http://www.postgresql.org/users-lounge/docs/7.2/postgres/libpq-connect.html

The Scriba PGSQL:: functions:

-- PGconn|String PGopen(String conns):
   Open a new database connection.
   @libpq PQconnectdb(), PQreset(), PQstatus(), PQerrorMessage(),
     [PQfinish(), PQsetNoticeProcessor()]
   @return new connection handle PGconn or a String describing the error. The
     string usually contains one or more terminating newlines, so you should
     call rtrim() on it

-- PGconn|String PGopen(PGconn old):
   Reconnect after broken connection (PGSQL::PGok() returned false).
   @libpq PQconnectdb(), PQreset(), PQstatus(), PQerrorMessage(),
     [PQfinish(), PQsetNoticeProcessor()]
   @return same connection handle `old'

-- void PGclose(PGconn old):
   Close a connection.
   @libpq PQfinish()

-- void PGclose(PGresult old):
   Close a resultset. 
   @libpq PQclear()

-- void PGconndefaults(Array &ret):
   Get default connection properties.
   @libpq PQconndefaults(),  PQconninfofree()
   @return new array of assoc arrays. Assoc array keys: "keyword", "envvar",
     "compiled", "val", "label", "dispchar", "dispsize"

-- String|int|bool PGconnget(PGconn conn, String key):
   Get overall properties of an existing connection.
   @libpq PQuser(), PQpass(), PQhost(), PQport(), PQtty(), PQoptions(),
   PQbackendPID(), PQgetssl()
   @param key one of "db" | "user" | "pass" | "host" | "port" | "tty" |
     "options" | "backendPID" | "SSLused"

-- bool PGok(PGconn|PGresult conn):
   Verify that a connection or resultset is still alive and functional.
   @libpq PQstatus(), PQresultStatus()
   @return true iff CONNECTION_OK || PGRES_TUPLES_OK || PGRES_COMMAND_OK
           (0 for false, -1 for true)

-- bool PGnotified(PGconn conn, String& relname, int& other_pid):
   Recieve asynchronous notification. See the SQL
   commands LISTEN, UNLISTEN and NOTIFY.
   @libpq PQconsumeInput(), PQnotifies()
   @return 0 if there are no pending notifications, undef if PQconsumeInput()
     failed; returns 1 otherwise

-- void PGdumpNotices(PGconn conn, bool disp):
   Enable or disable dumping notice (not notification!) messages to stderr.
   @libpq PQsetNoticeProcessor()
   @param disp when true, libpq notice messages are printed to stderr.
     When false, no notices are printed
   @return undef

-- PGresult|String PGexec(PGconn conn, String query [,String qarg[, String|int Query_cont [...]]] ):
   Execute a query and return a resultset.
   Ordering: query and every second string is unquoted; quoted strings are
   quoted with PQescapeString() unless they are preceded by undef (with
   the undef, PQescapeBytea() is appled). Quoted strings are surrounded with
   single quotes.
   @libpq PQexec(), PQescapeString(), PQescapeBytea(), PQresultStatus(),
     PQresStatus(), PQresultErrorMessage(), [PQclear()]
   @return new PGresult handle or error message String. The error message is
   the concatenation of a PGRES_* constant, a colon, a space, and a
   descriptive English error message.

-- String PGresultStatus(PGresult res):
   Query the status of the query execution.
   @libpq PQresultStatus(), PQresStatus(), PQresultErrorMessage()
   @return "" if no error and PGRES_TUPLES_OK;
           "-" if no error and PGRES_COMMAND_OK;
           "PGRES_*: " and error message otherwise

-- PGresult PGmakeEmptyPGresult(PGconn conn, String resultStatus):
   Create and return an empty resultset.
   @libpq PQmakeEmptyPGresult(), [PQclear()]
   @param conn may be undef
   @param resultStatus one of PGRES_EMPTY_QUERY PGRES_COMMAND_OK
          PGRES_TUPLES_OK PGRES_COPY_OUT PGRES_COPY_IN PGRES_BAD_RESPONSE
          PGRES_NONFATAL_ERROR PGRES_FATAL_ERROR

-- undef|int PGoid(PGresult):
   Return the OID of the last SQL INSERT operation.
   @libpq PQoidValue()
   @return the object ID of the inserted row, if the SQL command was an
     INSERT that inserted exactly one row into a table that has OIDs.
     Otherwise, returns undef.

-- String PGescapeString(String s):
   Escape a non-binary string to be included into
   an SQL command.
   Surrounding single quotes not included.
   @libpq PQescapeString()

 String PGescapeBytea(String s):
   Escape a bytea (binary 8-bit string) to be included to an SQL command.
   Surrounding single quotes not included.
   @libpq PQescapeBytea()

-- int PGnrows(PGresult res):
   Get the number of rows of a resultset.
   An undocumented alias is available for this function: PGntuples()
   @libpq PQntuples()

-- int PGncols(PGresult res):
   Get the number of rows of a resultset.
   An undocumented alias is available for this function: PGnfields()
   @libpq PQnfields()

-- String|null PGcol(PGresult res, int idx):
   Convert a column index to a column name.
   @libpq PQfname()
   @param idx >=0

-- int|undef PGcol(PGresult res, String colname):
   Convert a column name to a column index
   @libpq PQfnumber()
   @return int >=0 or undef

-- int|undef PGcoltype(PGresult res, int idx):
   Get a column's data type.
   The user should query the system table pg_type.
   @libpq PQftype()
   @return int (Oid), or undef

-- int|undef PGcoltype(PGresult res, String colname)
   Get a column's data type.
   The user should query the system table pg_type.
   @libpq PQftype(), PQfnumber()
   @return int (Oid), or undef

-- int|undef PGcolmod(PGresult res, int idx):
   Get a column's data type modifier.
   The user should query the system table pg_mod.
   @libpq PQfmod()
   @return int (Oid), or undef

-- int|undef PGcolmod(PGresult res, String colname)
   Get a column's data type modifier.
   The user should query the system table pg_mod.
   @libpq PQfmod(), PQfnumber()
   @return int (Oid), or undef

-- int|undef PGcolsize(PGresult res, int idx):
   Get a column's storage size on the server.
   The user should query the system table pg_size.
   @libpq PQfsize()
   @return int (Oid), or undef

-- int|undef PGcolsize(PGresult res, String colname):
   Get a column's storage size on the server.
   The user should query the system table pg_size.
   @libpq PQfsize(), PQfnumber()
   @return int (Oid), or undef

-- String|undef PGgetvalue(PGresult res, int tup_num, int idx):
   Get a cell value (always a string).
   @libpq PQgetvalue()
   @return undef when SQL NULL or invalid column index 

-- String|undef PGgetvalue(PGresult res, int tup_num, String colname):
   Get a cell value (always a string).
   @libpq PQgetvalue(), PQfnumber()
   @return undef when SQL NULL or invalid column name

-- int|undef PGgetlength(PGresult res, int tup_num, int idx):
   Get a cell's length.
   @libpq PQgetlength()
   @return int >=0 or undef if invalid column index

-- int|undef PGgetlength(PGresult res, int tup_num, String colname):
   Get a cell's length.
   @libpq PQgetlength(), PQfnumber()
   @return int >=0 or undef if invalid column name

-- bool|undef PGgetisnull(PGresult res, int tup_num, int idx):
   Return whether a cell is null.
   @libpq PQgetisnull()
   @return int (bool) or undef if invalid column index

-- bool|undef PGgetisnull(PGresult res, int tup_num, String colname):
   Return whether a cell is null.
   @libpq PQgetisnull(), PQfnumber()
   @return int (Oid), or undef if invalid column name 

-- bool PGbinaryTuples(PGresult):
   Return whether cells are returned as binary.
   @libpq PQbinaryTuples()

-- String PGcmdStatus(PGresult res):
   Get the status message of the completed query.
   @libpq PGcmdStatus()
   @return String: the command status (such as `CREATE' or `UPDATE 1')

-- int PGcmdTuples(PGresult res):
   Get the number of rows affected (not _returned_) by a query.
   @libpq PGcmdTuples()
   @return int: the number of rows affected for INSERT, UPDATE, DELETE etc.

Random notes from the author

!! : .c eleje, vége
!! : dist
OK : pgsql.bas
OK : no DEBUGMSG()
OK : docs
OK : thread safety declaration
Imp: docs with methodology (mySQL sincs dokumentálva)
Imp: lock for PGexec() [??]
OK : error rets
OK : what if error retrieving column info
Imp: what's the difference between PQescapeString() and PQescapeBytea()
Imp: a more general PGdumpNotices
Imp: PGexecRaise() function, raising errors more often
Not: call PGclose() when no more refs (impossible in ScriptBasic)
Dat: libpq is not binary-safe, ie. strings are terminated by '\0'
Dat: text, varchar etc. may not contain '\0'; bytea may
Dat: bytea may contain '\0'
Dat: PGconn and PGresult objects are independent: PQfinish() may be called
     before PQgetvalue()
Dat: there is no statement precompilation in PostgreSQL 7.0:
     compile("SELECT name FROM employee WHERE salary < ?"). There isn't in
     MySQL either. There is in Oracle8.
Dat: a SciptBasic associative array is an array with an even number of
     elements (key0, value0, key1, value1, ...) (barely documented in
     http://www.scriptbasic.com/html/texi/devguide.html)
Dat: we do _not_ free temporarily allocated memory when out of mem
Dat: ScriptBasic v1.0 Build29 cannot display module-specific error messages,
     just their numbers. Example: ``(0): error 0x00081001:Extension specific
     error: %s''. I have reported the bug.
Dat: SUXX: cannot handle reference variables inside extensions
Dat: run: ./cftc ss ss.bin; ./scriba -d -f ss.bin pgtest.bas
Dat: ScriptBasic error handling is not used, because it seems to be inadequate
     for reporting arbitrary string errors.


__END__ of README
