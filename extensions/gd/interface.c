/* FILE: gdinterf.c

This file is a ScriptBasic interface file to the library gd.

NTLIBS: libpng.lib gd.lib zlibs.lib
UXLIBS: -lc -lpng -lz -lgd
*/
#include <stdio.h>
#include "../../basext.h"

#include "gd.h"
#include "gdfontt.h"
#include "gdfonts.h"
#include "gdfontmb.h"
#include "gdfontl.h"
#include "gdfontg.h"

#define MAXBFONT 5
#define DEFAULTBFONT 3
#define DEFAULTPTSIZE 12.0
#define ANGLE_UP 270

#define GD_ERROR_ARGUMENT 0x00080001
#define GD_ERROR_FILE     0x00080002
#define GD_ERROR_ERROR    0x00080003
#define GD_ERROR_POLYAS   0x00080004
#define GD_ERROR_POLYAS1  0x00080005

/* the created images are stored in a list and any existing image
   is automatically destroyed when the module exists. */
typedef struct _ImageList {
  struct _ImageList *prev,*next;
  gdImagePtr im;
  char *TTFfont;
  int BinFont;
  int color;
  int angle;
  double ptsize;
  } ImageList, *pImageList;

typedef struct _GdClass {
  pImageList pImages;
  } GdClass, *pGdClass;


/* Inserts the image into the list of images. */
#define InsertImage(x) __InsertImage(pSt,ppModuleInternal,(x))
static pImageList __InsertImage(pSupportTable pSt,
                          void **ppModuleInternal,
                          gdImagePtr pImage){
  pGdClass p;
  pImageList q;

  p = (pGdClass)besMODULEPOINTER;

  q = besALLOC(sizeof(ImageList));
  if( q == NULL )return NULL;

  q->next = p->pImages;
  q->prev = NULL;
  q->im = pImage;
  if( q->next )
    q->next->prev = q;
  p->pImages = q;

  q->TTFfont = NULL;
  q->BinFont = DEFAULTBFONT; /* the default font is medium bold */
  q->color = 0; /* the default is the background color */
  q->angle = 0;
  q->ptsize = DEFAULTPTSIZE;
  return q;  
  }

/* Removes the list element. The image itself is NOT destroyed! */
#define RemoveImage(x) __RemoveImage(pSt,ppModuleInternal,(x))
static void __RemoveImage(pSupportTable pSt,
                          void **ppModuleInternal,
                          pImageList q){
  pGdClass p;

  p = (pGdClass)besMODULEPOINTER;

  if( q->prev )
    q->prev->next = q->next;
  else
    p->pImages = q->next;

  if( q->next )
    q->next->prev = q->prev;

  besFREE(q);
  }

besVERSION_NEGOTIATE

  return (int)INTERFACE_VERSION;

besEND

besSUB_START
  pGdClass p;

  besMODULEPOINTER = besALLOC(sizeof(GdClass));
  if( besMODULEPOINTER == NULL )return 0;
  p = (pGdClass)besMODULEPOINTER;
  p->pImages = NULL;

besEND

besSUB_FINISH
  pGdClass p;
  pImageList q,k;
  p = (pGdClass)besMODULEPOINTER;

  q = p->pImages;
  while( q ){
    gdImageDestroy(q->im);
    q = (k=q)->next;
    besFREE(k);
    }

  besFREE(p);
besEND

/* get mandatory long argument */
#define GET_M_L_ARGUMENT(x,y)  Argument = besARGUMENT(y);\
  besDEREFERENCE(Argument);\
  if( Argument == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;\
  Argument = besCONVERT2LONG(Argument);\
  x = (int)LONGVALUE(Argument);

#define GET_COLOR_ARGUMENT(y)  \
  if( besARGNR < y ){ color = p->color; }else{\
    Argument = besARGUMENT(y);\
    besDEREFERENCE(Argument);\
    if( Argument == NULL ) color = p->color; else{\
    Argument = besCONVERT2LONG(Argument);\
    color = (int)LONGVALUE(Argument);}}

/* get mandatory pointer argument */
#define GET_M_P_ARGUMENT(x,y)  Argument = besARGUMENT(y);\
  besDEREFERENCE(Argument);\
  if( Argument == NULL )return COMMAND_ERROR_ARGUMENT_RANGE;\
  if( TYPE(Argument) != VTYPE_STRING ||\
      STRLEN(Argument) != sizeof(x) )return COMMAND_ERROR_ARGUMENT_TYPE;\
  memcpy(&(x),STRINGVALUE(Argument),sizeof(x));

/*gdImageCreateFromPng(FILE *in) */
besFUNCTION(gdicfpng)
  VARIABLE Argument;
  gdImagePtr pIm;
  pImageList p;
  char *pszFileName;
  int FileAccess;
  FILE *fp;

  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return COMMAND_ERROR_FEW_ARGS;

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return GD_ERROR_ARGUMENT;
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszFileName);

  FileAccess = besHOOK_FILE_ACCESS(pszFileName);
  if( !(FileAccess&1) ){
    besFREE(pszFileName);
    return GD_ERROR_FILE;
    }

  fp = fopen(pszFileName,"rb");
  besFREE(pszFileName);
  if( fp == NULL )return GD_ERROR_FILE;
  pIm = gdImageCreateFromPng(fp);
  fclose(fp);

  if( pIm == NULL )return COMMAND_ERROR_MEMORY_LOW;

  p = InsertImage(pIm);

  if( p == NULL ){
    gdImageDestroy(pIm);
    return COMMAND_ERROR_MEMORY_LOW;
    }
  besALLOC_RETURN_STRING(sizeof(p));
  memcpy(STRINGVALUE(besRETURNVALUE),&p,sizeof(p));
besEND

besFUNCTION(getxdime)
  VARIABLE Argument;
  pImageList p;

  if( besARGNR < 1 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)

  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = gdImageSX(p->im);
besEND

besFUNCTION(getydime)
  VARIABLE Argument;
  pImageList p;

  if( besARGNR < 1 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)

  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = gdImageSY(p->im);
besEND

/* gdImageCreate */
besFUNCTION(gdic)
  VARIABLE Argument;
  int sx,sy;
  gdImagePtr pIm;
  pImageList p;

  besRETURNVALUE = NULL;

  if( besARGNR < 2 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_L_ARGUMENT(sx,1)
  GET_M_L_ARGUMENT(sy,2)

  pIm = gdImageCreate(sx,sy);
  if( pIm == NULL )return COMMAND_ERROR_MEMORY_LOW;

  p = InsertImage(pIm);

  if( p == NULL ){
    gdImageDestroy(pIm);
    return COMMAND_ERROR_MEMORY_LOW;
    }
  besALLOC_RETURN_STRING(sizeof(p));
  memcpy(STRINGVALUE(besRETURNVALUE),&p,sizeof(p));
besEND

/* gdImageDestroy(im) */
besFUNCTION(imdestr)
  VARIABLE Argument;
  pImageList p;

  if( besARGNR < 1 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)

  gdImageDestroy(p->im);
  RemoveImage(p);
  
besEND

/* gdAllocateColor(im,r,g,b) */
besFUNCTION(gdac)
  VARIABLE Argument;
  int r,g,b,color;
  pImageList p;

  besRETURNVALUE = NULL;

  if( besARGNR < 4 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)
  GET_M_L_ARGUMENT(r,2)
  GET_M_L_ARGUMENT(g,3)
  GET_M_L_ARGUMENT(b,4)

  color = gdImageColorAllocate(p->im,r,g,b);

  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = (long)color;
besEND

/* gdImageLine(im, 0, 0, 63, 63, white) */
besFUNCTION(gdline)
  VARIABLE Argument;
  int xs,ys,xe,ye,color;
  pImageList p;

  besRETURNVALUE = NULL;

  /* color is optional */
  if( besARGNR < 5 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)
  GET_M_L_ARGUMENT(xs,2)
  GET_M_L_ARGUMENT(ys,3)
  GET_M_L_ARGUMENT(xe,4)
  GET_M_L_ARGUMENT(ye,5)

  if( besARGNR == 5 ){/* color is missing */
    color = p->color;
    }else{
    Argument = besARGUMENT(6);
    besDEREFERENCE(Argument);
    if( Argument == NULL )/* color is an explicit undef */
      color = gdStyled;
    else{
      Argument = besCONVERT2LONG(Argument);
      color = (int)LONGVALUE(Argument);
      }
    }
  gdImageLine(p->im,xs,ys,xe,ye,color);
besEND

/* gdImageRectangle(im, 0, 0, 63, 63, white)
   gdImageFilledRectangle(im, 0, 0, 63, 63, white)
*/
besFUNCTION(gdrect)
  VARIABLE Argument;
  int xs,ys,xe,ye,color;
  pImageList p;
  long fill;

  besRETURNVALUE = NULL;

  if( besARGNR < 7 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)
  GET_M_L_ARGUMENT(xs,2)
  GET_M_L_ARGUMENT(ys,3)
  GET_M_L_ARGUMENT(xe,4)
  GET_M_L_ARGUMENT(ye,5)
  GET_COLOR_ARGUMENT(6)
  GET_M_L_ARGUMENT(fill,7)

  if( fill )
    gdImageFilledRectangle(p->im,xs,ys,xe,ye,color);
  else
    gdImageRectangle(p->im,xs,ys,xe,ye,color);
besEND

besFUNCTION(impng)
  VARIABLE Argument;
  pImageList p;
  FILE *fp;
  char *pszFileName;
  int FileAccess;

  if( besARGNR < 2 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)

  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return GD_ERROR_ARGUMENT;
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszFileName);
  FileAccess = besHOOK_FILE_ACCESS(pszFileName);
  if( !(FileAccess&2) ){
    besFREE(pszFileName);
    return GD_ERROR_FILE;
    }

  /* We can not use the hook functions here. Why?
     I do not really know why, but it seems that the main part of the
     process and the DLL uses different allocation heap on Windows NT
     and it also affects the file handling. I tried to open a file
     using the hook function and with fopen. They returned different
     file handles, and a fgetc, fread, fwrite or similar function could not
     cope with a file handle pointer opened via the hook function. Somehow
     they even destroyed the file pointer and caused core dump when file
     closing took place (via hook_fclose).                                 */
  fp = fopen(pszFileName,"wb");
  besFREE(pszFileName);
  if( fp == NULL )return GD_ERROR_FILE;
  gdImagePng(p->im,fp);
  fclose(fp);
besEND

/*void* gdImagePngPtr(gdImagePtr im, int *size)*/
besFUNCTION(impngstr)
  VARIABLE Argument;
  int size;
  void *q;
  pImageList p;

  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)
  q = gdImagePngPtr(p->im,&size);
  if( size <= 0 )return GD_ERROR_ERROR;
  besALLOC_RETURN_STRING((long)size);
  memcpy(STRINGVALUE(besRETURNVALUE),q,size);
  free(q);
besEND

/* gdImageSetPixel(im, 0, 0, white) */
besFUNCTION(gdpix)
  VARIABLE Argument;
  int x,y,color;
  pImageList p;

  besRETURNVALUE = NULL;

  if( besARGNR < 3 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)
  GET_M_L_ARGUMENT(x,2)
  GET_M_L_ARGUMENT(y,3)
  GET_COLOR_ARGUMENT(4)

  gdImageSetPixel(p->im,x,y,color);
besEND

/*

gd::Polygon(Image,Xarray,Yarray,Color)

*/
besFUNCTION(gdpoly)
  VARIABLE Argument;
  VARIABLE vX,vY;
  VARIABLE V;
  int ArraySize,color;
  long i,j;
  pImageList p;
  gdPoint *point;
  long fill;

  besRETURNVALUE = NULL;

  if( besARGNR < 5 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)

  vX = besARGUMENT(2);
  besDEREFERENCE(vX);
  if( TYPE(vX) != VTYPE_ARRAY )return COMMAND_ERROR_ARGUMENT_TYPE;
  ArraySize = ARRAYHIGH(vX) - ARRAYLOW(vX) +1;

  vY = besARGUMENT(3);
  besDEREFERENCE(vY);
  if( TYPE(vY) != VTYPE_ARRAY )return COMMAND_ERROR_ARGUMENT_TYPE;
  if( ArraySize != ARRAYHIGH(vY) - ARRAYLOW(vY) +1 )
    return GD_ERROR_POLYAS;

  if( ARRAYLOW(vX) != ARRAYLOW(vY) )return GD_ERROR_POLYAS1;

  point = besALLOC(ArraySize*sizeof(gdPoint));
  if( point == NULL )return COMMAND_ERROR_MEMORY_LOW;

  for( i=0 , j = ARRAYLOW(vX) ; i < ArraySize ; i++ , j++ ){
    V = ARRAYVALUE(vX,j);
    if( V == NULL ){
      besFREE(point);
      return COMMAND_ERROR_ARGUMENT_TYPE;
      }
    point[i].x = LONGVALUE(besCONVERT2LONG(V));
    V = ARRAYVALUE(vY,j);
    if( V == NULL ){
      besFREE(point);
      return COMMAND_ERROR_ARGUMENT_TYPE;
      }
    point[i].y = LONGVALUE(besCONVERT2LONG(V));
    }

  GET_COLOR_ARGUMENT(4)
  GET_M_L_ARGUMENT(fill,5)

  if( fill )
    gdImageFilledPolygon(p->im,point,ArraySize,color);
  else
    gdImagePolygon(p->im,point,ArraySize,color);
  besFREE(point);
besEND

/*
void gdImageArc(gdImagePtr im, int cx, int cy, int w, int h, int s, int e, int color)
*/
besFUNCTION(gdarc)
  VARIABLE Argument;
  long cx,cy,w,h,s,e,color;
  pImageList p;

  besRETURNVALUE = NULL;

  if( besARGNR < 7 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)
  GET_M_L_ARGUMENT(cx,2)
  GET_M_L_ARGUMENT(cy,3)
  GET_M_L_ARGUMENT(w,4)
  GET_M_L_ARGUMENT(h,5)
  GET_M_L_ARGUMENT(s,6)
  GET_M_L_ARGUMENT(e,7)
  GET_COLOR_ARGUMENT(8)

  gdImageArc(p->im, (int) cx, (int) cy, (int) w, (int) h, (int) s, (int) e, (int) color);

besEND

/*
void gdImageFillToBorder(gdImagePtr im, int x, int y, int border, int color) 
*/
besFUNCTION(gdfilltb)
  VARIABLE Argument;
  long x,y,border,color;
  pImageList p;

  besRETURNVALUE = NULL;

  if( besARGNR < 4 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)
  GET_M_L_ARGUMENT(x,2)
  GET_M_L_ARGUMENT(y,3)
  GET_M_L_ARGUMENT(border,4)
  GET_COLOR_ARGUMENT(5)

  gdImageFillToBorder(p->im, (int) x, (int) y, (int) border, (int) color);

besEND

/*
void gdImageFill(gdImagePtr im, int x, int y, int color)
*/
besFUNCTION(gdfill)
  VARIABLE Argument;
  long x,y,color;
  pImageList p;

  besRETURNVALUE = NULL;

  if( besARGNR < 3 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)
  GET_M_L_ARGUMENT(x,2)
  GET_M_L_ARGUMENT(y,3)
  GET_COLOR_ARGUMENT(4)

  gdImageFill(p->im, (int) x, (int) y, (int) color);

besEND

/*
void gdImageSetBrush(gdImagePtr im, gdImagePtr brush)
*/
besFUNCTION(gdstbrus)
  VARIABLE Argument;
  pImageList p,q;

  besRETURNVALUE = NULL;

  if( besARGNR < 2 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)
  GET_M_P_ARGUMENT(q,2)
  if( p == q )return COMMAND_ERROR_ARGUMENT_RANGE;

  gdImageSetBrush(p->im, q->im );

besEND

/*
void gdImageSetTile(gdImagePtr im, gdImagePtr tile)
*/
besFUNCTION(gdstile)
  VARIABLE Argument;
  pImageList p,q;

  besRETURNVALUE = NULL;

  if( besARGNR < 2 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)
  GET_M_P_ARGUMENT(q,2)

  gdImageSetTile(p->im, q->im );

besEND

/*
void gdImageSetStyle(gdImagePtr im, int *style, int styleLength) 
*/
besFUNCTION(gdsstyle)
  VARIABLE Argument;
  VARIABLE vSTYLE,V;
  int ArraySize;
  long i,j;
  pImageList p;
  int *style;

  besRETURNVALUE = NULL;

  if( besARGNR < 2 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)

  vSTYLE = besARGUMENT(2);
  besDEREFERENCE(vSTYLE);
  if( TYPE(vSTYLE) != VTYPE_ARRAY )return COMMAND_ERROR_ARGUMENT_TYPE;
  ArraySize = ARRAYHIGH(vSTYLE) - ARRAYLOW(vSTYLE) +1;

  style = besALLOC(ArraySize*sizeof(int));
  if( style == NULL )return COMMAND_ERROR_MEMORY_LOW;

  for( i=0 , j = ARRAYLOW(vSTYLE) ; i < ArraySize ; i++ , j++ ){
    V = ARRAYVALUE(vSTYLE,j);
    if( V == NULL )
      style[i] = gdTransparent;
    else
      style[i] = (int)LONGVALUE(besCONVERT2LONG(V));
    }
  gdImageSetStyle(p->im,style,ArraySize);
  besFREE(style);
besEND

/*
int gdImage/Blue/Green/Red(gdImagePtr im, int color) 
gd::GetColorComponent(image,color,gd::Red/gd::Green/gd::Blue)
*/
besFUNCTION(gdgetc)
  VARIABLE Argument;
  long color,component;
  pImageList p;

  besRETURNVALUE = NULL;

  if( besARGNR < 3 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)
  GET_M_L_ARGUMENT(color,2)
  GET_M_L_ARGUMENT(component,3);

  switch( component ){
    default: return COMMAND_ERROR_ARGUMENT_RANGE;
    case 1: color = gdImageRed(p->im,color);
            break;
    case 2: color = gdImageGreen(p->im,color);
            break;
    case 4: color = gdImageBlue(p->im,color);
            break;
    }

  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = color;

besEND

/*
int gdImageBoundsSafe(gdImagePtr im, int x, int y) 
*/
besFUNCTION(gdbons)
  VARIABLE Argument;
  long x,y;
  pImageList p;

  besRETURNVALUE = NULL;

  if( besARGNR < 3 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)
  GET_M_L_ARGUMENT(x,2)
  GET_M_L_ARGUMENT(y,3);

  besALLOC_RETURN_LONG;
  LONGVALUE(besRETURNVALUE) = gdImageBoundsSafe(p->im,(int)x,(int)y);

besEND

/* set the color for subsequent calls when color is not defined */
besFUNCTION(gdcolor)
  VARIABLE Argument;
  long color;
  pImageList p;

  besRETURNVALUE = NULL;

  if( besARGNR < 2 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)
  GET_M_L_ARGUMENT(color,2)

  p->color = color;
besEND


/* set the font to be used in subsequent gd::print commands */
besFUNCTION(gdfont)
  VARIABLE Argument;
  pImageList p;
  char *s;
  long slen;
  int IsTTF;

  besRETURNVALUE = NULL;

  if( besARGNR < 2 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)

  if( p->TTFfont )besFREE(p->TTFfont);
  p->TTFfont = NULL;
  p->BinFont = DEFAULTBFONT;
  p->angle = 0;
  p->ptsize = DEFAULTPTSIZE;

  Argument = besARGUMENT(2);
  besDEREFERENCE(Argument);
  if( Argument ){/* explicit undef sets to default font */
    if( IsTTF=(TYPE(Argument) == VTYPE_STRING) ){
      /* this is TrueType font */
      slen = STRLEN(Argument); /* this is a file name, we better check that there is no null char in it */
      s = STRINGVALUE(Argument);
      while( slen --)if( *s++ == (char)0 )return COMMAND_ERROR_ARGUMENT_RANGE;
      besCONVERT2ZCHAR(Argument,p->TTFfont);
      }else{
      Argument = besCONVERT2LONG(Argument);
      p->BinFont = LONGVALUE(Argument);
      if( p->BinFont < 1 || p->BinFont > MAXBFONT )p->BinFont = DEFAULTBFONT;
      }
    }

  if( besARGNR < 3 )return COMMAND_ERROR_SUCCESS;
  Argument = besARGUMENT(3);
  besDEREFERENCE(Argument);
  if( Argument != NULL ){/* explicit undef makes it to default 0 */
    Argument = besCONVERT2LONG(Argument);
    p->angle = LONGVALUE(Argument);
    if( ! IsTTF ){
      if( p->angle != 0 && p->angle != ANGLE_UP ){
        p->angle = 0;
        return COMMAND_ERROR_ARGUMENT_RANGE;
        }
      }
    }

  if( ! IsTTF || besARGNR < 4 )return COMMAND_ERROR_SUCCESS; /* size is ignored in case of built in functions */
  Argument = besARGUMENT(4); /* size of a TTF font */
  besDEREFERENCE(Argument);
  if( Argument == NULL )return COMMAND_ERROR_SUCCESS;
  Argument = besCONVERT2DOUBLE(Argument);
  p->ptsize = DOUBLEVALUE(Argument);    

besEND

/*
void gdImageString[Up](gdImagePtr im, gdFontPtr font, int x, int y, unsigned char *s, int color)
char *gdImageStringTTF(gdImagePtr im, int *brect, int fg, char *fontname, double ptsize, double angle, int x, int y, char *string) 

gd::print image,x,y,"text to print"
*/
besFUNCTION(gdprint)
  VARIABLE Argument;
  VARIABLE *pV;
  long x,y;
  pImageList p;
  int argI;
  char *s,*r;
  gdFontPtr font;
  int brect[8];
  unsigned long slen;

  besRETURNVALUE = NULL;

  if( besARGNR < 3 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)
  GET_M_L_ARGUMENT(x,2)
  GET_M_L_ARGUMENT(y,3);

  pV = besALLOC((besARGNR-3)*sizeof(VARIABLE));
  if( pV == NULL )return COMMAND_ERROR_MEMORY_LOW;
  pV -= 4;
  for( slen = 0, argI = 4 ; argI <= besARGNR ; argI++ ){
    Argument = besARGUMENT(argI);
    besDEREFERENCE(Argument);
    pV[argI] = besCONVERT2STRING(Argument); /* note that undef is converted to null string */
    slen += STRLEN(pV[argI]);
    }
  s = besALLOC(slen+1);
  if( s == NULL ){
    pV += 4;
    besFREE(pV);
    return COMMAND_ERROR_MEMORY_LOW;
    }
  for( r = s , argI = 4 ; argI <= besARGNR ; argI++ ){
    for( slen = 0 ; STRINGVALUE(pV[argI])[slen] && slen < STRLEN(pV[argI]) ; slen++ )
      *r++ = STRINGVALUE(pV[argI])[slen];
    }
  *r = (char)0;
  pV += 4;
  besFREE(pV);

  if( p->TTFfont ){
    r = gdImageStringTTF(p->im, brect, p->color, p->TTFfont, p->ptsize,
                          (double)p->angle, (int) x, (int) y, s) ;
    if( r ){
      besFREE(s);
      return GD_ERROR_ERROR;
      }
    }else{
    font = gdFontMediumBold;
    switch( p->BinFont ){
      case 1: font = gdFontTiny; break;
      case 2: font = gdFontSmall; break;
      case 3: font = gdFontMediumBold; break;
      case 4: font = gdFontLarge; break;
      case 5: font = gdFontGiant; break;
      }
    if( p->angle == ANGLE_UP )
      gdImageStringUp(p->im,font, (int) x, (int) y, s, p->color);
    else
      gdImageString(p->im,font, (int) x, (int) y, s, p->color);
    }
  besFREE(s);

besEND

/*
void gdImageColorTransparent(gdImagePtr im, int color)
*/
besFUNCTION(gdtrnspa)
  VARIABLE Argument;
  long c;
  pImageList p;

  besRETURNVALUE = NULL;

  if( besARGNR < 2 )return COMMAND_ERROR_FEW_ARGS;

  GET_M_P_ARGUMENT(p,1)
  GET_M_L_ARGUMENT(c,2)

  gdImageColorTransparent(p->im,(int)c);
besEND
