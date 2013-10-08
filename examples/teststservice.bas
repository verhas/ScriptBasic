import nt.bas
print """This test program starts, pauses, continues and stops
the "Removable Storage" service. After each step it waits for user input.
Check the state of the service in the SCM graphical display
pressing F5 to refresh the graphical interface and then
press ENTER in this command window to go on to the next step.
"""
            
Service$ = "NtmsSvc"
nt::StartService Service$
print Service$ & " has been started\n"
line input wait
nt::PauseService Service$ 
print Service$ & " is paused...\n"
line input wait
nt::ContinueService Service$
print Service$ & " is continued...\n"
line input wait
nt::StopService Service$
print Service$ & " has been stopped\n"
line input wait
