import "../sibawa.ini"

mt::DeleteSession

cgi::Header 200,"text/html"
cgi::FinishHeader
PrintTemplate "logout.html"
end
