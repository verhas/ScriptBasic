declare command iff alias "iff" lib "ext_trial"

function true_sub

print "true sub was started\n"
true_sub = "true"
end function

function false_sub

print "false sub was started\n"
false_sub = "false"
end function

a = iff(len("0.00"), true_sub() ,false_sub())

print "The result of a iff is ",a,"\n"

