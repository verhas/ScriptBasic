module t
declare sub     ::md5              alias "md5fun"         lib "t"
declare command ::ArrayToString    alias "serialize"      lib "t"
declare command ::ArrayToXML       alias "xmlserialize"   lib "t"
declare sub     ::StringToArray    alias "unserialize"    lib "t"
declare command ::Array2String     alias "serialize"      lib "t"
declare command ::Array2XML        alias "xmlserialize"   lib "t"
declare sub     ::String2Array     alias "unserialize"    lib "t"
declare command ::ArrayToStringMD5 alias "md5serialize"   lib "t"
declare sub     ::StringToArrayMD5 alias "md5unserialize" lib "t"
declare command ::Array2StringMD5  alias "md5serialize"   lib "t"
declare sub     ::String2ArrayMD5  alias "md5unserialize" lib "t"
declare sub     ::SaveString       alias "savestring"     lib "t"
declare sub     ::LoadString       alias "loadstring"     lib "t"
declare sub     ::Exit             alias "toolExit"       lib "t"
end module

