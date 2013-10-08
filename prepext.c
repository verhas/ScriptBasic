/*
FILE: prepext.c
HEADER: prepext.h

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

// this is the Internal Preprocessor interface version
#define IP_INTERFACE_VERSION 1

typedef struct _Prepext {
  long lVersion;
  void *pPointer;
  void *pMemorySegment;
  struct _SupportTable *pST;
  } Prepext, *pPrepext;

enum PreprocessorCommands {
// constant values that tell the preprocessor the actual action that the
// preprocessor is called for
  PreprocessorLoad = 0,
  PreprocessorReadStart,
  PreprocessorReadDone0,
  PreprocessorReadDone1,
  PreprocessorReadDone2,
  PreprocessorReadDone3,

  PreprocessorLexInit,
  PreprocessorLexDone,
  PreprocessorLexNASymbol,
  PreprocessorLexASymbol,
  PreprocessorLexSymbol,
  PreprocessorLexString,
  PreprocessorLexMString,
  PreprocessorLexInteger,
  PreprocessorLexReal,
  PreprocessorLexCharacter,

  PreprocessorExStart,
  PreprocessorExStartLine,
  PreprocessorExEnd,
  PreprocessorExFinish,
  PreprocessorExStartLocal,
  PreprocessorExEndLocal,
  PreprocessorExLineNode,

  PreprocessorExeStart,
  PreprocessorExeFinish,
  PreprocessorExeNoRun,

// constant values that the preprocessor can pass back to the calling
// level to tell what to do next
  PreprocessorContinue, // go on call the next available preprocessor
  PreprocessorDone, // this preprocessor has done what had to be done at this level, do not call further preprocessors
  PreprocessorUnload,


  _PreprocesorDummy_
  };

*/
