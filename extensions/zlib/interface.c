/* FILE: zlbintrf.c

This file is a ScriptBasic interface file to the library zlib.


NTLIBS: zlibs.lib
UXLIBS: -lc -lz
DWLIBS: -lc -lz
*/
#include <stdio.h>
#include "../../basext.h"

#include "zlib.h"

#define ZLIB_ERROR_INTERNAL001 0x00080001
#define ZLIB_ERROR_INTERNAL002 0x00080002
#define ZLIB_ERROR_INTERNAL003 0x00080003
#define ZLIB_ERROR_INTERNAL004 0x00080004
#define ZLIB_ERROR_INTERNAL005 0x00080005
#define ZLIB_ERROR_INTERNAL006 0x00080006
#define ZLIB_ERROR_INTERNAL007 0x00080007

#define ZLIB_ERROR_NOCOMPRESS  0x00080100
#define ZLIB_ERROR_ARGUMENT    0x00080101
#define ZLIB_ERROR_DATA        0x00080102
#define ZLIB_ERROR_DATA1       0x00080103
#define ZLIB_ERROR_FILE_READ   0x00080104
#define ZLIB_ERROR_FILE_WRITE  0x00080105

/* This is the buffer increment size in case no data was inflated
   in the first run or in case estimate is too small (smaller than this
   value).                                                              */
#define ZBUFFER_INCREASE 1024


/* This is the file buffer size when converting a file to gz and back.  */
#define ZBUFFER_SIZE 1024

besVERSION_NEGOTIATE

  return (int)INTERFACE_VERSION;

besEND

besSUB_START

besEND

besSUB_FINISH

besEND

/* functions that are passed to the zlib library to allocate memory */
static void *zliballoc_interface(void *opaque, uInt items, uInt size){
  pSupportTable pSt;
  void *pvReturn;

  pSt = opaque;
  pvReturn = besALLOC((long)items*size);
  if( pvReturn )return pvReturn;
  return Z_NULL;
  }
static void zlibfree_interface(void *opaque, voidpf address){
  pSupportTable pSt;

  pSt = opaque;
  besFREE(address);
  }

/**
=H Zlib functions

To use these function the program has to import the file T<zlib.bas>

=verbatim
import zlib.bas
=noverbatim
--------------------------------------

The functions in this module are:

*/

/**
=section compress
=H Compress a string
=verbatim
CompressedString = zlib::Compress(UncompressedString)
=noverbatim
This function implements a simple compression interface.
*/
besFUNCTION(zlbcmprs)
  VARIABLE Argument;
  z_stream zsStream;
  int iCompressionLevel;
  unsigned char *pszBuffer;
  int iZError;

  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  zsStream.data_type = Z_UNKNOWN;
  zsStream.opaque = pSt;
  zsStream.zalloc = zliballoc_interface;
  zsStream.zfree  = zlibfree_interface;

  /* Get the compression level to be used from the option

     option zlib$CompressionLevel x

     If this is not set then the default compression level
     is 6 (zlib 5).

     Note that the BASIC program should use compression level
     1 ... 10. This is converted to 0 ... 9. This is because
     option value zero is the default in ScriptBasic and there is
     no way to guess if the option is set to zero or is not present.

     Therefore the BASIC code should not bother compression level and
     use the default level 6 (zlib 5) or should set it between 1 and 10.

     If the compression level is higher than 10 then it is set to 10 (zlib level 9).
     If the compression level is negative it is set to 1 (zlib level 0).
  */

  iCompressionLevel = (int)besOPTION("zlib$CompressionLevel");
  if( iCompressionLevel > 10 )iCompressionLevel = 10;
  if( iCompressionLevel < 0 )iCompressionLevel = 1;
  if( iCompressionLevel == 0 )
    iCompressionLevel = Z_DEFAULT_COMPRESSION;
  else
    iCompressionLevel --;

  /* get the string to be compressed */

  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  /* if the argument is undefined we raise error */
  if( Argument == NULL )return ZLIB_ERROR_ARGUMENT;
  if( Argument->vType != VTYPE_STRING )
    Argument = besCONVERT2STRING(Argument);

  pszBuffer = besALLOC(STRLEN(Argument));
  if( pszBuffer == NULL )return COMMAND_ERROR_MEMORY_LOW;

  zsStream.next_in = STRINGVALUE(Argument);
  zsStream.avail_in = STRLEN(Argument);
  zsStream.total_in = STRLEN(Argument);
  zsStream.avail_out = STRLEN(Argument);
  zsStream.next_out = pszBuffer;
  zsStream.total_out = 0;

  iZError = deflateInit(&zsStream,iCompressionLevel);
  if( iZError == Z_MEM_ERROR )return COMMAND_ERROR_MEMORY_LOW;
  /* this should not happen, because we have checked the compression level value */
  if( iZError == Z_STREAM_ERROR )return ZLIB_ERROR_INTERNAL001;
  /* this happens if the module was built using an incorrect version of the static lib file */
  if( iZError == Z_VERSION_ERROR )return ZLIB_ERROR_INTERNAL002;

  iZError = deflate(&zsStream,Z_FINISH);

  if( iZError != Z_STREAM_END )return ZLIB_ERROR_NOCOMPRESS;

  besALLOC_RETURN_STRING(zsStream.total_out);

  memcpy(STRINGVALUE(besRETURNVALUE),pszBuffer,zsStream.total_out);
  besFREE(pszBuffer);
  deflateEnd(&zsStream);
besEND

/**
=section uncompress
=H Uncompress a string
=verbatim
UncompressedString = zlib::UnCompress(CompressedString)
=noverbatim
This function implements a simple uncompression interface.
*/
besFUNCTION(zlbucprs)
  VARIABLE Argument;
  long lBufferSize,lCompressedSize,lNewBufferSize;
  z_stream zsStream;
  unsigned char *pszBuffer,*pszNewBuffer;
  int iZError;

  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  zsStream.data_type = Z_UNKNOWN;
  zsStream.opaque = pSt;
  zsStream.zalloc = zliballoc_interface;
  zsStream.zfree  = zlibfree_interface;

  /* get the string to be uncompressed */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);

  /* if the argument is undefined we raise error */
  if( Argument == NULL )return ZLIB_ERROR_ARGUMENT;

  /* the argument should be a string. This is not likely that a long
     or a double converted to string could be reasonably uncompressed. */
  if( Argument->vType != VTYPE_STRING )return ZLIB_ERROR_ARGUMENT;

  /* first we allocate a buffer that is as large as the compressed image. */
  lCompressedSize = lBufferSize = STRLEN(Argument);
  pszBuffer = besALLOC(lBufferSize);
  if( pszBuffer == NULL )return COMMAND_ERROR_MEMORY_LOW;

  zsStream.next_in = STRINGVALUE(Argument);
  zsStream.avail_in = STRLEN(Argument);
  zsStream.total_in = STRLEN(Argument);
  zsStream.avail_out = STRLEN(Argument);
  zsStream.next_out = pszBuffer;
  zsStream.total_out = 0;

  iZError = inflateInit(&zsStream);
  if( iZError == Z_MEM_ERROR )return COMMAND_ERROR_MEMORY_LOW;
  /* this should not happen, because we have checked the compression level value */
  if( iZError == Z_STREAM_ERROR )return ZLIB_ERROR_INTERNAL003;
  /* this happens if the module was built using an incorrect version of the static lib file */
  if( iZError == Z_VERSION_ERROR )return ZLIB_ERROR_INTERNAL004;

  /* inflate the input buffer to the output buffer and put as many
     output bytes to the output as possible */
  while( (iZError = inflate(&zsStream,Z_SYNC_FLUSH)) != Z_STREAM_END ){
    switch( iZError ){
      case Z_DATA_ERROR:   besFREE(pszBuffer);
                           return ZLIB_ERROR_DATA;
      case Z_NEED_DICT:    besFREE(pszBuffer);
                           return ZLIB_ERROR_DATA1;
      case Z_STREAM_ERROR: besFREE(pszBuffer);
                           return ZLIB_ERROR_INTERNAL005;
      case Z_MEM_ERROR:    besFREE( pszBuffer );
                           return COMMAND_ERROR_MEMORY_LOW;
      case Z_BUF_ERROR:    besFREE(pszBuffer);
                           return ZLIB_ERROR_INTERNAL006;
      }
    if( iZError != Z_OK ){
      besFREE(pszBuffer);
      return ZLIB_ERROR_INTERNAL007;
      }

    /* estimate the buffer size needed to hold the uncompressed data */
    if( lCompressedSize - zsStream.avail_in > 0 ){
      lNewBufferSize = (zsStream.total_out*lCompressedSize) / (lCompressedSize - zsStream.avail_in);
      /* We avoid extraordinarily large buffer allocation in case the uncompressed data starts
         with some redundant area and therefore starts inflating extremely and indicate huge
         final size. Never increase the buffer larger than the double of the current. */
      if( lNewBufferSize > 2 * lBufferSize )
        lNewBufferSize = 2 * lBufferSize;
      /* The estimate may be sensitive to rounding error when there are only a few bytes left. To
         avoid the situation when the final bytes are allocated individually we always increase the
         buffer at least ZBUFFER_INCREASE bytes. */
      if( lNewBufferSize < lBufferSize + ZBUFFER_INCREASE )
        lNewBufferSize = lBufferSize + ZBUFFER_INCREASE;
      }else{
      /* if the output buffer was so small that the decompressor just could not start then we have
         no estimate on the output buffer size. Just increase it adding the default increase to
         the size. */
      lNewBufferSize = lBufferSize + ZBUFFER_INCREASE;
      }

    /* Allocate the new buffer. */
    pszNewBuffer = besALLOC(lNewBufferSize);
    if( pszNewBuffer == NULL ){
      besFREE(pszBuffer);
      return COMMAND_ERROR_MEMORY_LOW;
      }

    /* copy the content of the old buffer to the new location and release the old buffer */
    memcpy(pszNewBuffer,pszBuffer,zsStream.total_out);
    besFREE(pszBuffer);

    pszBuffer = pszNewBuffer; /* from now on this is the new buffer */

    zsStream.next_out = pszBuffer + zsStream.total_out;
    zsStream.avail_out += lNewBufferSize - zsStream.total_out;
    lBufferSize = lNewBufferSize;
    }

  if( iZError != Z_STREAM_END )return ZLIB_ERROR_NOCOMPRESS;

  besALLOC_RETURN_STRING(zsStream.total_out);

  memcpy(STRINGVALUE(besRETURNVALUE),pszBuffer,zsStream.total_out);
  besFREE(pszBuffer);
  inflateEnd(&zsStream);
besEND

#define GZ_SUFFIX ".gz"
/**
=section gzip
=H Compress a file

=verbatim
zlib::gzip input_file [,output_file]
=noverbatim
This command compresses a file.
*/
besFUNCTION(gzipfunc)
  VARIABLE Argument;
  int FileAccess;
  char *pszInputFileName;
  char *pszOutputFileName;
  FILE *fin;
  gzFile out;
  char mode[4];
  size_t len;
  char *buf;
  int iCompressionLevel;
  int iRemoveOriginal;

  mode[0] = 'w';mode[1] = 'b';mode[2] = (char)0;

  iCompressionLevel = (int)besOPTION("zlib$CompressionLevel");
  if( iCompressionLevel > 10 )iCompressionLevel = 10;
  if( iCompressionLevel < 0 )iCompressionLevel = 1;
  if( iCompressionLevel > 0 ){
    mode[2] = '0' + iCompressionLevel;
    mode[3] = (char)0;
    }
  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  /* get the input file name */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return EX_ERROR_TOO_FEW_ARGUMENTS;
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszInputFileName);

  if( besARGNR >= 3 )
    Argument = besARGUMENT(3);
  else
    Argument = NULL;
  if( Argument ){
    besDEREFERENCE(Argument);
    Argument = besCONVERT2LONG(Argument);
    iRemoveOriginal = LONGVALUE(Argument);
    }else iRemoveOriginal = 1;

  if( besARGNR >= 2 ){
    Argument = besARGUMENT(2);
    besDEREFERENCE(Argument);
    }else Argument = NULL;

  if( Argument == NULL ){
    pszOutputFileName = besALLOC(strlen(pszInputFileName)+strlen(GZ_SUFFIX)+1);
    strcpy(pszOutputFileName,pszInputFileName);
    strcat(pszOutputFileName,GZ_SUFFIX);
    }else{
    /* get the output file name */
    Argument = besCONVERT2STRING(Argument);
    besCONVERT2ZCHAR(Argument,pszOutputFileName);
    }

  if( !strcmp(pszInputFileName,pszOutputFileName) ){
    besFREE(pszInputFileName);
    besFREE(pszOutputFileName);
    return EX_ERROR_TOO_FEW_ARGUMENTS;
    }

  FileAccess = besHOOK_FILE_ACCESS(pszOutputFileName);
  if( !(FileAccess&2) ){
    besFREE(pszInputFileName);
    besFREE(pszOutputFileName);
    return COMMAND_ERROR_FILE_CANNOT_BE_OPENED;
    }

  fin = besHOOK_FOPEN(pszInputFileName,"rb");
  if( fin == NULL ){
    besFREE(pszOutputFileName);
    return COMMAND_ERROR_FILE_CANNOT_BE_OPENED;
    }
  out = gzopen(pszOutputFileName,mode);
  besFREE(pszOutputFileName);
  if( out == NULL ){
    besFREE(pszInputFileName);
    besHOOK_FCLOSE(fin);
    return COMMAND_ERROR_FILE_CANNOT_BE_OPENED;
    }

  buf = besALLOC(ZBUFFER_SIZE);
  if( buf == NULL ){
    besFREE(pszInputFileName);
    besHOOK_FCLOSE(fin);
    gzclose(out);
    return COMMAND_ERROR_MEMORY_LOW;
    }
  for(;;){
    len = besHOOK_FREAD(buf, 1, ZBUFFER_SIZE, fin);
    if( besHOOK_FERROR(fin) ){
      besFREE(pszInputFileName);
      gzclose(out);
      besHOOK_FCLOSE(fin);
      return ZLIB_ERROR_FILE_READ;
      }
    if( len == 0 )break;
#pragma warning (disable:4018)
    if( gzwrite(out, buf, (unsigned)len) != len ){
#pragma warning (default:4018)
      besFREE(pszInputFileName);
      gzclose(out);
      besHOOK_FCLOSE(fin);
      return ZLIB_ERROR_FILE_WRITE;
      }
    }
  besFREE(buf);
  gzclose(out);
  besHOOK_FCLOSE(fin);

  /* the final step is to remove the original file */
  if( iRemoveOriginal )
    besHOOK_REMOVE(pszInputFileName);
  besFREE(pszInputFileName);
besEND

/**
=section gunzip
=H Uncompress a file

=verbatim
zlib::gunzip input_file [,output_file]
=noverbatim
This command uncompresses a file.
*/
besFUNCTION(gunzpfnc)
  VARIABLE Argument;
  int FileAccess;
  char *pszInputFileName;
  char *pszOutputFileName;
  FILE *out,*fp;
  gzFile fin;
  long len;
  char *buf;
  int iRemoveOriginal,ch,gzHeaderOK;
  static int gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */

  besRETURNVALUE = NULL;

  if( besARGNR < 1 )return EX_ERROR_TOO_FEW_ARGUMENTS;

  /* get the input file name */
  Argument = besARGUMENT(1);
  besDEREFERENCE(Argument);
  if( Argument == NULL )return EX_ERROR_TOO_FEW_ARGUMENTS;
  Argument = besCONVERT2STRING(Argument);
  besCONVERT2ZCHAR(Argument,pszInputFileName);

  if( besARGNR >= 3 )
    Argument = besARGUMENT(3);
  else
    Argument = NULL;
  if( Argument ){
    besDEREFERENCE(Argument);
    Argument = besCONVERT2LONG(Argument);
    iRemoveOriginal = LONGVALUE(Argument);
    }else iRemoveOriginal = 1;

  if( besARGNR >= 2 ){
    Argument = besARGUMENT(2);
    besDEREFERENCE(Argument);
    }else Argument = NULL;

  if( Argument == NULL ){
    len = strlen(pszInputFileName);
    pszOutputFileName = besALLOC(len+1);
    strcpy(pszOutputFileName,pszInputFileName);
    /* create the output file chopping off the .gz or .z extension */
    if( len > 2 &&
        tolower(pszOutputFileName[len-1]) == 'z' &&
        tolower(pszOutputFileName[len-2]) == 'g' &&
        pszOutputFileName[len-3] == '.' ){
        pszOutputFileName[len-3] = (char)0;
        }else
     if( len > 1 &&
        tolower(pszOutputFileName[len-1]) == 'z' &&
        pszOutputFileName[len-2] == '.' ){
        pszOutputFileName[len-2] = (char)0;
        }else{
        /* if there is only a single argument (input file name) and
           that argument string is not ending with .z or .gz then
           we can not guess the output file name. */
        besFREE(pszOutputFileName);
        besFREE(pszInputFileName);
        return EX_ERROR_TOO_FEW_ARGUMENTS;
        }
    }else{
    /* get the output file name */
    Argument = besCONVERT2STRING(Argument);
    besCONVERT2ZCHAR(Argument,pszOutputFileName);
    }

  /* the input and the output file names can not be the same
     we can not cope with the same file with different name issue however. */
  if( !strcmp(pszInputFileName,pszOutputFileName) ){
    besFREE(pszInputFileName);
    besFREE(pszOutputFileName);
    return EX_ERROR_TOO_FEW_ARGUMENTS;
    }

  FileAccess = besHOOK_FILE_ACCESS(pszInputFileName);
  if( !(FileAccess&1) ){
    besFREE(pszInputFileName);
    besFREE(pszOutputFileName);
    return COMMAND_ERROR_FILE_CANNOT_BE_OPENED;
    }

  gzHeaderOK = 1;
  fp = fopen(pszInputFileName,"rb");
  if( fp != NULL ){
    ch = fgetc(fp);
    if( ch != gz_magic[0] )gzHeaderOK = 0;
    ch = fgetc(fp);
    if( ch != gz_magic[1] )gzHeaderOK = 0;
    fclose(fp);
    }else gzHeaderOK = 0;

  fin = gzHeaderOK ? gzopen(pszInputFileName,"rb") : NULL;
  if( fin == NULL ){
    besFREE(pszOutputFileName);
    return COMMAND_ERROR_FILE_CANNOT_BE_OPENED;
    }
  out = besHOOK_FOPEN(pszOutputFileName,"wb");
  besFREE(pszOutputFileName);
  if( out == NULL ){
    besFREE(pszInputFileName);
    gzclose(fin);
    return COMMAND_ERROR_FILE_CANNOT_BE_OPENED;
    }

  buf = besALLOC(ZBUFFER_SIZE);
  if( buf == NULL ){
    besFREE(pszInputFileName);
    besHOOK_FCLOSE(out);
    gzclose(fin);
    return COMMAND_ERROR_MEMORY_LOW;
    }
  for(;;){
    len = gzread(fin,buf, ZBUFFER_SIZE);
    if( len < 0 ){
      besFREE(pszInputFileName);
      gzclose(fin);
      besHOOK_FCLOSE(out);
      return ZLIB_ERROR_FILE_READ;
      }
    if( len == 0 )break;
    if( besHOOK_FWRITE(buf,1,len,out) != len ){
      besFREE(pszInputFileName);
      gzclose(fin);
      besHOOK_FCLOSE(out);
      return ZLIB_ERROR_FILE_WRITE;
      }
    }
  besFREE(buf);
  gzclose(fin);
  besHOOK_FCLOSE(out);

  /* the final step is to remove the original file */
  if( iRemoveOriginal )
    besHOOK_REMOVE(pszInputFileName);
  besFREE(pszInputFileName);
besEND

