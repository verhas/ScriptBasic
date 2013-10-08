; scriba.conf.lsp
; ScriptBasic sample configuration file
;
; Note that this configuration file format is from v1.0b19 or later and has to be compiled
; to internal binary format before starting ScriptBasic

; this is the extension of the dynamic load libraries on this system
dll ".dll"


; where the modules are to be loaded from
module "t:\\ScriptBasic\\modules\\"

; where to search system and module include files
; trailing / or \\ is needed
include "t:\\ScriptBasic\\include\\"

; this is where modules have to install their documentation
docu "t:\\ScriptBasic\\doc\\"

; the maximum number of includes that script basic processes
; this will stop the reading when the programmer makes error
; and includes files recursively. 1000 is the default value anyway
maxinclude 100

;
; define external preprocessors
;
preproc (
  internal (
    test "T:\\MyProjects\\ScriptBasic\\Debug\\preproc.dll"
    dbg "T:\\\\ScriptBasac\\modules\\dbg.dll"
    )

; extensions that preprocessors are to be applied on
  extensions (
; here the key is the file name extension and the value is the symbolic name of the external preprocessor
; for example
;    lsp "lisp"
; would say that all input that has the file name extension '.lsp' should be preprocessed using the external
; preprocessor named 'lisp'. The commad line to start this preprocessor has to be defined a few lines below
; saying:
;   lisp (
;     executable "command line"
;      directory "directory for the temporary files after preprocessing"
;    )
; Note that this is only an example, there is no 'lisp' preprocessor for ScriptBasic or at least I do not know any.
;
     heb "heb"
     )
; the external preprocessors
  external (
    heb (
      executable "t:\\MyProjects\\ScriptBasic\\Release\\scriba.exe t:\\MyProjects\\ScriptBasic\\source\\heber.bas"
      directory "t:\\MyProjects\\ScriptBasic\\hebtemp\\"
      )
    )
  )

;
; LIMIT VALUES TO STOP INIFINITE LOOP
;

; the maximal number of steps allowed for a program to run
; comment it out or set to zero to have no limit
maxstep 0

; the maximal number of steps allowed for a program to run
; inside a function.
; comment it out or set to zero to have no limit
maxlocalstep 0

; the maximal number of recursive function call deepness
; essentially this is the "stack" size
maxlevel 300


; the maximal memory in bytes that a basic program is allowed to use
; for its variables
maxmem 1000000

;
; ScriptBasic loads the modules before starting the code
; in the order they are specified here
;
;preload "ext_trial"

;
; This is the directory where we store the compiled code
; to automatically avoid recompilation
;
cache "t:\\MyProjects\\ScriptBasic\\cache\\"

isapi (
;
; where to write error messages when running the isapi interpreter
;
  report "t:\\MyProjects\\ScriptBasic\\error.log"

;
; When an error happens this file is sent to the client after the
; listed error messages. Error messages are recorded in the error log file
; if there is configured any and are also sent to the client browser.
; after the list of error messages this file is sent. This is actually the tail
; of an HTML file. It has to contain </TT></FONT></BODY></HTML> tags in this
; order with optional extra text between. You probably want to place the webmaster
; eMail address in this text.
;
  errmesfile "t:\\MyProjects\\ScriptBasic\\source\\error.html"

;
; If this config key exists scribais.dll will cache the compiled code
; into memory. To avoid memory caching during development comment this
; config line out.
;
; Note that if there is a code in memory cache it is used even if the code on
; disk has changed! This results fast execution but may be inconvinient during 
; development.
;
;memcache yes

;
; This parameter is used by the CGI module when the interface is ISAPI
; This parameter gives the size of the buffer that the module uses to handle
; POST parameters above 48K (usually uploads). The larger the buffer is the
; faster upload handling can be.
;
  buffer "655360"
  )

cgi (
;
; These are the keys used by the CGI module
;
  debugfile "t:\\MyProjects\\ScriptBasic\\cgidebug.txt"
  )

;
; berkeley db config
;
bdb (

 ; directories where to store the 
 dir (
   home "t:\\MyProjects\\ScriptBasic\\sampledb" ; the home directory of operation of the Berkerley DB
   data "db"  ; database files
   log  "log" ; log files
   temp "tmp" ; temporary files
   )

; ScriptBasic extension modules can access only string configuration values
; therefore you have to specify these numbers within quotes (later versions will change it)
;  limits (
;    lg_max "1024000"
;    mp_mmapsize "0000"
;    mp_size "000"
;    tx_max "000"
;    lk_max "000"
;    )

  ; what lock strategy to use
  ; it can be any of the followings
  ; default oldest random youngest
  lockstrategy default

  flags (
    ; set the value to yes or no
    lock_default no
    init_lock yes
    init_log yes
    init_mpool yes
    init_txn yes
    create yes
    )
  )

;
; MySQL configuration
;
mysql (
  connections (
    test (        ; the name of the connection
	  host "127.0.0.1" ; the host for the connection
	  db "test"   ; database for the connection
	  user "root" ; user for the connection
	  password "" ; password for the connection
	  port 0      ; the port to use
	  socket ""  ; the name of the socket or ""
	  flag 0      ; the client flag
	  clients 10  ; how many clients to serve before really closing the connections
	  )
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
    )
  )

odbc (
  connections (
    test (        ; the name of the connection
	  dsn "test" ; data source name
	  user "" ; user for the connection, to test we use MS-Access thus there is no user or passwd
	  password "" ; password for the connection
	  )
    )
  )

;
; This type of internal preprocessor was experimental and is not supported by ScriptBasic
; since build v1.0b26
; Configure the sample preprocessor
;
; preproc$sample_uppercase "D:/MyProjects/sb/Debug/preproc.dll"
;
;break

;
; Configure the simple ScriptBasic httpd daemon
;
; Note that scripts may change the working directory therefore
; all directories should be specified here full path. For example
; the directory names here do not include the drive c: or e:
; because I develop it on two machines and it was inconvenient
; to alter and recompile the config file each time I moved the
; source to the other machine. When I run a script that changes
; the drive the http daemon stops in a few seconds because the
; guard thread do not find the pid file and therefore tells the
; engine to stop.
;

servers (
  server (
    port 8889
    ip "10.22.2.94"
    protocol http
    )
  server (
    port 21
    ip "127.0.0.1"
    protocol ftp
    salute """220 Eszter SB Application Engine salute text"""
    codebase "T:\\MyProjects\\ScriptBasic\\source\\examples\\ftp\\"
    programs (
      PASS "T:\\MyProjects\\ScriptBasic\\source\\examples\\ftp\\pass.bas"
      ACCT "T:\\MyProjects\\ScriptBasic\\source\\examples\\ftp\\acct.bas"
      )
    )
  threads 20
  listenbacklog 30
  port 8080
  home "t:\\MyProjects\\ScriptBasic\\source\\examples\\"
  proxyip 0 ; set it true if you use Eszter engine behind Apache rewrite and proxy modules
;  ip "127.0.0.1"
  pid (
    file "t:\\MyProjects\\ScriptBasic\\httpdlog\\pid.txt"
    delay 10 ; number of seconds to sleep between the examination of the pid file if that still exists
    wait ( 
      period 10 ; number of times to wait for the threads to stop before forcefully exiting
      length 1  ; number of seconds to wait for the threads to stop (total wait= period*length)
      )
    )

  vdirs (
    dir "/user/:t:\\MyProjects\\ScriptBasic\\source\\examples\\user\\"
    dir "/cgi-bin/:t:\\MyProjects\\ScriptBasic\\source\\examples\\cgi-bin\\"
    dir "/sibawa/:t:\\MyProjects\\ScriptBasic\\source\\examples\\sibawa\\cgi-bin\\"
    )
  client (
    allowed "127.0.0.1/255.255.255.255"
    allowed "16.94.58.4/0.0.0.0"

;    denied "127.0.0.1/0.0.0.0"
;    denied "16.192.68.5/255.255.0.0"
    )
;  run (
;    start   "t:\\MyProjects\\ScriptBasic\\source\\examples\\runstart.bas" ; start the program in an asynchronous thread
;    start "t:\\MyProjects\\ScriptBasic\\source\\examples\\sibawa\\sibawastart.bas"
;    restart "t:\\MyProjects\\ScriptBasic\\source\\examples\\runrestart.bas" ; same as start, but when the program terminates start it again
;    )
  errmsgdest 3
  nolog 0 ; set this true not to use logs or ignore erroneouslog configuration
  log (
    panic ( file "t:\\MyProjects\\ScriptBasic\\httpdlog\\panic.log" )
    app   ( file "t:\\MyProjects\\ScriptBasic\\httpdlog\\app.log" )
    err   ( file "t:\\MyProjects\\ScriptBasic\\httpdlog\\err.log" )
    hit   ( file "t:\\MyProjects\\ScriptBasic\\httpdlog\\hit.log" )
    stat  ( file "t:\\MyProjects\\ScriptBasic\\httpdlog\\stat.log" )
    )
  ; the error page when a page is not found
msg404 """
<HTML>
<HEAD>
<TITLE>Error 404 page not found</TITLE>
</HEAD>
<BODY>
<FONT FACE="Verdana" SIZE="2">
<H1>Page not found</H1>
We regretfully inform you that the page you have requested can not be found on this server.
<p>
In case you are sure that this is a server configuration error, please contact
<FONT SIZE="3"><TT>root@localhost</TT></FONT>
</FONT>
</BODY>
</HTML>
 """
  code404 "200 OK" ; the http error code for page not found. The default is 404
  ; the program to run when a page is not found
  ;run404 "t:\\MyProjects\\ScriptBasic\\source\\examples\\run404.bas"
  mt (
    sleep 55
    )
  )
