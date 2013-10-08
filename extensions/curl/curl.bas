'
' FILE: curl.bas
'
' This is the module declaration of the ScriptBasic external module curl
'
module curl

declare sub ::Init       alias "sb_curl_init"     lib "curl"
declare sub ::Option     alias "sb_curl_option"   lib "curl"
declare sub ::Perform    alias "sb_curl_perform"  lib "curl"
declare sub ::Finish     alias "sb_curl_finish"   lib "curl"
declare sub ::Error      alias "sb_curl_error"    lib "curl"
declare sub ::Info       alias "sb_curl_info"     lib "curl"
declare sub ::Escape     alias "sb_curl_escape"   lib "curl"
declare sub ::Unescape   alias "sb_curl_unescape" lib "curl"
declare sub ::Getdate    alias "sb_curl_getdate"  lib "curl"
declare sub ::Version    alias "sb_curl_version"  lib "curl"
end module
