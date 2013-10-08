import japi.bas

frame    = japi::frame("")

    menubar  = japi::menubar(frame)
    file$     = japi::menu(menubar,"File")
    edit     = japi::menu(menubar,"Edit")
    options  = japi::menu(menubar,"Options")
    submenu  = japi::menu(options,"Settings")
    help     = japi::helpmenu(menubar,"Help")

    open$     = japi::menuitem(file$,"Open")
    save     = japi::menuitem(file$,"Save")
    japi::seperator(file$)
    quit     = japi::menuitem(file$,"Quit")
    japi::disable(save)
    japi::setshortcut(quit,"q")

    cut      = japi::menuitem(edit,"Cut")
    copy     = japi::menuitem(edit,"Copy")
    paste    = japi::menuitem(edit,"Paste")

    about    = japi::menuitem(help,"About")

    enable   = japi::checkmenuitem(submenu,"Enable Settings")
    settings = japi::menuitem(submenu,"Settings")
    japi::disable(settings)

    japi::show(frame)

while true
        obj=japi::nextaction()

        if obj=enable then
        
            if japi::getstate(enable)= FALSE then
                japi::disable(settings)
            else
                japi::enable(settings)
            endif
        endif
        if obj=cut then
        
            japi::gettext(cut,inhalt)
            if( inhalt = "Cut")then
                japi::settext(cut,"Ausschneiden")
            else
                japi::settext(cut,"Cut")
            endif
        endif
        if(obj = copy)then
            japi::gettext(copy,inhalt)
            if( inhalt = "Copy")then
                japi::settext(copy,"Kopieren")
            else
                japi::settext(copy,"Copy")
            endif
        endif
        if(obj = paste)then
            japi::gettext(paste,inhalt)
            if( inhalt = "Paste" )then
                japi::settext(paste,"Einfuegen")
            else
                japi::settext(paste,"Paste")
            endif
        endif

        if (obj = quit) or (obj = frame) then stop

wend
