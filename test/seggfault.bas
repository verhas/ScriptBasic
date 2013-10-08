t = "<tag>OPEN</tag>just some text<tag>CLOSE</tag>"
PRINT "haho \n"
IF t LIKE "<tag>*</tag>" THEN
 FOR I=1 TO 11
   PRINT I, "=" , JOKER(I),"\n"
 NEXT I  
ELSE
 PRINT "No Match\n"
END IF
