'
' Module gd
'
module gd

Const ::Styled        = -2
Const ::Brushed       = -3
Const ::StyledBrushed = -4
Const ::Tiled         = -5
' Const Transparent   = -6 use undef instead, it is converted to transparent automatically

Const FontTiny   = 1
Const FontSmall  = 2
Const FontMedium = 3
Const FontLarge  = 4
Const FontGiant  = 5

declare sub ::Create alias "gdic" lib "gd"
declare sub ::CreateFromPng alias "gdicfpng" lib "gd"
declare sub ::Color alias "gdac" lib "gd"
declare sub ::Line alias "gdline" lib "gd"
declare sub ::_Rect alias "gdrect" lib "gd"
declare sub ::Point alias "gdpix" lib "gd"
declare sub ::SavePng alias "impng" lib "gd"
declare sub ::Png alias "impngstr" lib "gd"
declare sub ::Destroy alias "imdestr" lib "gd"
declare sub ::SizeX alias "getxdime" lib "gd"
declare sub ::SizeY alias "getydime" lib "gd"
declare sub ::_Poly alias "gdpoly" lib "gd"
declare sub ::Arc alias "gdarc" lib "gd"
declare sub ::FillToBorder alias "gdfilltb" lib "gd"
declare sub ::Fill alias "gdfill" lib "gd"
declare sub ::SetBrush alias "gdstbrus" lib "gd"
declare sub ::SetTile alias "gdstile" lib "gd"
declare sub ::LineStyle alias "gdsstyle" lib "gd"
declare sub ::GetColorComponent alias "gdgetc" lib "gd"
declare sub ::IsXYInside alias "gdbons" lib "gd"
declare sub ::SetFont alias "gdfont" lib "gd"
declare sub ::SetColor alias "gdcolor" lib "gd"
declare sub ::SetTransparentColor alias "gdtrnspa" lib "gd"
declare sub ::print alias "gdprint" lib "gd"

sub ::Poly(Image,X,Y,Color)
  ::_Poly Image,X,Y,Color,0
end sub
sub ::FilledPoly(Image,X,Y,Color)
  ::_Poly Image,X,Y,Color,1
end sub

sub ::Rectangle(Image,x1,y1,x2,y2,color)
  ::_Rect Image,x1,y1,x2,y2,color,0
end sub
sub ::FilledRectangle(Image,x1,y1,x2,y2,color)
  ::_Rect Image,x1,y1,x2,y2,color,1
end sub

function ::GetRedComponent(image,color)
  ::GetRedComponent = ::GetColorComponent(image,color,1)
end function
function ::GetGreenComponent(image,color)
  ::GeGreenComponent = ::GetColorComponent(image,color,2)
end function
function ::GetBlueComponent(image,color)
  ::GetBlueComponent = ::GetColorComponent(image,color,4)
end function

sub ::Circle(image,x,y,r,color)
  ::Arc image,x,y,r,r,0,360,color
end sub

sub ::Ellipse(image,x,y,w,h,color)
  ::Arc image,x,y,w,h,0,360,color
end sub

end module
