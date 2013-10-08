/* 
FILE:   uniqfnam.c
HEADER: uniqfnam.h

--GNU LGPL
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

TO_HEADER:

#define UNIQ_FILE_NAME_LENGTH 32

*/


/*POD
=H Creating unique file name
=abstract
The function in this file is used to create a unique file name for
internal storage. This is used in code cache and preprocessing
operations.

=toc

CUT*/

#include <string.h>

#include "tools/global.h"
#include "tools/md5.h"

/*POD
=section uniqfnam
=H Calculate unique file name

The input file name should be the name of a file including the full path
to the file name. The function calculates the MD5 digest of the file name,
which is a 16-byte number and converts it to ASCII and copies the result to
the output argument T<pszOutputFileName>. The argument T<pszOutputFileName>
should point to a buffer of at least 33 characters (32 characters plus the
ZCHAR).

/*FUNCTION*/
void uniqfnam(char *pszInputFileName,
              char *pszOutputFileName
  ){
/*noverbatim
CUT*/
  MD5_CTX MyContext;
  unsigned char digest[16];
  int i;

  /* calculate the MD5 digest of the file name */
  MD5Init(&MyContext);
  MD5Update(&MyContext, pszInputFileName, strlen(pszInputFileName));
  MD5Final(digest,&MyContext);

  /* convert the digest to ASCII */
  for( i = 0 ; i < 16 ; i++ ){
    pszOutputFileName[2*i] = 'A' + (digest[i]&0x0F);
    digest[i] >>= 4;
    pszOutputFileName[2*i+1] = 'A' + (digest[i]&0x0F);
    }
  pszOutputFileName[32] = (char)0;
  }
