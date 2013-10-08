import "../sibawa.ini"

CheckPrivilege "admin"

cgi::Header 200,"text/html"
cgi::FinishHeader

PrintTemplate("admin/newgroup.html")

stop
end
