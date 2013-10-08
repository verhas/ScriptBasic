import curl.bas
import re.bas

on error goto curle

URL$ = "http://scriptbasic.com/html/documentation.html"

CURL = curl::init()
curl::option CURL,"URL",URL$
curl::option CURL,"PROXY","http://gate-internal.westel900.hu:8080"
print "Getting the page ", URL$,"\n"
HTML$ = curl::perform(CURL)
curl::finish CURL

splita HTML$ BY "\n" TO html_lines

for i=lbound(html_lines) to ubound(html_lines)

  Line$ = html_lines[i]
  if re::match(Line$,"href=\"texi/(.*)\\.pdf\"") then
    URL$  = "http://scriptbasic.com/html/texi/" & re::dollar(1) & ".pdf"
    FILE$ = re::dollar(1) & ".pdf"

    CURL = curl::init()
    curl::option CURL,"URL",URL$
    curl::option CURL,"HTTPHEADER","Accept: */*"
    curl::option CURL,"FILE", FILE$
    curl::option CURL,"PROXY","http://gate-internal.westel900.hu:8080"
    curl::perform(CURL)
    curl::finish CURL

    print "The file ",FILE$," was downloaded successfully.\n"
  end if

next i

print """Program was executed without error.
"""
stop
curle:
print "***ERROR: ",curl::error(CURL),"\n"

