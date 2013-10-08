import "../sibawa.ini"

cgi::Header 200,"text/html"
cgi::FinishHeader

if HasPrivilege("admin") then
  PrintTemplate("admin/index.html")
else
  PrintTemplate("index.html")
endif

end
