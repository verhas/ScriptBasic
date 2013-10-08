#! /usr/bin/scriba
' Sample ScriptBasic external preprocessor
'
' This preprocessor converts heb (Html Embedded Basic) files to BASIC
'
' You can embed basic code between
'                          <% basic program %>
' which is executed
'
' <%= basic expression %>
'
cmdlin = command()
split cmdlin by " " to InputFile,OutputFile
open InputFile for input as 1
InputText = input(lof(1),1)
close 1
if Mid$(InputText,1,3) = "#!/" then
  FirstLineLength = InStr(InputText,"\n")
  InputText = Mid$(InputText,FirstLineLength+1)
end if
' import the heb.bas file to get access to the cgi functions
OutputText = "import heb.bas\n"

while  len(InputText) > 0
  StringPosition = InStr(InputText,"<%")
  if not IsDefined(StringPosition) then goto Finished
  AppendString = Mid$(InputText,1,StringPosition-1)
  InputText = Mid$(InputText,StringPosition+2)
  if len(AppendString) > 0 Then
    AppendString = Replace(AppendString,"\\","\\\\")
    AppendString = Replace(AppendString,"\"","\\\"")
    OutputText = OutputText & "print \"\"\"" & AppendString & "\"\"\"\n"
  end if
  StringPosition = InStr(InputText,"%>")
  if not IsDefined(StringPosition) then error(1)
  AppendString = Mid$(InputText,1,StringPosition-1)
  InputText = Mid$(InputText,StringPosition+2)
  if len(AppendString) > 0 then
    if Mid$(AppendString,1,1) = "=" then
      AppendString = "print " & Mid$(AppendString,2)
    end if
    OutputText = OutputText & AppendString & "\n"
  end if
wend

Finished:

if len(InputText) > 0 then
  InputText = Replace(InputText,"\"","\\\"")
  OutputText = OutputText & "print \"\"\"" & InputText & "\"\"\"\n"
end if

open OutputFile for output as 1
print#1,OutputText
close 1
