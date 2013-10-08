'
' Module hash
'
' This module lets you create memory stored hashes.
'
' Note that refValue and refThisValue return references to
' the actual value and great care should be paid handling
' these directly
'
module hash


const ErrorInvalidHandle = &H80001
const ErrorNoThisKey     = &H80002

'
' Create a new hash and return the handle to the hash
'
' h = hash::New()
declare sub ::New alias "newh" lib "hash"

'
' Set a value in the hash
'
' hash::SetValue h,key,value
declare sub ::SetValue alias "sethv" lib "hash"

'
' Get a value for a given key (note that it returns reference)
'
' q = hash::refValue(h,key)
declare sub ::refValue alias "gethv" lib "hash"

'
' Check that a key exists in the hash (may have undef value)
'
' if hash::Exists(h,key) then ...
declare sub ::Exists alias "ivhv" lib "hash"

'
' Delete a key/value pair from the hash
'
' hash::Delete h,key
declare sub ::Delete alias "delhk" lib "hash"

'
' Start iteration setting the pointer to the first element
'
' hash::Start h
declare sub ::Start alias "starth" lib "hash"

'
' Start iteration backward setting the pointer to the last element
'
' hash::End h
declare sub ::End alias "endh" lib "hash"

'
' Go forward one element
'
' hash::Next h
declare sub ::Next alias "nexthk" lib "hash"

'
' Go backward one element
'
' hash::Prev h
declare sub ::Prev alias "pervhk" lib "hash"

'
' Get the key of the actual key/value pair
'
' k = hash::ThisKey(h)
declare sub ::ThisKey alias "thishk" lib "hash"

'
' Get the reference to the value of the actual key/value pair
'
' v = hash::refThisValue(h)
declare sub ::refThisValue alias "thishv" lib "hash"

'
' Release a hash and free allocated memory
'
' hash::Release h
declare sub ::Release alias "relh" lib "hash"


'
' Get a value for a given key
'
' q = hash::Value(h,key)
function ::Value(h,k)
  ::Value = ByVal ::refValue(h,k)
end function

'
' Get the value of the actual key/value pair
'
' v = hash::ThisValue(h)
function ::ThisValue(h)
  ::ThisValue = ByVal ::refThisValue(h)
end function

end module
