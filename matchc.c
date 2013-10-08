/*
FILE: matchc.c
HEADER: matchc.h

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

This file contains declarations that are used by more than one command file in the
directory commands.

The pattern matching operator "LIKE" implemented in string.c uses an own data structure
and the directory listing "OPENDIR" implemented in file.c also need access to the same
data structure.

TO_HEADER:
typedef struct _PatternParam {
  unsigned long cArraySize;// allocated array size
  unsigned long cAArraySize;//actual array size (that is used actuall, should not be greater than cArraySize)
  unsigned long *pcbParameterArray;
  char **ParameterArray;
  unsigned long cbBufferSize;
  char *pszBuffer;
  int iMatches;
  pMatchSets pThisMatchSets;
  } PatternParam, *pPatternParam;
int initialize_like(pExecuteObject pEo);
*/
