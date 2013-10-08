#!/usr/local/bin/basic -e

include "CGI"

print CGI::Header("Ez a ScriptBasic-próba")


if CGI::Parse() = 0 then
	print "No QUERY_STRING\n"
	end
end if

Egyik = CGI::Values("egyik")
Masik = CGI::Values("masik")

print "<body bgcolor=#FFFFFF>\n"
print "<h1>", Egyik,"</h1>"

print "\n<hr noshade><b>", Masik, "</b><hr noshade>\n"

print CGI::Foot()
