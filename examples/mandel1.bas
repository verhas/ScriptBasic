import japi.bas

function mandel(zre,zim,maxiter)
  local x,y,tmp,betrag
  local iter

  x=0.0
  y=0.0
  iter=0
  betrag=0.0

  while iter < maxiter
    iter += 1
    tmp=x*x-y*y+zre
    y=2*x*y+zim
    x=tmp
    betrag = x*x + y*y
    if betrag > 4 then
      mandel = iter
      exit function
    end if
  wend
  mandel = maxiter
end function

breite=320
hoehe=240
do_work=FALSE

xstart = -1.8
xend   =  0.8
ystart = -1.0
yend   =  1.0

frame   = japi::frame("Variables Mandelbrot")
japi::setborderlayout(frame)

menubar = japi::menubar(frame)
file$    = japi::menu(menubar,"File")
calc     = japi::menu(menubar,"Calc")
quit     = japi::menuitem(file$,"Quit")
start    = japi::menuitem(calc,"Start")
stopp    = japi::menuitem(calc,"Stop")

canvas  = japi::canvas(frame,breite,hoehe)

japi::pack(frame)
japi::show(frame)

'   Waiting for actions
x=-1
y=-1
while true
        if do_work then
            obj=japi::getaction()
        else
            obj=japi::nextaction()
        endif
        if obj = quit then stop

        if(obj = start) then
            x=-1
            y=-1
            do_work=TRUE
            japi::setnamedcolor(canvas,J_WHITE)
        endif
        if(obj = stopp) then do_work=FALSE

        if(do_work) then
            x=(x+1) % breite
            if(x = 0)then y=(y+1) % hoehe
            if((x = breite-1) and (y = hoehe-1)) then
                do_work=FALSE
            else
                zre = xstart + x*(xend-xstart) / breite
                zim = ystart + y*(yend-ystart) / hoehe
                it = mandel(zre,zim,512)
                japi::setcolor(canvas,(it*11),(it*13),(it*17))
                japi::drawpixel(canvas,x,y)
            endif
        endif

        if(obj = canvas)then
            breite = japi::getwidth(canvas)
            hoehe  = japi::getheight(canvas)
            x=-1
            y=-1
        endif

wend
