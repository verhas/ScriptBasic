import gd.bas

brush = gd::Create(10,10)
white = gd::Color(brush,255,255,255)
black = gd::Color(brush,0,240,0)
gd::Line brush,0,0,10,10,black
gd::Line brush,0,10,10,0,black

' gd::SavePng brush,"brush.png"

image = gd::Create(400,300)

white = gd::Color(image,255,255,255)
gd::SetTransparentColor image,white
black = gd::Color(image,0,0,0)
red =  gd::Color(image,255,0,0)
blue =  gd::Color(image,0,0,255)
green = gd::Color(image,0,255,0)

gd::Point image,0,0,black

gd::Rectangle image,200,50,250,100,red
gd::FilledRectangle image,225,75,275,125,green

gd::Rectangle image,324,190,376,290,black
gd::SetTile image,brush
' caused stack overflow on a fine NT? Should be some poor implementation
'gd::FillToBorder image,325,191,black,gd::Tiled

gd::Circle image,350,50,40,blue
gd::FillToBorder image,350,50,blue,green
gd::Fill image,201,51,blue

gd::SetBrush image,brush
gd::Line image,300,200,300,350,gd::Brushed

gd::SetColor image,black

gd::SetFont image,gd::FontTiny
gd::print image,0,0,"THIS PICTURE WAS CREATED FROM ScriptBasic USING THE MODULE GD/PNG"
gd::print image,0,10,"x=",gd::SizeX(image)," y=",gd::SizeY(image)
gd::print image,100,100,"Tiny ",12*3+55

gd::SetFont image,gd::FontSmall
gd::print image,100,120,"Small ",55*63

gd::SetFont image,gd::FontMedium
gd::print image,100,150,"Medium ",24/19

gd::SetFont image,gd::FontLarge
gd::print image,100,190,"Large ",sin(3.1)

gd::SetFont image,gd::FontGiant
gd::print image,100,240,"Giant ",log(1000)


for i=0 to 65 step 5
  gd::Line image,i,20,65-i,75
next i

LineStyle[0] = black
LineStyle[1] = black
LineStyle[2] = undef
LineStyle[3] = undef
LineStyle[4] = red
LineStyle[5] = green
LineStyle[6] = blue
LineStyle[7] = undef
LineStyle[8] = red
LineStyle[9] = red

gd::LineStyle image,LineStyle

gd::Line image,0,90,100,90,undef

for i=0 to 65 step 5
  gd::Line image,i,100,65-i,165,undef
next i


ImagePng = gd::Png(image)

gd::Destroy image

fn = 0
open "test.png" for output as fn
binmode fn
print#fn,ImagePng
undef ImagePng
close#fn

print "donez\n"
