module mt

declare sub ::SetVariable        alias "setmtvariable" lib "mt"
declare sub ::GetVariable        alias "getmtvariable" lib "mt"

declare sub ::LockWrite          alias "lockmtwrite"   lib "mt"
declare sub ::UnlockWrite        alias "unlockmtwrite" lib "mt"
declare sub ::LockRead           alias "lockmtread"    lib "mt"
declare sub ::UnlockRead         alias "unlockmtread"  lib "mt"

declare sub ::NewSessionId       alias "newsession"    lib "mt"
declare sub ::DeleteSession      alias "delsession"    lib "mt"
declare sub ::SetSessionId       alias "setsession"    lib "mt"
declare sub ::GetSessionId       alias "getsession"    lib "mt"
declare sub ::CheckSessionId     alias "chksession"    lib "mt"
declare sub ::SetSessionVariable alias "setsvariable"  lib "mt"
declare sub ::GetSessionVariable alias "getsvariable"  lib "mt"

declare sub ::SessionTimeout     alias "sessionto"     lib "mt"
declare sub ::GetSessionTimeout  alias "getsesto"      lib "mt"
declare sub ::GetSessionPingTime alias "getsesto"      lib "mt"
declare sub ::SessionCount       alias "sessioncount"  lib "mt"
declare sub ::ListSessions       alias "listses"       lib "mt"

end module
