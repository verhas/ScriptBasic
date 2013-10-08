#!/usr/bin/scriba
' This is the ScriptBasic version of the program headerer.pl
'
' This code was created to demonstrate the string handling power of the language. You can not
' totally replace the original headerer.pl by headerer.bas because there may be no interpreter
' for headerer.bas when you want to run it first time to install ScriptBasic.
'
' read a C file and create a header file from it
'
'  Lines processed: HEADER: headerfilename
'                   FILE:   name of the file (used to create header file name unless HEADER is defined)
'                   TO_HEADER:
'                      lines until a single */ on a line are put to the header file
'
'                   /*FUNCTION*/
'                   lines are put to the header file until a single { is found on a line
'                   ; is added after these lines
'
'                   GLOBAL declaration
'                   is put to the header file replcing 'GLOBAL' to 'extern'
'                   define GLOBAL to nothing in the source file
'

cmdlin = command()

START_HERE:
split cmdlin by " " to $file,cmdlin

if $file = "" or not IsDefined($file) then stop

$input_file_name = ""
$header_file_name = ""
$header_file_opened = false
$header_file = undef
$podon = FALSE

fn = 0
open $file for input as fn
$header_content = ""

while not eof(fn)
  line input#fn,$line
  $line = chomp($line)

  if $line LIKE "FILE:*" then
    $input_file_name = trim$(joker(1))
    goto NextLine
  end if

  if $line LIKE "*FILE:*" then
    $input_file_name = trim$(joker(2))
    goto NextLine
  end if

  if $line LIKE "HEADER:*" then
    $header_file_name = trim$(joker(1))
    goto NextLine
  end if

  if $line LIKE "*HEADER:*" then
    $header_file_name = trim$(joker(2))
    goto NextLine
  end if

  if trim$($line) = "TO_HEADER:" then
    gosub open_header_file
    while not eof(fn)
      line input#fn, $line
      $line = chomp($line)
      if trim($line) = "=POD" then
        $podon = TRUE
        goto NextLine10
      end if
      if trim($line) = "=CUT" then
        $podon = FALSE
        goto NextLine10
      end if
      if $line LIKE "~*/" then goto OutOfHeader
      if $line LIKE "*//*" then $line = joker(1)
      if $line LIKE "//*" or $line = "//" then $line = ""
      if $line LIKE "*\\ " then $line = joker(1) & "\\"
      $line = rtrim($line)
      if not $podon then _
        $header_content = $header_content & $line & "\n"
NextLine10:
    wend
OutOfHeader:
    goto NextLine
  end if

  if $line LIKE "/~*FUNCTION~*/" then
    gosub open_header_file
    while not eof(fn)
      line input#fn, $line
      $line = chomp($line)
      if $line LIKE "){" or $line LIKE " ){" then goto OutOfFunctionHeader
      $header_content = $header_content & "\n" & $line
     wend
OutOfFunctionHeader:
    $header_content = $header_content & ");\n"
    goto NextLine
  end if
NextLine:
wend

close fn

$header_content = $header_content & """
#ifdef __cplusplus
}
#endif
#endif
"""

if $header_file_opened then
  fh = 0
  on error goto NoPreviousHeaderFile

  open $open_header_file_name for input as fh
  ' check if the file is identical
  $q = input(lof(fh),fh)
  close fh
  if $q = $header_content then goto START_HERE
NoPreviousHeaderFile:
  fh = 0
  open $open_header_file_name for output as fh
  print#fh,$header_content
  close fh
else
  print "No header was created for ",$file,"\n"
end if
goto START_HERE

'-------------------------------------------
' SUB OPEN_HEADER_FILE
'
open_header_file:

if $header_file_opened then return

$header_file_opened = true
if $header_file_name = "" and $input_file_name = "" then
  if $file LIKE "*.*" then
    $header_file_name = joker(1) & ".h"
  else
    $header_file_name = $file & ".h"
  end if
end if
if $header_file_name = "" then
  if $input_file_name LIKE "*.*" then
    $header_file_name = joker(1) & ".h"
  else
    $header_file_name = $input_file_name & ".h"
  end if
end if
if $header_file_name = "" then
  print "No header file name is defined for file ",$file
  goto START_HERE
end if

' modify the header name so that it is created in the same directory as the source
$dir = $file
' convert \ characters to /
while $dir LIKE "*\\*"
  $dir = joker(1) & "/" & joker(2)
wend
if $dir LIKE "\\*" then $dir = "/" & joker(1)
if $dir LIKE "*\\" then $dir = joker(1) & "/"
if $dir = "\\" then $dir = "/"

if $dir LIKE "*/*" or $dir LIKE "/*" then
  $open_header_file_name = $dir & "/" & $header_file_name
else
  $open_header_file_name = $header_file_name
end if
$header_symbol = ucase($header_file_name)
while $header_symbol LIKE "*/*"
  $header_symbol = joker(2)
wend
$header_symbol = "__" & $header_symbol & "__"
while $header_symbol LIKE "*.*"
  $header_symbol = joker(1) & "_" & joker(2)
wend
$header_content = $header_content & "/*\n" & _
                  $header_file_name & _
                  "\n*/\n#ifndef " & $header_symbol & _
                  "\n#define " & $header_symbol & " 1\n" & _
                  """#ifdef  __cplusplus
extern "C" {
#endif
"""
return

