/* 
FILE:   filesys.c
HEADER: filesys.h

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

#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#ifndef _WIN32_WCE
#include <io.h>
#include <direct.h>
#include <lm.h>
#include <lmaccess.h>
#endif
#else
#ifdef _WIN32_WCE
#include <windows.h>
#include <winbase.h>
#include <io.h>
#include <direct.h>
#include <lm.h>
#include <lmaccess.h>
#else
#ifdef __MACOS__
#include <unistd.h>
#include <dirent.h>
#else
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#endif
#ifndef TYPE_SOCKET
typedef int SOCKET;
#endif
#endif
#endif

#ifndef LOCK_SH
#define LOCK_SH 1
#endif

#ifndef LOCK_EX
#define LOCK_EX 2
#endif

#ifndef LOCK_NB
#define LOCK_NB 4
#endif

#ifndef LOCK_UN
#define LOCK_UN 8
#endif


*/

#ifdef _WIN32_WCE
#else
#include <fcntl.h>
#ifndef __MACOS__
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <errno.h>
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef WIN32
#ifndef _WIN32_WCE
#include <winsock2.h>
#include <aclapi.h>
#endif
#define SHUT_RDWR SD_BOTH
#else
#ifndef __MACOS__
#include <sys/socket.h>
#endif
#include <utime.h>
#include <signal.h>
#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif
#ifndef SD_BOTH
#define SD_BOTH 2
#endif
#endif

#ifdef vms
#ifndef VMS
#define VMS
#endif
#endif

#include "filesys.h"
#include "errcodes.h"
/*POD
@c Handling system specific file operations
=abstract
The file T<filesys.h> contains file handling primitive functions. The reason for this module
is to have all system specific file handling functions to be separated in a single file.
All other modules use these functions that behave the same on Win32 platform as well as on UNIX.
=end
These functions are to be used by other parts of the program. They implement system
specific operations, and other levels need not care about these system specific stuff.

The function names are prefixed usually with T<file_>, some are prefixed with T<sys_>.

=toc

CUT*/

/*POD
=H file_fopen
@c Open a file

This is same as fopen.

VMS has some specialities when writing a file.

/*FUNCTION*/
FILE *file_fopen(
  char *pszFileName,
  char *pszOpenMode
  ){
/*noverbatim
CUT*/
#ifdef VMS
/* it is presented here to ease porting to VMS, but it was never tested. */
  if( *pszOpenMode == "w" )
    return fopen(pszFileName,pszOpenMode,"rat=cr","rfm=var");
  else
    return fopen(pszFileName,pszOpenMode);
#else

  return fopen(pszFileName,pszOpenMode);
#endif
  }


/*POD
=H file_fclose
@c Close a file

This is same as fclose. Nothing special. This is just a placeholder.

/*FUNCTION*/
void file_fclose(FILE *fp
  ){
/*noverbatim
CUT*/
  fclose(fp);
  }

/*POD
=H file_size
@c return the size of a file

/*FUNCTION*/
long file_size(char *pszFileName
  ){
/*noverbatim
CUT*/
  struct stat buf;
  int LastChar,i,st;

  i = strlen(pszFileName);
  if( i == 0 )return 0;
#ifdef __MACOS__
  st = stat(pszFileName,&buf);
#else
  i--;
  LastChar = pszFileName[i];
  if( LastChar == '/' )pszFileName[i] = (char)0;
  st = stat(pszFileName,&buf);
  if( LastChar == '/' )pszFileName[i] = LastChar;
#endif
  if( st == -1 )return 0;
  return buf.st_size;
  }

#ifdef WIN32
/* This is a Windows specific routine that converts normal UNIX
   time value (seconds since the epoch) to Windows FILETIME structure
*/
#define MSEPOCH 116444736000000000L
static void Utime2Filetime(long lTime,
                           PFILETIME pFileTime){
  union myuft {
    LONGLONG llTime;
    FILETIME FT;
    } *p;

  p = (union myuft *)pFileTime;
  p->llTime = lTime;
  p->llTime *= 10000000; /* convert from seconds to 100nsecs */
  /* This is the file time value of January 1, 1970. 00:00 */
  p->llTime += MSEPOCH;
  return;
  }

static void Filetime2Utime(long *plTime,
                           PFILETIME pFileTime){
  LONGLONG llTime;

  memcpy(&llTime,pFileTime,sizeof(llTime));

  /* This is the file time value of January 1, 1970. 00:00 */
  llTime -= MSEPOCH;

  llTime /= 10000000; /* convert from 100nsecs to seconds */
  *plTime = (long)llTime;
  return;
  }
#endif

/*POD
=H file_time_accessed
@c return the time the file was last accessed

/*FUNCTION*/
long file_time_accessed(char *pszFileName
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  FILETIME FileTime;
  long lTime;
  HANDLE hFile;
  int LastChar,i;

  i = strlen(pszFileName);
  if( i == 0 )return 0;
  i--;
  LastChar = pszFileName[i];
  if( LastChar == '/' || LastChar == '\\' )pszFileName[i] = (char)0;
  hFile = CreateFile(pszFileName,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if( LastChar == '/' || LastChar == '\\' )pszFileName[i] = LastChar;

  if( hFile == INVALID_HANDLE_VALUE )return 0;
  if( !GetFileTime(hFile,NULL,&FileTime,NULL) ){
    CloseHandle(hFile);
    return 0;
    }
  CloseHandle(hFile);
  Filetime2Utime(&lTime,&FileTime);
  return lTime;

#else
  struct stat buf;
  int LastChar,i,st;

  i = strlen(pszFileName);
  if( i == 0 )return 0;
#ifdef __MACOS__
  st = stat(pszFileName,&buf);
#else
  i--;
  LastChar = pszFileName[i];
  if( LastChar == '/' )pszFileName[i] = (char)0;
  st = stat(pszFileName,&buf);
  if( LastChar == '/' )pszFileName[i] = LastChar;
#endif
  if( st == -1 )return 0;
  return buf.st_atime;
#endif
  }

/*POD
=H file_time_modified
@c return the time the file was modified

/*FUNCTION*/
long file_time_modified(char *pszFileName
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  FILETIME FileTime;
  unsigned long lTime;
  HANDLE hFile;
  SYSTEMTIME Syst;
  int LastChar,i;

  i = strlen(pszFileName);
  if( i == 0 )return 0;
  i--;
  LastChar = pszFileName[i];
  if( LastChar == '/' || LastChar == '\\' )pszFileName[i] = (char)0;
  hFile = CreateFile(pszFileName,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if( LastChar == '/' || LastChar == '\\' )pszFileName[i] = LastChar;

  if( hFile == INVALID_HANDLE_VALUE )return 0;
  if( !GetFileTime(hFile,NULL,NULL,&FileTime) ){
    CloseHandle(hFile);
    return 0;
    }
  CloseHandle(hFile);
  FileTimeToSystemTime(&FileTime,&Syst);
  Filetime2Utime(&lTime,&FileTime);
  return lTime;

#else
  struct stat buf;
  int LastChar,i,st;

  i = strlen(pszFileName);
  if( i == 0 )return 0;
#ifdef __MACOS__
  st = stat(pszFileName,&buf);
#else
  i--;
  LastChar = pszFileName[i];
  if( LastChar == '/' )pszFileName[i] = (char)0;
  st = stat(pszFileName,&buf);
  if( LastChar == '/' )pszFileName[i] = LastChar;
#endif
  if( st == -1 )return 0;
  return buf.st_mtime;
#endif
  }

/*POD
=H file_time_created
@c return the time the file was created

/*FUNCTION*/
long file_time_created(char *pszFileName
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  FILETIME FileTime;
  long lTime;
  HANDLE hFile;
  int LastChar,i;

  i = strlen(pszFileName);
  if( i == 0 )return 0;
  i--;
  LastChar = pszFileName[i];
  if( LastChar == '/' || LastChar == '\\' )pszFileName[i] = (char)0;
  hFile = CreateFile(pszFileName,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if( LastChar == '/' || LastChar == '\\' )pszFileName[i] = LastChar;

  if( hFile == INVALID_HANDLE_VALUE )return 0;
  if( !GetFileTime(hFile,&FileTime,NULL,NULL) ){
    CloseHandle(hFile);
    return 0;
    }
  CloseHandle(hFile);
  Filetime2Utime(&lTime,&FileTime);
  return lTime;

#else
  struct stat buf;
  int LastChar,i,st;

  i = strlen(pszFileName);
  if( i == 0 )return 0;
#ifdef __MACOS__
  st = stat(pszFileName,&buf);
#else
  i--;
  LastChar = pszFileName[i];
  if( LastChar == '/' )pszFileName[i] = (char)0;
  st = stat(pszFileName,&buf);
  if( LastChar == '/' )pszFileName[i] = LastChar;
#endif
  if( st == -1 )return 0;
  return buf.st_ctime;
#endif
  }

/*POD
=H file_isdir
@c return true if the file is a directory

/*FUNCTION*/
int file_isdir(char *pszFileName
  ){
/*noverbatim
CUT*/
  struct stat buf;
  int LastChar,i,st;

  i = strlen(pszFileName);
  if( i == 0 )return 0;
#ifdef __MACOS__
  st = stat(pszFileName,&buf);
#else
  i--;
  LastChar = pszFileName[i];
  if( LastChar == '/' || LastChar == '\\' )pszFileName[i] = (char)0;
  st = stat(pszFileName,&buf);
  if( LastChar == '/' || LastChar == '\\' )pszFileName[i] = LastChar;
#endif
  if( st == -1 )return 0;
  return (buf.st_mode&S_IFDIR) ? -1 : 0;
  }

/*POD
=H file_isreg
@c return true if the file is a regular file (not directory)

/*FUNCTION*/
int file_isreg(char *pszFileName
  ){
/*noverbatim
CUT*/
  struct stat buf;
  int LastChar,i,st;

  i = strlen(pszFileName);
  if( i == 0 )return 0;
#ifdef __MACOS__
  st = stat(pszFileName,&buf);
#else
  i--;
  LastChar = pszFileName[i];
  if( LastChar == '/' || LastChar == '\\' )pszFileName[i] = (char)0;
  st = stat(pszFileName,&buf);
  if( LastChar == '/' || LastChar == '\\' )pszFileName[i] = LastChar;
#endif
  if( st == -1 )return 0;
  return (buf.st_mode&S_IFREG) ? -1 : 0;
  }

/*POD
=H file_exists
@c return true if the file exists

/*FUNCTION*/
int file_exists(char *pszFileName
  ){
/*noverbatim
CUT*/
  struct stat buf;
  int LastChar,i,st;

  i = strlen(pszFileName);
  if( i == 0 )return 0;
#ifdef __MACOS__
  st = stat(pszFileName,&buf);
#else
  i--;
  LastChar = pszFileName[i];
  if( LastChar == '/' || LastChar == '\\' )pszFileName[i] = (char)0;
  st = stat(pszFileName,&buf);
  if( LastChar == '/' || LastChar == '\\' )pszFileName[i] = LastChar;
#endif
  if( st == -1 )
    return 0;
  else
    return -1;
  }

/*POD
=H file_truncate
@c truncate a file to a given length

It return 0 on success and -1 on error.

/*FUNCTION*/
int file_truncate(FILE *fp,
                  long lNewFileSize
  ){
/*noverbatim
CUT*/
#if (defined(WIN32) || defined(__MACOS__))
#if BCC32
#define _chsize chsize
#endif
  return _chsize(_fileno(fp),lNewFileSize);
#else
  return ftruncate(fileno(fp),lNewFileSize);
#endif
  }

/*POD
=H file_fgetc
@c Get a single character from a file

Nothing special, it is just a placeholder.

/*FUNCTION*/
int file_fgetc(FILE *fp
  ){
/*noverbatim
CUT*/
  return fgetc(fp);
  }

/*POD
=H file_ferror
@c ferror

Nothing special, it is just a placeholder.

/*FUNCTION*/
int file_ferror(FILE *fp
  ){
/*noverbatim
CUT*/
  return ferror(fp);
  }

/*POD
=H file_fread
@c fread

Nothing special, it is just a placeholder.

/*FUNCTION*/
int file_fread(char *buf,
               int size,
               int count,
               FILE *fp
  ){
/*noverbatim
CUT*/
  return fread(buf,size,count,fp);
  }

/*POD
=H file_fwrite
@c fwrite

Nothing special, it is just a placeholder.

/*FUNCTION*/
int file_fwrite(char *buf,
               int size,
               int count,
               FILE *fp
  ){
/*noverbatim
CUT*/
  return fwrite(buf,size,count,fp);
  }

/*POD
=H file_fputc
@c Get a single character from a file

Nothing special, it is just a placeholder.

/*FUNCTION*/
int file_fputc(int c, FILE *fp
  ){
/*noverbatim
CUT*/
  return fputc(c,fp);
  }

/*POD
=H file_setmode
@c Set the mode of a file stream to binary or to ASCII

Nothing special, it is just a placeholder. On UNIX this is doing
nothing transparently.

/*FUNCTION*/
void file_setmode(FILE *fp,
                  int mode
  ){
/*noverbatim
CUT*/
#ifdef WIN32
#if BCC32
#define _setmode setmode
#endif
  _setmode(_fileno(fp),mode);
#endif
  return;
  }

/*POD
=H file_binmode
@c Set a file stream to binary mode
/*FUNCTION*/
void file_binmode(FILE *fp
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  file_setmode(fp,_O_BINARY);
#endif
  return;
  }

/*POD
=H file_textmode
@c Set a file stream to text mode
/*FUNCTION*/
void file_textmode(FILE *fp
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  file_setmode(fp,_O_TEXT);
#endif
  return;
  }

/*POD
=H file_flock
@c Lock a file

/*FUNCTION*/
int file_flock(FILE *fp,
               int iLockType
  ){
/*noverbatim
CUT*/
#define LK_LEN		0xffff0000
#ifdef WIN32
#define LK_ERR(f,i)	((f) ? (i = 0) : 0 )

  OVERLAPPED o;
  int i = -1;
  HANDLE fh;

  fh = (HANDLE)_get_osfhandle(_fileno(fp));
  memset(&o, 0, sizeof(o));

  switch(iLockType) {
    case LOCK_SH:		/* shared lock */
      LK_ERR(LockFileEx(fh, 0, 0, LK_LEN, 0, &o),i);
      break;
    case LOCK_EX:		/* exclusive lock */
	LK_ERR(LockFileEx(fh, LOCKFILE_EXCLUSIVE_LOCK, 0, LK_LEN, 0, &o),i);
	break;
    case LOCK_SH|LOCK_NB:	/* non-blocking shared lock */
	LK_ERR(LockFileEx(fh, LOCKFILE_FAIL_IMMEDIATELY, 0, LK_LEN, 0, &o),i);
	break;
    case LOCK_EX|LOCK_NB:	/* non-blocking exclusive lock */
	LK_ERR(LockFileEx(fh,
		       LOCKFILE_EXCLUSIVE_LOCK|LOCKFILE_FAIL_IMMEDIATELY,
		       0, LK_LEN, 0, &o),i);
	break;
    case LOCK_UN:		/* unlock lock */
	LK_ERR(UnlockFileEx(fh, 0, LK_LEN, 0, &o),i);
	break;
  default:			/* unknown */
	/*errno = EINVAL;*/
	break;
    }
    return i;
#elif defined(__MACOS__)
  return 0;
#else
/*  return flock(fileno(fp),iLockType); */
  struct flock fl;
  int WaitOption;

  switch(iLockType) {
    case LOCK_SH:		/* shared lock */
      iLockType =  F_RDLCK;
      WaitOption = F_SETLKW;
      break;
    case LOCK_EX:		/* exclusive lock */
      iLockType = F_WRLCK;
      WaitOption = F_SETLKW;
      break;
    case LOCK_SH|LOCK_NB:	/* non-blocking shared lock */
      iLockType =  F_RDLCK;
      WaitOption = F_SETLK;
      break;
    case LOCK_EX|LOCK_NB:	/* non-blocking exclusive lock */
      iLockType = F_WRLCK;
      WaitOption = F_SETLK;
      break;
    case LOCK_UN:		/* unlock lock */
      iLockType = F_UNLCK;
      WaitOption = F_SETLKW;
      break;
    default:			/* unknown */
      return -1;
    }
  fl.l_type   = iLockType;
  fl.l_whence = SEEK_SET;
  fl.l_start  = 0;
  fl.l_len    = LK_LEN;

  return fcntl( fileno( fp ), F_SETLKW , &fl );

#endif
  }

/*POD
=H file_lock
@c Lock a range of a file

/*FUNCTION*/
int file_lock(FILE *fp,
              int iLockType,
              long lStart,
              long lLength
  ){
/*noverbatim
CUT*/
#ifdef WIN32
#undef LK_ERR
#undef LK_LEN
#define LK_ERR(f,i)	return ((f) ? 0 : -1)
#define LK_LEN		0xffff0000
#define dwReserved 0L
  OVERLAPPED o;
  int i = -1;
  HANDLE fh;

  fh = (HANDLE)_get_osfhandle(_fileno(fp));
  memset(&o, 0, sizeof(o));
  o.Offset = (DWORD)lStart;

  switch(iLockType) {
    case LOCK_SH:		/* shared lock */
      LK_ERR(LockFileEx(fh, 0, dwReserved, lLength, 0, &o),i);
    case LOCK_EX:		/* exclusive lock */
	    LK_ERR(LockFileEx(fh, LOCKFILE_EXCLUSIVE_LOCK, dwReserved, lLength, 0, &o),i);
    case LOCK_SH|LOCK_NB:	/* non-blocking shared lock */
	    LK_ERR(LockFileEx(fh, LOCKFILE_FAIL_IMMEDIATELY, dwReserved, lLength, 0, &o),i);
    case LOCK_EX|LOCK_NB:	/* non-blocking exclusive lock */
	    LK_ERR(LockFileEx(fh, LOCKFILE_EXCLUSIVE_LOCK|LOCKFILE_FAIL_IMMEDIATELY, dwReserved, lLength, 0, &o),i);
    case LOCK_UN:		/* unlock lock */
	    LK_ERR(UnlockFileEx(fh, 0, lLength, 0, &o),i);
    default:			/* unknown */
      return -1;
    }
#elif defined(__MACOS__)
  return 0;
#else
  struct flock fl;
  int WaitOption;

  switch(iLockType) {
    case LOCK_SH:		/* shared lock */
      iLockType =  F_RDLCK;
      WaitOption = F_SETLKW;
      break;
    case LOCK_EX:		/* exclusive lock */
      iLockType = F_WRLCK;
      WaitOption = F_SETLKW;
      break;
    case LOCK_SH|LOCK_NB:	/* non-blocking shared lock */
      iLockType =  F_RDLCK;
      WaitOption = F_SETLK;
      break;
    case LOCK_EX|LOCK_NB:	/* non-blocking exclusive lock */
      iLockType = F_WRLCK;
      WaitOption = F_SETLK;
      break;
    case LOCK_UN:		/* unlock lock */
      iLockType = F_UNLCK;
      WaitOption = F_SETLKW;
      break;
    default:			/* unknown */
      return -1;
    }
  fl.l_type   = iLockType;
  fl.l_whence = SEEK_SET;
  fl.l_start  = lStart;
  fl.l_len    = lLength;

  return fcntl( fileno( fp ), F_SETLKW , &fl );
#endif
  }

/*POD
=H file_feof
@c Check end of file condition

Nothing special, it is just a placeholder.

/*FUNCTION*/
int file_feof(FILE *fp
  ){
/*noverbatim
CUT*/
  return feof(fp);
  }

/*POD
=H file_mkdir
@c Create a directory

This is the usual UNIX mkdir function. The difference is that the access code is always 0777 on UNIX
which means that the user, group and others can read, write and execute the directory. If the permission
needed is different from that you have to call the T<file_chmod> function as soon as it becomes available.

The argument of the function is the name of the desired directory.

/*FUNCTION*/
int file_mkdir(char *pszDirectoryName
  ){
/*noverbatim
CUT*/
  char *s;

#ifndef __MACOS__
  for( s = pszDirectoryName ; *s ; s++ )if( *s == '\\' )*s= '/';
#endif
#ifdef WIN32
  return _mkdir(pszDirectoryName);
#else
  return mkdir(pszDirectoryName,0777);
#endif
  }

/*POD
=H file_rmdir
@c Remove a directory

This is the usual UNIX rmdir function.

The argument of the function is the name of the directory to be deleted.

/*FUNCTION*/
int file_rmdir(char *pszDirectoryName
  ){
/*noverbatim
CUT*/
  char *s;

#ifndef __MACOS__
  for( s = pszDirectoryName ; *s ; s++ )if( *s == '\\' )*s= '/';
#endif
#ifdef WIN32
  /* Note that there is no need to convert the / characters to \
     because / is a valid separator under Windows NT just as \ is.  */
  return _rmdir(pszDirectoryName);
#else
  return rmdir(pszDirectoryName);
#endif
  }

/*POD
=H file_remove
@c Remove a file

Nothing special, it is just a placeholder. This function performs the UNIX T<remove> functionality. This
function also exists under WIN32, therefore this function is only a placeholder.

/*FUNCTION*/
int file_remove(char *pszFileName
  ){
/*noverbatim
CUT*/
  char *s;

#ifndef __MACOS__
  for( s = pszFileName ; *s ; s++ )if( *s == '\\' )*s= '/';
#endif
  return remove(pszFileName);
  }

/* Note that the recursive calls can use the same file name buffer and
   still the different concurrently running threads use different buffers.
   This would NOT be the case if buffer is static inside the recursive function.
   We could use an auto buffer but that is waste of stack space.
*/
#define MAX_FNLEN 1024
int file_deltree_r(char * buffer){
  tDIR DirList;
  DIR*pDirList;

  struct dirent *pD;
  int dirlen;

  dirlen=strlen(buffer);
#ifdef __MACOS__
  if( buffer[dirlen-1] != ':' ){
    dirlen++;
    if( dirlen >= MAX_FNLEN )return -1;
    strcpy(buffer+dirlen-1,":");
    }
#else
  if( buffer[dirlen-1] != '/' ){
    dirlen++;
    if( dirlen >= MAX_FNLEN )return -1;
    strcpy(buffer+dirlen-1,"/");
    }
#endif
  pDirList = file_opendir(buffer,&DirList);
  if( pDirList == NULL )return -1;
  while( pD = file_readdir(pDirList) ){
    /* skip . and .. directories */
    if( pD->d_name[0] == '.' && 
       ( pD->d_name[1] == (char)0 ||
         ( pD->d_name[1] == '.' && pD->d_name[2] == (char)0 ) ) )continue;
    if( dirlen+strlen(pD->d_name) >= MAX_FNLEN )return -1;
    strcpy(buffer+dirlen,pD->d_name);
    if( file_isdir(buffer) )
      file_deltree_r(buffer);
    else
      file_remove(buffer);
    }
  file_closedir(pDirList);
  dirlen--;
  buffer[dirlen] = (char)0;
  return file_rmdir(buffer);
  }

/*POD
=H file_deltree
@c Delete a directory tree

/*FUNCTION*/
int file_deltree(char *pszDirectoryName
  ){
/*noverbatim
CUT*/
  char buffer[MAX_FNLEN];
#ifndef __MACOS__
  char *s;

  for( s = pszDirectoryName ; *s ; s++ )if( *s == '\\' )*s= '/';
#endif
  if( ! file_exists(pszDirectoryName) )return -1;
  if( ! file_isdir(pszDirectoryName) )return -1;
  /* does not fit into the buffer ? */
  if( strlen(pszDirectoryName) >= MAX_FNLEN )return -1;

  strcpy(buffer,pszDirectoryName);
  return file_deltree_r(buffer);
  }

/*POD
=H file_MakeDirectory

This function is a bit out of the line of the other functions in this module. This function uses the
T<file_mkdir> function to create a directory. The difference is that this function tries to create a
directory recursively. For example you can create the directory

T</usr/bin/scriba>

with a simple call and the function will create the directories T</usr> if it did not exist, then 
T</usr/bin> and finally T</usr/bin/scriba> The function fails if the directory can not be created
because of access restrictions or because the directory path or a sub path already exists, and is not
a directory.

The argument of the function is the name of the desired directory.

The function alters the argument replacing each \ character to /

The argument may end with / since v1.0b30

If the argument is a Windows full path including the drive letter, like
'C:' the function tries to create the directory 'C:', which fails, but
ignores this error because only the last creation in the line down the
directory path is significant.

In case of error, the argument may totally be destroyed.

/*FUNCTION*/
int file_MakeDirectory(char *pszDirectoryName
  ){
/*noverbatim
CUT*/
  char *s;
  int iDirNameLen,i,iResult;

#ifndef __MACOS__
  for( s=pszDirectoryName ; *s ; s++ )
    if( *s == '\\' )*s = '/';
#endif
  iDirNameLen = strlen(pszDirectoryName);

  i = 0;
  iResult = 0;
  while( i < iDirNameLen ){
#ifdef __MACOS__
    while( pszDirectoryName[i] && pszDirectoryName[i] != ':' )i++;
#else
    while( pszDirectoryName[i] && pszDirectoryName[i] != '/' )i++;
#endif
    pszDirectoryName[i] = (char)0;
    if( file_exists(pszDirectoryName) ){
      if( ! file_isdir(pszDirectoryName) )
          return -1;/* the path exists and there is a non-directory sub-path in it*/
      else{
        iResult = 0;
#ifdef __MACOS__
        if( i < iDirNameLen ) pszDirectoryName[i] = ':';
#else
        if( i < iDirNameLen ) pszDirectoryName[i] = '/';
#endif
        }
      }else{
      iResult = file_mkdir(pszDirectoryName);
#ifdef __MACOS__
      if( i < iDirNameLen ) pszDirectoryName[i] = ':';
#else
      if( i < iDirNameLen ) pszDirectoryName[i] = '/';
#endif
      }
    i++;
    }
  return iResult;
  }


/*
TO_HEADER:
#ifdef WIN32
struct dirent {
  char d_name[MAX_PATH];
  };
typedef struct _OpenDir {
  HANDLE hFindFile;
  WIN32_FIND_DATA FindFileData;
  struct dirent CurrentEntry;
  } DIR,tDIR;
#else
typedef unsigned char tDIR;
#endif
*/

/*POD
=H file_opendir
@c Open a directory for listing

This function implements the T<opendir> function of UNIX. The difference between this implementation and
the UNIX version is that this implementation requires a T<DIR> structure to be passed as an argument. The
reason for this is that the Windows system calls do not allocate memory and pass return values in structures
allocated by the caller. Because we did not want to implement memory allocation in these routines
we followed the Windows like way.

The first argument T<pszDirectoryName> is a ZCAR directory name to be scanned. The second argument is an
allocated T<DIR> structure that has to be valid until the T<file_closedir> is called.

The second parameter under UNIX is not used. However to be safe and portable to Win32 the parameter
should be handled with care.
/*FUNCTION*/
DIR *file_opendir(char *pszDirectoryName,
                  tDIR *pDirectory
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  if( strlen(pszDirectoryName) >= MAX_PATH - 4 )return NULL;
  strcpy(pDirectory->CurrentEntry.d_name,pszDirectoryName);
  strcat(pDirectory->CurrentEntry.d_name,"/*.*");
  pDirectory->hFindFile = FindFirstFile(pDirectory->CurrentEntry.d_name,&(pDirectory->FindFileData));
  if( pDirectory->hFindFile == INVALID_HANDLE_VALUE )return NULL;
  return pDirectory;
#else
  return opendir(pszDirectoryName);
#endif
  }

/*POD
=H file_readdir
@c Read next item from a directory

This function is the implementation of the UNIX T<readdir>

/*FUNCTION*/
struct dirent *file_readdir(DIR *pDirectory
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  char *s;

  strcpy(pDirectory->CurrentEntry.d_name,pDirectory->FindFileData.cFileName);
  if( ! FindNextFile(pDirectory->hFindFile,&(pDirectory->FindFileData)) ){
    *(pDirectory->FindFileData.cFileName) = (char)0;
    }
  s = pDirectory->CurrentEntry.d_name;
  if( *s ){
    for( ; *s ; s++ )if( isupper(*s) )*s = tolower(*s);
    return &(pDirectory->CurrentEntry);
    }
  else
    return NULL;
#else
  return readdir(pDirectory);
#endif
  }

/*POD
=H file_closedir
@c Close a directory opened for listing

/*FUNCTION*/
void file_closedir(DIR *pDirectory
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  FindClose(pDirectory->hFindFile);
#else
  closedir(pDirectory);
#endif
  return;
  }

/*POD
=H file_sleep
@c Sleep the process

/*FUNCTION*/
void sys_sleep(long lSeconds
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  lSeconds *= 1000; /* convert to milliseconds */
  Sleep((DWORD)lSeconds);
#else
  sleep(lSeconds);
#endif
  }

/*POD
=H file_curdir
@c Get the current working directory

The first argument should point to a buffer having space for at least
T<cbBuffer> characters. The function will copy the name of the current
directory into this buffer. 

Return value is zero on success. If the current directory can not be
retrieved or the buffer is too short the return value is -1.
/*FUNCTION*/
int file_curdir(char *Buffer,
                unsigned long cbBuffer
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  int i;

  i = GetCurrentDirectory(cbBuffer,Buffer);
  if( i == 0 )return -1;
  return 0;
#else
  char *s;

  s = getcwd(Buffer,cbBuffer);
  if( s == NULL )return -1;
  return 0;
#endif
  }

/*POD
=H file_chdir
@c Change the current working direcory

/*FUNCTION*/
int file_chdir(char *Buffer
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  int i;

  i = SetCurrentDirectory(Buffer);
  if( i == 0 )return -1;
  return 0;
#else
  int i;

  return chdir(Buffer);
#endif
  }

/* HERE WE HAVE SOME FUNCTIONS THAT ARE NEEDED TO IMPLEMENT CHOWN UNDERR WINDOWS NT

The routine was written from the sources of the Windows NT command line utility chown
of Alexander.Frink@Uni-Mainz.DE, at http://wwwthep.physik.uni-mainz.de/~frink/nt.html

*/
#ifdef WIN32
#define MAXNAMELEN 256

#define toUnicode(FROM,TO)                     \
    MultiByteToWideChar(CP_OEMCP,              \
                        0,                     \
                        (char *)FROM,          \
                        strlen((char *)FROM)+1,\
                        TO,                    \
                        sizeof(TO))

#define fromUnicode(FROM,TO)             \
    WideCharToMultiByte(CP_OEMCP,        \
                        0,               \
                        FROM,            \
                        -1,              \
                        TO,              \
                        sizeof(TO),      \
                        NULL,            \
                        NULL)

#define SZ_SD_BUF   100


BOOL 
GetAccountSid( 
    LPTSTR SystemName, 
    LPTSTR AccountName, 
    PSID *Sid 
    ) 
{ 
    LPTSTR ReferencedDomain=NULL; 
    DWORD cbSid=128;    // initial allocation attempt 
    DWORD cchReferencedDomain=16; // initial allocation size 
    SID_NAME_USE peUse; 
    BOOL bSuccess=FALSE; // assume this function will fail 
#if BCC32
    BOOL bLeave=FALSE;
#define __finally if( bLeave )
#define __leave {bLeave=TRUE; break;}
#define __try do
#define __end_try while(0);
#else
#define __end_try
#endif 
    __try { 
 
    // 
    // initial memory allocations 
    // 
    *Sid = (PSID)HeapAlloc(GetProcessHeap(), 0, cbSid); 
 
    if(*Sid == NULL) __leave; 
 
    ReferencedDomain = (LPTSTR)HeapAlloc( 
                    GetProcessHeap(), 
                    0, 
                    cchReferencedDomain * sizeof(TCHAR) 
                    ); 
 
    if(ReferencedDomain == NULL) __leave; 
 
    // 
    // Obtain the SID of the specified account on the specified system. 
    // 
    while(!LookupAccountName( 
                    SystemName,         // machine to lookup account on 
                    AccountName,        // account to lookup 
                    *Sid,               // SID of interest 
                    &cbSid,             // size of SID 
                    ReferencedDomain,   // domain account was found on 
                    &cchReferencedDomain, 
                    &peUse 
                    )) { 
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) { 
            // 
            // reallocate memory 
            // 
            *Sid = (PSID)HeapReAlloc( 
                        GetProcessHeap(), 
                        0, 
                        *Sid, 
                        cbSid 
                        ); 
            if(*Sid == NULL) __leave; 
 
            ReferencedDomain = (LPTSTR)HeapReAlloc( 
                        GetProcessHeap(), 
                        0, 
                        ReferencedDomain, 
                        cchReferencedDomain * sizeof(TCHAR) 
                        ); 
            if(ReferencedDomain == NULL) __leave; 
        } 
        else __leave; 
    } 
 
    // 
    // Indicate success. 
    // 
    bSuccess = TRUE; 
 
    } __end_try 
    __finally { 
 
    // 
    // Cleanup and indicate failure, if appropriate. 
    // 
 
    HeapFree(GetProcessHeap(), 0, ReferencedDomain); 
 
    if(!bSuccess) { 
        if(*Sid != NULL) { 
            HeapFree(GetProcessHeap(), 0, *Sid); 
            *Sid = NULL; 
        } 
    } 
 
    } // finally 
 
    return bSuccess; 
} 

BOOL 
GetSid( 
    LPTSTR AccountName, 
    PSID *Sid 
    ) 
{
/*
The NetGetDCName function is implemented in netapi32.dll This function is specific to Windows NT. If
we create the exe file statically linking netapi32.lib then the exe can not start on Win95/98. This
dynamic linking implemented in this code (since build14) allows Win95/98 to run the code, and get a
trappable (on error goto works) error if the basic program wants to set the owner of a file, which
is not available under Win95/98 anyway.

Note that a multi-thread implementation of this code should also work fine. The different threads may
load the dll multiple times, but that is not a problem. The function NetGetDCName has a single entry
point and that is loaded into the static variable pNetGetDCName. After this has been set the first time
it will not change later even during another thread updating this variable. It can happen that
thread "A" sets the value of this variable, and half of the bytes are updated and other parts are not and at
this moment thread "B" uses the variable to call the function it points to. But thread "B" should see the
same address even in this case, because the update value is identical to the original.
*/
    typedef NET_API_STATUS (*tNetGetDCName)(LPWSTR,LPWSTR,LPBYTE *);
    static tNetGetDCName pNetGetDCName = NULL;
#define NetGetDCName pNetGetDCName

    HINSTANCE Netapi32DllInstance;
    PBYTE wszDomainController;
    UCHAR szDomainController[MAXNAMELEN];
    UCHAR szDomain[MAXNAMELEN];
    WCHAR wszDomain[MAXNAMELEN];
    char *s,*p;

    /* if the account name is DOMAIN\USER the copy the DOMAIN into szDomain */
    s = AccountName;
    p = szDomain;
    while( *s && *s != '\\' ){
      *p++ = *s++;
      }
    if( *s ){
      *p = (char)0;
      AccountName = s+1; /* point after the \ to contain the USER */
      }
    else
      *szDomain = (char)0;
    if( *szDomain ) {
        toUnicode(szDomain,wszDomain);
        if( NetGetDCName == NULL ){
          if( (Netapi32DllInstance = LoadLibrary("netapi32")) != NULL ){
            NetGetDCName = (tNetGetDCName)GetProcAddress(Netapi32DllInstance,"NetGetDCName");
            }
          }
        if( NetGetDCName && NetGetDCName(NULL,wszDomain,&wszDomainController) != NERR_Success )return FALSE;
        fromUnicode((LPWSTR)wszDomainController,szDomainController);
        return GetAccountSid(szDomainController,
                             AccountName,
                             Sid);
    } else {
        return GetAccountSid(NULL,
                             AccountName,
                             Sid);
    }
}

BOOL SetPrivilege(
    HANDLE hToken,          // token handle (NULL: current process)
    LPCTSTR Privilege,      // Privilege to enable/disable
    BOOL bEnablePrivilege   // TRUE to enable.  FALSE to disable
    )
{
    TOKEN_PRIVILEGES tp;
    LUID luid;
    TOKEN_PRIVILEGES tpPrevious;
    DWORD cbPrevious=sizeof(TOKEN_PRIVILEGES);
    BOOL CloseAtEnd=FALSE;

    //
    // Retrieve a handle of the access token
    //
    if (hToken==NULL) {
		if (!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hToken))return FALSE;
        CloseAtEnd=TRUE;
	}

    if (!LookupPrivilegeValue( NULL, Privilege, &luid )) {
		return FALSE;
	}

    //
    // first pass.  get current privilege setting
    //
    tp.PrivilegeCount           = 1;
    tp.Privileges[0].Luid       = luid;
    tp.Privileges[0].Attributes = 0;

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tp,
            sizeof(TOKEN_PRIVILEGES),
            &tpPrevious,
            &cbPrevious
            );

    if (GetLastError() != ERROR_SUCCESS) {
		return FALSE;
	}

    //
    // second pass.  set privilege based on previous setting
    //
    tpPrevious.PrivilegeCount       = 1;
    tpPrevious.Privileges[0].Luid   = luid;

    if(bEnablePrivilege) {
        tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
    }
    else {
        tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
            tpPrevious.Privileges[0].Attributes);
    }

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tpPrevious,
            cbPrevious,
            NULL,
            NULL
            );

    if (GetLastError() != ERROR_SUCCESS) {
		return FALSE;
	}

    /* call CloseHandle or not ??? */
    if (CloseAtEnd) {
        CloseHandle(hToken);
    }
    
    return TRUE;
}
#endif //WIN32

/*POD
=H file_chown
@c Change owner of a file

This function implements the chown command of the UNIX operating system
on UNIX and Windows NT. The first argument is the ZCHAR terminated
file name. No wild card characters are allowed.

The second argument is the name of the desired new user. The function
sets the owner of the file to the specified user, and returns zero
if the setting was succesful. If the setting fails the function returns
an error code. The error codes are:

COMMAND_ERROR_CHOWN_NOT_SUPPORTED
COMMAND_ERROR_CHOWN_INVALID_USER
COMMAND_ERROR_CHOWN_SET_OWNER

/*FUNCTION*/
int file_chown(char *pszFile,
               char *pszOwner
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  UCHAR                ucFileSDBuf[SZ_SD_BUF] = "";
  PSECURITY_DESCRIPTOR psdFileSDrel = (PSECURITY_DESCRIPTOR)&ucFileSDBuf;
  PSECURITY_DESCRIPTOR psdFileSDabs = NULL;
  PSID                 psidOwner;

  if (GetVersion() & 0x80000000)return COMMAND_ERROR_CHOWN_NOT_SUPPORTED;

  SetPrivilege(NULL,SE_TAKE_OWNERSHIP_NAME,TRUE);
  SetPrivilege(NULL,SE_RESTORE_NAME,TRUE);
  SetPrivilege(NULL,SE_BACKUP_NAME,TRUE);
  SetPrivilege(NULL,SE_CHANGE_NOTIFY_NAME,TRUE);

  if (!GetSid(pszOwner,&psidOwner))return COMMAND_ERROR_CHOWN_INVALID_USER;

  if( !InitializeSecurityDescriptor(psdFileSDrel, SECURITY_DESCRIPTOR_REVISION))
    return COMMAND_ERROR_CHOWN_SET_OWNER;
  if (!SetSecurityDescriptorOwner(psdFileSDrel,psidOwner, FALSE))
    return COMMAND_ERROR_CHOWN_SET_OWNER;
  if (!IsValidSecurityDescriptor(psdFileSDrel))
    return COMMAND_ERROR_CHOWN_SET_OWNER;
  if (!SetFileSecurity(pszFile,(SECURITY_INFORMATION)(OWNER_SECURITY_INFORMATION),psdFileSDrel))
    return COMMAND_ERROR_CHOWN_SET_OWNER;

  return 0;
#elif defined(__MACOS__)
  /* Macintosh doesn't have file ownership */
  return 0;
#else
  struct passwd *pasw;

  pasw = getpwnam(pszOwner);
  if( pasw == NULL )return COMMAND_ERROR_CHOWN_INVALID_USER;
  if( chown(pszFile,pasw->pw_uid,-1) )return COMMAND_ERROR_CHOWN_SET_OWNER;
#endif
  }

/*POD
=H file_getowner
@c Get the owner of a file

/*FUNCTION*/
int file_getowner(char *pszFileName,
                  char *pszOwnerBuffer,
                  long cbOwnerBuffer
 ){
/*noverbatim
CUT*/
#ifdef WIN32
#define UNAMEMAXSIZE 256

  PSID psid;
  PSECURITY_DESCRIPTOR ppSecurityDescriptor;
  DWORD cbName=UNAMEMAXSIZE;
  char Name[UNAMEMAXSIZE];
  char ReferencedDomainName[256];
  DWORD cbReferencedDomainName = 256;
  SID_NAME_USE peUse;
  int Result;
  
  Result =
  GetNamedSecurityInfo(pszFileName,
                       SE_FILE_OBJECT,
                       OWNER_SECURITY_INFORMATION,
                       &psid,
                       NULL, // sidGroup is not needed
                       NULL, // dACL is not needed
                       NULL, // sACL is not needed
                       &ppSecurityDescriptor);

  if( Result )return Result;
  
  Result = 
  LookupAccountSid(NULL,
                   psid,
                   Name,
                   &cbName ,
                   ReferencedDomainName,
                   &cbReferencedDomainName,
                   &peUse);

  LocalFree( ppSecurityDescriptor );
  if( Result ){
    if( (cbName=strlen(Name)) + 
        (cbReferencedDomainName=strlen(ReferencedDomainName)) +
                                                2 <= (unsigned)cbOwnerBuffer ){
      if( cbReferencedDomainName ){
        strcpy(pszOwnerBuffer,ReferencedDomainName);
        strcat(pszOwnerBuffer,"\\");
        }else
        *pszOwnerBuffer = (char)0;
      strcat(pszOwnerBuffer,Name);
      }
    return 0;
    }
  return 1;
#elif defined(__MACOS__)
  /* Macintosh doesn't have file ownership */
  char *owner = "Apple";
  
  if( strlen(owner) < cbOwnerBuffer ){
    strcpy(pszOwnerBuffer,owner);
    return 0;
    }
  return 1;
#else

  struct stat FileState;
  struct passwd *pasw;

  if( stat(pszFileName,&FileState) )return 1;

  pasw = getpwuid(FileState.st_uid);
  if( strlen(pasw->pw_name) < cbOwnerBuffer ){
    strcpy(pszOwnerBuffer,pasw->pw_name);
    return 0;
    }
  return 1;

#endif
  }


#ifdef WIN32
/*
Open a file so that the file time can be set. In case the file is read only the
function alters the file attributes so that the file is NOT read only anymore.
When the file time is altered calling my_CloseHandle restores the file attributes
*/
static HANDLE my_CreateFile(char *pszFile, int *fFlag){
  HANDLE hFile;
  DWORD attrib;

  *fFlag = 0; /* the attributes were not altered, attrib does not hold any value to restore */
  hFile = CreateFile(pszFile,
                     GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                     0);
  if( hFile != INVALID_HANDLE_VALUE )return hFile;

  attrib = GetFileAttributes(pszFile);
  if( attrib & FILE_ATTRIBUTE_READONLY ){
    *fFlag = 1; /* remember that we have altered the attributes, has to be restored when closing the handle */
    /* remove the read only bit from the flags and ... */
    SetFileAttributes(pszFile,attrib & ~FILE_ATTRIBUTE_READONLY);
    /* ... try to open the file again */
    hFile = CreateFile(pszFile,
                       GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                       0);
    /* the file is now opned ok */
    if( hFile != INVALID_HANDLE_VALUE )return hFile;
    /* if the file still cannot be opened, then reset the attributes to the original */
    SetFileAttributes(pszFile,attrib);
    *fFlag = 0;
    }
  return hFile;
  }

/* Close the file handle that was opened by my_CreateFile to alter one of the time values of a file.
   If the read-only flag was cleared to make the code able to alter the file time then this flag is
   reset after the operation.
*/
static void my_CloseHandle(HANDLE hFile, char *pszFile, int *fFlag){
  DWORD attrib;

  if( *fFlag ){
    /* this may be slower to retrieve the file attributes again, but it does not hurt in case somebody
       alters some other attribute in the mean time. */
    attrib = GetFileAttributes(pszFile);
    attrib |= FILE_ATTRIBUTE_READONLY;
    SetFileAttributes(pszFile,attrib);
    }
  CloseHandle(hFile);
  }
#endif
/*POD
=H file_SetCreateTime
@c Set the creation time of a file

Note that this time value does not exist on UNIX and
therefore calling this function under UNIX result error.

The argument to the function is the file name and the desired time
in number of seconds since the epoch. (January 1, 1970. 00:00)

If the time was set the return value is zero. If there is an error the
return value is the error code.
/*FUNCTION*/
int file_SetCreateTime(char *pszFile,
                       long lTime
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  FILETIME FileTime;
  HANDLE hFile;
  int fFlag;

  Utime2Filetime(lTime,&FileTime);
  hFile = my_CreateFile(pszFile,&fFlag);
  if( hFile == INVALID_HANDLE_VALUE )
    return COMMAND_ERROR_CREATIME_FAIL;
  if( !SetFileTime(hFile,&FileTime,NULL,NULL) ){
    my_CloseHandle(hFile,pszFile,&fFlag);
    return COMMAND_ERROR_CREATIME_FAIL;
    }
  my_CloseHandle(hFile,pszFile,&fFlag);
  return 0;
#else
  return 0;
#endif
  }

/*POD
=H file_SetModifyTime
@c Set the modification time of a file

The argument to the function is the file name and the desired time
in number of seconds since the epoch. (January 1, 1970. 00:00)

If the time was set the return value is zero. If there is an error the
return value is the error code.
/*FUNCTION*/
int file_SetModifyTime(char *pszFile,
                       long lTime
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  FILETIME FileTime;
  HANDLE hFile;
  int fFlag;

  Utime2Filetime(lTime,&FileTime);
  hFile = my_CreateFile(pszFile,&fFlag);
  if( hFile == INVALID_HANDLE_VALUE )
    return COMMAND_ERROR_MODTIME_FAIL;
  if( !SetFileTime(hFile,NULL,NULL,&FileTime) ){
    my_CloseHandle(hFile,pszFile,&fFlag);
    return COMMAND_ERROR_MODTIME_FAIL;
    }
  my_CloseHandle(hFile,pszFile,&fFlag);
  return 0;
#else
  struct utimbuf uTime;

  uTime.modtime = lTime;
  uTime.actime  = file_time_accessed(pszFile);
  if( utime(pszFile,&uTime) == -1 )return COMMAND_ERROR_MODTIME_FAIL;
  return 0;
#endif
  }

/*POD
=H file_SetAccessTime
@c Set the access time of a file

The argument to the function is the file name and the desired time
in number of seconds since the epoch. (January 1, 1970. 00:00)

If the time was set the return value is zero. If there is an error the
return value is the error code.
/*FUNCTION*/
int file_SetAccessTime(char *pszFile,
                       long lTime
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  FILETIME FileTime;
  HANDLE hFile;
  int fFlag;

  Utime2Filetime(lTime,&FileTime);
  hFile = my_CreateFile(pszFile,&fFlag);
  if( hFile == INVALID_HANDLE_VALUE )
    return COMMAND_ERROR_ACCTIM_FAIL;
  if( !SetFileTime(hFile,NULL,&FileTime,NULL) ){
    my_CloseHandle(hFile,pszFile,&fFlag);
    return COMMAND_ERROR_ACCTIM_FAIL;
    }
  my_CloseHandle(hFile,pszFile,&fFlag);
  return 0;
#else
  struct utimbuf uTime;

  uTime.actime = lTime;
  uTime.modtime  = file_time_modified(pszFile);
  if( utime(pszFile,&uTime) == -1 )return COMMAND_ERROR_ACCTIM_FAIL;
  return 0;
#endif
  }

/* Initialize the Windows socket sub system to use the version 2.2 */
#ifdef WIN32
#define WIN32SOCKETINIT  WORD wVersionRequested; \
  WSADATA wsaData; int err; \
  wVersionRequested = MAKEWORD( 2, 2 ); \
  err = WSAStartup( wVersionRequested, &wsaData );\
  if ( err != 0 )return err;
#else
#define WIN32SOCKETINIT int err;
#endif


/*POD
=H file_gethostname
@c Get the name of the actual host

This function gets the name of the host that runs the program.
The result of the function is positive if no TCP/IP protocol is
available on the machine or some error occured.

In case of success the return value is zero.
/*FUNCTION*/
int file_gethostname(char *pszBuffer,
                     long cbBuffer
  ){
/*noverbatim
The first argument should point to the character buffer, and the second
argument should hold the size of the buffer in bytes.
CUT*/
#ifndef __NO_SOCKETS__
  WIN32SOCKETINIT

  return gethostname(pszBuffer,(int)cbBuffer);
#else
  return 1;
#endif
  }

/*POD
=H file_gethost
@c Get host by name or by address

This function gets the T<struct hostent> entry for the given address.
The address can be given as a FQDN or as an IP octet tuple, like
www.digital.com or 16.193.48.55

Optionally the address may contain a port number separated by : from
the name or the IP number. The port number is simply ignored.

/*FUNCTION*/
int file_gethost(char *pszBuffer,
                 struct hostent *pHost
  ){
/*noverbatim
T<pszBuffer> should hold the name or the address of the target machine.
This buffer is not altered during the function.

T<pHost> should point to a buffer ready to hold the hostent information.

Note that the structure T<hostent> contains pointers outside the structre.
Those pointers are copied verbatim thus they point to the original content
as returned by the underlying socket layer. This means that the values
the T<hostent> structure points to should not be freed, altered and 
the values needed later should be copied as soon as possible into a safe
location before any other socket call is done.
CUT*/
#ifndef __NO_SOCKETS__
  int i,OctetCounter,IPnumber;
  unsigned char addr[4],*s;
  struct hostent *q;
  WIN32SOCKETINIT

  /* decide if the given string is a dd.dd.dd.dd IP number */
  IPnumber = 1; /* assume that it is an IP number when we start */
  addr[0] = 0;
  OctetCounter = 0;
  for( i = 0 ; pszBuffer[i] && pszBuffer[i] != ':' ; i++ ){
     if( pszBuffer[i] == '.' ){
       OctetCounter ++;
       if( OctetCounter > 3 ){
         IPnumber = 0; /* there are too many octets, there should be no more than four */
         break;        /* we will take care of IPng later */
         }
       addr[OctetCounter] = 0;
       continue;
       }
     if( ! isdigit(pszBuffer[i]) ){
       IPnumber = 0;/* if there is a non digit character it can not be IP number */
       while( pszBuffer[i] && pszBuffer[i] != ':' )i++;/* we need the colon by i */
       break;
       }
     addr[OctetCounter] = 10*addr[OctetCounter] + pszBuffer[i] - '0';
     }

  /* if there was nothing telling that this is not an IP number
     and we have found four octets total */
  if( IPnumber && OctetCounter == 3 ){
    q = gethostbyaddr(addr,4,AF_INET);
    if( q == NULL )return 1;
    memcpy(pHost,q,sizeof(struct hostent));
    return 0;
    }else{/* note that we can not modify pszBuffer even temporarily, because
             WindowsNT string constants are real constants and the process is
             not allowed to alter the string constants. */
    if( pszBuffer[i] ){
      s = (unsigned char *)malloc(i+2);
      if( s == NULL )return 1;
      memcpy(s,pszBuffer,i);
      s[i] = (char)0;
      q = gethostbyname(s);
      free(s);
      }else q = gethostbyname(pszBuffer);
    if( q == NULL )return 1;
    memcpy(pHost,q,sizeof(struct hostent));
    return 0;
    }
#else
  return 1;
#endif
  }

/*POD
=H file_tcpconnect
@c Connect a socket to a server:port

This function tries to connect to the remote port of a remote server.
The first argument of the function should be a pointer to T<SOCKET>
variable as defined in T<filesys.h> or in the Windows header files. The
second argument is a string that contains the name of the remote host,
or the IP number of the remote host and the desired port number following
the name separated by a colon. For example T<index.hu:80> tries to connect
to the http port of the server T<index.hu>. You can also write
T<16.192.80.33:80> to get a connection. The function automatically recognizes
IP numbers and host names. The socket is created automatically calling the
system function T<socket>.

If the function successfully connected to the remote server the return value
is zero. Otherwise the return value is the error code.

/*FUNCTION*/
int file_tcpconnect(SOCKET *sClient,
                    char *pszRemoteSocket
  ){
/*noverbatim
CUT*/
#ifndef __NO_SOCKETS__
  unsigned long iaddr,octet;
  unsigned int iPort,IPnumber,OctetCounter,i;
  struct sockaddr_in RemoteAddress;
  struct hostent RemoteMachine;
  static unsigned long pow[4] = { 1, 256 , 256*256, 256*256*256 };
  WIN32SOCKETINIT
  /* decide if the given string is a dd.dd.dd.dd:port IP number */
  IPnumber = 1; /* assume that it is an IP number when we start */
  iaddr = 0;
  octet = 0;
  OctetCounter = 0;
  for( i = 0 ; pszRemoteSocket[i]  ; i++ ){
     if( pszRemoteSocket[i] == '.' || pszRemoteSocket[i] == ':'){
       iaddr = iaddr + octet * pow[OctetCounter];
       octet = 0;
       OctetCounter ++;
       if( OctetCounter > 4 ){
         IPnumber = 0; /* there are too many octets, there should be no more than four */
         break;        /* we will take care of IPng later */
         }
       if( pszRemoteSocket[i] == ':')break;
       continue;
       }
     if( ! isdigit(pszRemoteSocket[i]) ){
       IPnumber = 0;/* if there is a non digit character it can not be IP number */
       while( pszRemoteSocket[i] && pszRemoteSocket[i] != ':' )i++;/* we need the colon by i */
       break;
       }
     octet = 10*octet + pszRemoteSocket[i] - '0';
     }

  while( pszRemoteSocket[i] && pszRemoteSocket[i] != ':' )i++;
  if( pszRemoteSocket[i] != ':' )return 1;
  i++;
  for( iPort = 0 ; pszRemoteSocket[i] ; i++ ){
    if( ! isdigit(pszRemoteSocket[i]) )return 1;
    iPort = 10*iPort + pszRemoteSocket[i] - '0';
    }

  /* if there was nothing telling that this is not an IP number
     and we have found four octets total */
  if( !IPnumber || OctetCounter != 4 ){
    if( err = file_gethost(pszRemoteSocket,&RemoteMachine) )return err;
    memcpy(&iaddr,RemoteMachine.h_addr,4);
    }
#ifdef WIN32
  RemoteAddress.sin_addr.S_un.S_addr = iaddr;
#else
  RemoteAddress.sin_addr.s_addr = iaddr;
#endif
  RemoteAddress.sin_port = htons((unsigned short)iPort);
  RemoteAddress.sin_family = AF_INET;
  *sClient = socket(AF_INET,SOCK_STREAM,0);
  return connect((*sClient),(struct sockaddr *)&RemoteAddress,sizeof(RemoteAddress));
#else
  return 1;
#endif
  }

/*POD
=H file_tcpsend
@c send bytes to remote server via socket

/*FUNCTION*/
int file_tcpsend(SOCKET sClient,
                 char *pszBuffer,
                 long cbBuffer,
                 int iFlags
  ){
/*noverbatim
CUT*/
#ifndef __NO_SOCKETS__
  WIN32SOCKETINIT

  return send(sClient,pszBuffer,(int)cbBuffer,iFlags);
#else
  return 1;
#endif
  }

/*POD
=H file_tcprecv
@c receive bytes from remote server via socket

/*FUNCTION*/
int file_tcprecv(SOCKET sClient,
                 char *pszBuffer,
                 long cbBuffer,
                 int iFlags
  ){
/*noverbatim
CUT*/
#ifndef __NO_SOCKETS__
  WIN32SOCKETINIT

  return recv(sClient,pszBuffer,(int)cbBuffer,iFlags);
#else
  return 1;
#endif
  }

/*POD
=H file_tcpclose
@c close a tcp connection

/*FUNCTION*/
int file_tcpclose(SOCKET sClient
  ){
/*noverbatim
CUT*/
#ifndef __NO_SOCKETS__
  WIN32SOCKETINIT

  if( shutdown(sClient,SHUT_RDWR) )return 1;
#ifdef WIN32
  closesocket(sClient);
#else
  close(sClient);
#endif
  return 0;
#else
  return 1;
#endif
  }

/*POD
=H file_killproc
@c Kill a process


This function kills a process identified by the process ID (PID).

If the process is killed successfully the return value is zero, otherwise
a positive value.
/*FUNCTION*/
int file_killproc(long pid
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  HANDLE hProcess;

  hProcess = OpenProcess(PROCESS_TERMINATE,0,(DWORD)pid);
  if( hProcess == NULL )return 1;
  if( TerminateProcess(hProcess,1) )return 0;
  return 1;
#elif defined(__MACOS__)
  return 1;
#else
  return kill( (pid_t)pid,9);
#endif
  }

/* Eric Young.
 * This version of crypt has been developed from my MIT compatable
 * DES library.
 * The library is available at pub/DES at ftp.psy.uq.oz.au
 * eay@psych.psy.uq.oz.au
 */

typedef unsigned char des_cblock[8];

typedef struct des_ks_struct
	{
	union	{
		des_cblock _;
		/* make sure things are correct size on machines with
		 * 8 byte longs */
		unsigned long pad[2];
		} ks;
#define _	ks._
	} des_key_schedule[16];

#define DES_KEY_SZ 	(sizeof(des_cblock))
#define DES_ENCRYPT	1
#define DES_DECRYPT	0

#define ITERATIONS 16
#define HALF_ITERATIONS 8

#define c2l(c,l)	(l =((unsigned long)(*((c)++)))    , \
			 l|=((unsigned long)(*((c)++)))<< 8, \
			 l|=((unsigned long)(*((c)++)))<<16, \
			 l|=((unsigned long)(*((c)++)))<<24)

#define l2c(l,c)	(*((c)++)=(unsigned char)(((l)    )&0xff), \
			 *((c)++)=(unsigned char)(((l)>> 8)&0xff), \
			 *((c)++)=(unsigned char)(((l)>>16)&0xff), \
			 *((c)++)=(unsigned char)(((l)>>24)&0xff))

static unsigned long SPtrans[8][64]={
/* nibble 0 */
0x00820200, 0x00020000, 0x80800000, 0x80820200,
0x00800000, 0x80020200, 0x80020000, 0x80800000,
0x80020200, 0x00820200, 0x00820000, 0x80000200,
0x80800200, 0x00800000, 0x00000000, 0x80020000,
0x00020000, 0x80000000, 0x00800200, 0x00020200,
0x80820200, 0x00820000, 0x80000200, 0x00800200,
0x80000000, 0x00000200, 0x00020200, 0x80820000,
0x00000200, 0x80800200, 0x80820000, 0x00000000,
0x00000000, 0x80820200, 0x00800200, 0x80020000,
0x00820200, 0x00020000, 0x80000200, 0x00800200,
0x80820000, 0x00000200, 0x00020200, 0x80800000,
0x80020200, 0x80000000, 0x80800000, 0x00820000,
0x80820200, 0x00020200, 0x00820000, 0x80800200,
0x00800000, 0x80000200, 0x80020000, 0x00000000,
0x00020000, 0x00800000, 0x80800200, 0x00820200,
0x80000000, 0x80820000, 0x00000200, 0x80020200,
/* nibble 1 */
0x10042004, 0x00000000, 0x00042000, 0x10040000,
0x10000004, 0x00002004, 0x10002000, 0x00042000,
0x00002000, 0x10040004, 0x00000004, 0x10002000,
0x00040004, 0x10042000, 0x10040000, 0x00000004,
0x00040000, 0x10002004, 0x10040004, 0x00002000,
0x00042004, 0x10000000, 0x00000000, 0x00040004,
0x10002004, 0x00042004, 0x10042000, 0x10000004,
0x10000000, 0x00040000, 0x00002004, 0x10042004,
0x00040004, 0x10042000, 0x10002000, 0x00042004,
0x10042004, 0x00040004, 0x10000004, 0x00000000,
0x10000000, 0x00002004, 0x00040000, 0x10040004,
0x00002000, 0x10000000, 0x00042004, 0x10002004,
0x10042000, 0x00002000, 0x00000000, 0x10000004,
0x00000004, 0x10042004, 0x00042000, 0x10040000,
0x10040004, 0x00040000, 0x00002004, 0x10002000,
0x10002004, 0x00000004, 0x10040000, 0x00042000,
/* nibble 2 */
0x41000000, 0x01010040, 0x00000040, 0x41000040,
0x40010000, 0x01000000, 0x41000040, 0x00010040,
0x01000040, 0x00010000, 0x01010000, 0x40000000,
0x41010040, 0x40000040, 0x40000000, 0x41010000,
0x00000000, 0x40010000, 0x01010040, 0x00000040,
0x40000040, 0x41010040, 0x00010000, 0x41000000,
0x41010000, 0x01000040, 0x40010040, 0x01010000,
0x00010040, 0x00000000, 0x01000000, 0x40010040,
0x01010040, 0x00000040, 0x40000000, 0x00010000,
0x40000040, 0x40010000, 0x01010000, 0x41000040,
0x00000000, 0x01010040, 0x00010040, 0x41010000,
0x40010000, 0x01000000, 0x41010040, 0x40000000,
0x40010040, 0x41000000, 0x01000000, 0x41010040,
0x00010000, 0x01000040, 0x41000040, 0x00010040,
0x01000040, 0x00000000, 0x41010000, 0x40000040,
0x41000000, 0x40010040, 0x00000040, 0x01010000,
/* nibble 3 */
0x00100402, 0x04000400, 0x00000002, 0x04100402,
0x00000000, 0x04100000, 0x04000402, 0x00100002,
0x04100400, 0x04000002, 0x04000000, 0x00000402,
0x04000002, 0x00100402, 0x00100000, 0x04000000,
0x04100002, 0x00100400, 0x00000400, 0x00000002,
0x00100400, 0x04000402, 0x04100000, 0x00000400,
0x00000402, 0x00000000, 0x00100002, 0x04100400,
0x04000400, 0x04100002, 0x04100402, 0x00100000,
0x04100002, 0x00000402, 0x00100000, 0x04000002,
0x00100400, 0x04000400, 0x00000002, 0x04100000,
0x04000402, 0x00000000, 0x00000400, 0x00100002,
0x00000000, 0x04100002, 0x04100400, 0x00000400,
0x04000000, 0x04100402, 0x00100402, 0x00100000,
0x04100402, 0x00000002, 0x04000400, 0x00100402,
0x00100002, 0x00100400, 0x04100000, 0x04000402,
0x00000402, 0x04000000, 0x04000002, 0x04100400,
/* nibble 4 */
0x02000000, 0x00004000, 0x00000100, 0x02004108,
0x02004008, 0x02000100, 0x00004108, 0x02004000,
0x00004000, 0x00000008, 0x02000008, 0x00004100,
0x02000108, 0x02004008, 0x02004100, 0x00000000,
0x00004100, 0x02000000, 0x00004008, 0x00000108,
0x02000100, 0x00004108, 0x00000000, 0x02000008,
0x00000008, 0x02000108, 0x02004108, 0x00004008,
0x02004000, 0x00000100, 0x00000108, 0x02004100,
0x02004100, 0x02000108, 0x00004008, 0x02004000,
0x00004000, 0x00000008, 0x02000008, 0x02000100,
0x02000000, 0x00004100, 0x02004108, 0x00000000,
0x00004108, 0x02000000, 0x00000100, 0x00004008,
0x02000108, 0x00000100, 0x00000000, 0x02004108,
0x02004008, 0x02004100, 0x00000108, 0x00004000,
0x00004100, 0x02004008, 0x02000100, 0x00000108,
0x00000008, 0x00004108, 0x02004000, 0x02000008,
/* nibble 5 */
0x20000010, 0x00080010, 0x00000000, 0x20080800,
0x00080010, 0x00000800, 0x20000810, 0x00080000,
0x00000810, 0x20080810, 0x00080800, 0x20000000,
0x20000800, 0x20000010, 0x20080000, 0x00080810,
0x00080000, 0x20000810, 0x20080010, 0x00000000,
0x00000800, 0x00000010, 0x20080800, 0x20080010,
0x20080810, 0x20080000, 0x20000000, 0x00000810,
0x00000010, 0x00080800, 0x00080810, 0x20000800,
0x00000810, 0x20000000, 0x20000800, 0x00080810,
0x20080800, 0x00080010, 0x00000000, 0x20000800,
0x20000000, 0x00000800, 0x20080010, 0x00080000,
0x00080010, 0x20080810, 0x00080800, 0x00000010,
0x20080810, 0x00080800, 0x00080000, 0x20000810,
0x20000010, 0x20080000, 0x00080810, 0x00000000,
0x00000800, 0x20000010, 0x20000810, 0x20080800,
0x20080000, 0x00000810, 0x00000010, 0x20080010,
/* nibble 6 */
0x00001000, 0x00000080, 0x00400080, 0x00400001,
0x00401081, 0x00001001, 0x00001080, 0x00000000,
0x00400000, 0x00400081, 0x00000081, 0x00401000,
0x00000001, 0x00401080, 0x00401000, 0x00000081,
0x00400081, 0x00001000, 0x00001001, 0x00401081,
0x00000000, 0x00400080, 0x00400001, 0x00001080,
0x00401001, 0x00001081, 0x00401080, 0x00000001,
0x00001081, 0x00401001, 0x00000080, 0x00400000,
0x00001081, 0x00401000, 0x00401001, 0x00000081,
0x00001000, 0x00000080, 0x00400000, 0x00401001,
0x00400081, 0x00001081, 0x00001080, 0x00000000,
0x00000080, 0x00400001, 0x00000001, 0x00400080,
0x00000000, 0x00400081, 0x00400080, 0x00001080,
0x00000081, 0x00001000, 0x00401081, 0x00400000,
0x00401080, 0x00000001, 0x00001001, 0x00401081,
0x00400001, 0x00401080, 0x00401000, 0x00001001,
/* nibble 7 */
0x08200020, 0x08208000, 0x00008020, 0x00000000,
0x08008000, 0x00200020, 0x08200000, 0x08208020,
0x00000020, 0x08000000, 0x00208000, 0x00008020,
0x00208020, 0x08008020, 0x08000020, 0x08200000,
0x00008000, 0x00208020, 0x00200020, 0x08008000,
0x08208020, 0x08000020, 0x00000000, 0x00208000,
0x08000000, 0x00200000, 0x08008020, 0x08200020,
0x00200000, 0x00008000, 0x08208000, 0x00000020,
0x00200000, 0x00008000, 0x08000020, 0x08208020,
0x00008020, 0x08000000, 0x00000000, 0x00208000,
0x08200020, 0x08008020, 0x08008000, 0x00200020,
0x08208000, 0x00000020, 0x00200020, 0x08008000,
0x08208020, 0x00200000, 0x08200000, 0x08000020,
0x00208000, 0x00008020, 0x08008020, 0x08200000,
0x00000020, 0x08208000, 0x00208020, 0x00000000,
0x08000000, 0x08200020, 0x00008000, 0x00208020};
static unsigned long skb[8][64]={
/* for C bits (numbered as per FIPS 46) 1 2 3 4 5 6 */
0x00000000,0x00000010,0x20000000,0x20000010,
0x00010000,0x00010010,0x20010000,0x20010010,
0x00000800,0x00000810,0x20000800,0x20000810,
0x00010800,0x00010810,0x20010800,0x20010810,
0x00000020,0x00000030,0x20000020,0x20000030,
0x00010020,0x00010030,0x20010020,0x20010030,
0x00000820,0x00000830,0x20000820,0x20000830,
0x00010820,0x00010830,0x20010820,0x20010830,
0x00080000,0x00080010,0x20080000,0x20080010,
0x00090000,0x00090010,0x20090000,0x20090010,
0x00080800,0x00080810,0x20080800,0x20080810,
0x00090800,0x00090810,0x20090800,0x20090810,
0x00080020,0x00080030,0x20080020,0x20080030,
0x00090020,0x00090030,0x20090020,0x20090030,
0x00080820,0x00080830,0x20080820,0x20080830,
0x00090820,0x00090830,0x20090820,0x20090830,
/* for C bits (numbered as per FIPS 46) 7 8 10 11 12 13 */
0x00000000,0x02000000,0x00002000,0x02002000,
0x00200000,0x02200000,0x00202000,0x02202000,
0x00000004,0x02000004,0x00002004,0x02002004,
0x00200004,0x02200004,0x00202004,0x02202004,
0x00000400,0x02000400,0x00002400,0x02002400,
0x00200400,0x02200400,0x00202400,0x02202400,
0x00000404,0x02000404,0x00002404,0x02002404,
0x00200404,0x02200404,0x00202404,0x02202404,
0x10000000,0x12000000,0x10002000,0x12002000,
0x10200000,0x12200000,0x10202000,0x12202000,
0x10000004,0x12000004,0x10002004,0x12002004,
0x10200004,0x12200004,0x10202004,0x12202004,
0x10000400,0x12000400,0x10002400,0x12002400,
0x10200400,0x12200400,0x10202400,0x12202400,
0x10000404,0x12000404,0x10002404,0x12002404,
0x10200404,0x12200404,0x10202404,0x12202404,
/* for C bits (numbered as per FIPS 46) 14 15 16 17 19 20 */
0x00000000,0x00000001,0x00040000,0x00040001,
0x01000000,0x01000001,0x01040000,0x01040001,
0x00000002,0x00000003,0x00040002,0x00040003,
0x01000002,0x01000003,0x01040002,0x01040003,
0x00000200,0x00000201,0x00040200,0x00040201,
0x01000200,0x01000201,0x01040200,0x01040201,
0x00000202,0x00000203,0x00040202,0x00040203,
0x01000202,0x01000203,0x01040202,0x01040203,
0x08000000,0x08000001,0x08040000,0x08040001,
0x09000000,0x09000001,0x09040000,0x09040001,
0x08000002,0x08000003,0x08040002,0x08040003,
0x09000002,0x09000003,0x09040002,0x09040003,
0x08000200,0x08000201,0x08040200,0x08040201,
0x09000200,0x09000201,0x09040200,0x09040201,
0x08000202,0x08000203,0x08040202,0x08040203,
0x09000202,0x09000203,0x09040202,0x09040203,
/* for C bits (numbered as per FIPS 46) 21 23 24 26 27 28 */
0x00000000,0x00100000,0x00000100,0x00100100,
0x00000008,0x00100008,0x00000108,0x00100108,
0x00001000,0x00101000,0x00001100,0x00101100,
0x00001008,0x00101008,0x00001108,0x00101108,
0x04000000,0x04100000,0x04000100,0x04100100,
0x04000008,0x04100008,0x04000108,0x04100108,
0x04001000,0x04101000,0x04001100,0x04101100,
0x04001008,0x04101008,0x04001108,0x04101108,
0x00020000,0x00120000,0x00020100,0x00120100,
0x00020008,0x00120008,0x00020108,0x00120108,
0x00021000,0x00121000,0x00021100,0x00121100,
0x00021008,0x00121008,0x00021108,0x00121108,
0x04020000,0x04120000,0x04020100,0x04120100,
0x04020008,0x04120008,0x04020108,0x04120108,
0x04021000,0x04121000,0x04021100,0x04121100,
0x04021008,0x04121008,0x04021108,0x04121108,
/* for D bits (numbered as per FIPS 46) 1 2 3 4 5 6 */
0x00000000,0x10000000,0x00010000,0x10010000,
0x00000004,0x10000004,0x00010004,0x10010004,
0x20000000,0x30000000,0x20010000,0x30010000,
0x20000004,0x30000004,0x20010004,0x30010004,
0x00100000,0x10100000,0x00110000,0x10110000,
0x00100004,0x10100004,0x00110004,0x10110004,
0x20100000,0x30100000,0x20110000,0x30110000,
0x20100004,0x30100004,0x20110004,0x30110004,
0x00001000,0x10001000,0x00011000,0x10011000,
0x00001004,0x10001004,0x00011004,0x10011004,
0x20001000,0x30001000,0x20011000,0x30011000,
0x20001004,0x30001004,0x20011004,0x30011004,
0x00101000,0x10101000,0x00111000,0x10111000,
0x00101004,0x10101004,0x00111004,0x10111004,
0x20101000,0x30101000,0x20111000,0x30111000,
0x20101004,0x30101004,0x20111004,0x30111004,
/* for D bits (numbered as per FIPS 46) 8 9 11 12 13 14 */
0x00000000,0x08000000,0x00000008,0x08000008,
0x00000400,0x08000400,0x00000408,0x08000408,
0x00020000,0x08020000,0x00020008,0x08020008,
0x00020400,0x08020400,0x00020408,0x08020408,
0x00000001,0x08000001,0x00000009,0x08000009,
0x00000401,0x08000401,0x00000409,0x08000409,
0x00020001,0x08020001,0x00020009,0x08020009,
0x00020401,0x08020401,0x00020409,0x08020409,
0x02000000,0x0A000000,0x02000008,0x0A000008,
0x02000400,0x0A000400,0x02000408,0x0A000408,
0x02020000,0x0A020000,0x02020008,0x0A020008,
0x02020400,0x0A020400,0x02020408,0x0A020408,
0x02000001,0x0A000001,0x02000009,0x0A000009,
0x02000401,0x0A000401,0x02000409,0x0A000409,
0x02020001,0x0A020001,0x02020009,0x0A020009,
0x02020401,0x0A020401,0x02020409,0x0A020409,
/* for D bits (numbered as per FIPS 46) 16 17 18 19 20 21 */
0x00000000,0x00000100,0x00080000,0x00080100,
0x01000000,0x01000100,0x01080000,0x01080100,
0x00000010,0x00000110,0x00080010,0x00080110,
0x01000010,0x01000110,0x01080010,0x01080110,
0x00200000,0x00200100,0x00280000,0x00280100,
0x01200000,0x01200100,0x01280000,0x01280100,
0x00200010,0x00200110,0x00280010,0x00280110,
0x01200010,0x01200110,0x01280010,0x01280110,
0x00000200,0x00000300,0x00080200,0x00080300,
0x01000200,0x01000300,0x01080200,0x01080300,
0x00000210,0x00000310,0x00080210,0x00080310,
0x01000210,0x01000310,0x01080210,0x01080310,
0x00200200,0x00200300,0x00280200,0x00280300,
0x01200200,0x01200300,0x01280200,0x01280300,
0x00200210,0x00200310,0x00280210,0x00280310,
0x01200210,0x01200310,0x01280210,0x01280310,
/* for D bits (numbered as per FIPS 46) 22 23 24 25 27 28 */
0x00000000,0x04000000,0x00040000,0x04040000,
0x00000002,0x04000002,0x00040002,0x04040002,
0x00002000,0x04002000,0x00042000,0x04042000,
0x00002002,0x04002002,0x00042002,0x04042002,
0x00000020,0x04000020,0x00040020,0x04040020,
0x00000022,0x04000022,0x00040022,0x04040022,
0x00002020,0x04002020,0x00042020,0x04042020,
0x00002022,0x04002022,0x00042022,0x04042022,
0x00000800,0x04000800,0x00040800,0x04040800,
0x00000802,0x04000802,0x00040802,0x04040802,
0x00002800,0x04002800,0x00042800,0x04042800,
0x00002802,0x04002802,0x00042802,0x04042802,
0x00000820,0x04000820,0x00040820,0x04040820,
0x00000822,0x04000822,0x00040822,0x04040822,
0x00002820,0x04002820,0x00042820,0x04042820,
0x00002822,0x04002822,0x00042822,0x04042822,
};

/* See ecb_encrypt.c for a pseudo description of these macros. */
#define PERM_OP(a,b,t,n,m) ((t)=((((a)>>(n))^(b))&(m)),\
	(b)^=(t),\
	(a)^=((t)<<(n)))

#define HPERM_OP(a,t,n,m) ((t)=((((a)<<(16-(n)))^(a))&(m)),\
	(a)=(a)^(t)^(t>>(16-(n))))\

static char shifts2[16]={0,0,1,1,1,1,1,1,0,1,1,1,1,1,1,0};

static int body(
	unsigned long *out0,
	unsigned long *out1,
	des_key_schedule ks,
	unsigned long Eswap0,
	unsigned long Eswap1);

static int
des_set_key(des_cblock *key, des_key_schedule schedule)
	{
	register unsigned long c,d,t,s;
	register unsigned char *in;
	register unsigned long *k;
	register int i;

	k=(unsigned long *)schedule;
	in=(unsigned char *)key;

	c2l(in,c);
	c2l(in,d);

	/* I now do it in 47 simple operations :-)
	 * Thanks to John Fletcher (john_fletcher@lccmail.ocf.llnl.gov)
	 * for the inspiration. :-) */
	PERM_OP (d,c,t,4,0x0f0f0f0f);
	HPERM_OP(c,t,-2,0xcccc0000);
	HPERM_OP(d,t,-2,0xcccc0000);
	PERM_OP (d,c,t,1,0x55555555);
	PERM_OP (c,d,t,8,0x00ff00ff);
	PERM_OP (d,c,t,1,0x55555555);
	d=	(((d&0x000000ff)<<16)| (d&0x0000ff00)     |
		 ((d&0x00ff0000)>>16)|((c&0xf0000000)>>4));
	c&=0x0fffffff;

	for (i=0; i<ITERATIONS; i++)
		{
		if (shifts2[i])
			{ c=((c>>2)|(c<<26)); d=((d>>2)|(d<<26)); }
		else
			{ c=((c>>1)|(c<<27)); d=((d>>1)|(d<<27)); }
		c&=0x0fffffff;
		d&=0x0fffffff;
		/* could be a few less shifts but I am to lazy at this
		 * point in time to investigate */
		s=	skb[0][ (c    )&0x3f                ]|
			skb[1][((c>> 6)&0x03)|((c>> 7)&0x3c)]|
			skb[2][((c>>13)&0x0f)|((c>>14)&0x30)]|
			skb[3][((c>>20)&0x01)|((c>>21)&0x06) |
			                      ((c>>22)&0x38)];
		t=	skb[4][ (d    )&0x3f                ]|
			skb[5][((d>> 7)&0x03)|((d>> 8)&0x3c)]|
			skb[6][ (d>>15)&0x3f                ]|
			skb[7][((d>>21)&0x0f)|((d>>22)&0x30)];

		/* table contained 0213 4657 */
		*(k++)=((t<<16)|(s&0x0000ffff))&0xffffffff;
		s=     ((s>>16)|(t&0xffff0000));
		
		s=(s<<4)|(s>>28);
		*(k++)=s&0xffffffff;
		}
	return(0);
	}

/******************************************************************
 * modified stuff for crypt.
 ******************************************************************/

/* The changes to this macro may help or hinder, depending on the
 * compiler and the achitecture.  gcc2 always seems to do well :-). 
 * Inspired by Dana How <how@isl.stanford.edu>
 * DO NOT use the alternative version on machines with 8 byte longs.
 */
#ifdef ALT_ECB
#define D_ENCRYPT(L,R,S) \
	v=(R^(R>>16)); \
	u=(v&E0); \
	v=(v&E1); \
	u=((u^(u<<16))^R^s[S  ])<<2; \
	t=(v^(v<<16))^R^s[S+1]; \
	t=(t>>2)|(t<<30); \
	L^= \
	*(unsigned long *)(des_SP+0x0100+((t    )&0xfc))+ \
	*(unsigned long *)(des_SP+0x0300+((t>> 8)&0xfc))+ \
	*(unsigned long *)(des_SP+0x0500+((t>>16)&0xfc))+ \
	*(unsigned long *)(des_SP+0x0700+((t>>24)&0xfc))+ \
	*(unsigned long *)(des_SP+       ((u    )&0xfc))+ \
  	*(unsigned long *)(des_SP+0x0200+((u>> 8)&0xfc))+ \
  	*(unsigned long *)(des_SP+0x0400+((u>>16)&0xfc))+ \
 	*(unsigned long *)(des_SP+0x0600+((u>>24)&0xfc));
#else /* original version */
#define D_ENCRYPT(L,R,S)	\
	v=(R^(R>>16)); \
	u=(v&E0); \
	v=(v&E1); \
	u=(u^(u<<16))^R^s[S  ]; \
	t=(v^(v<<16))^R^s[S+1]; \
	t=(t>>4)|(t<<28); \
	L^=	SPtrans[1][(t    )&0x3f]| \
		SPtrans[3][(t>> 8)&0x3f]| \
		SPtrans[5][(t>>16)&0x3f]| \
		SPtrans[7][(t>>24)&0x3f]| \
		SPtrans[0][(u    )&0x3f]| \
		SPtrans[2][(u>> 8)&0x3f]| \
		SPtrans[4][(u>>16)&0x3f]| \
		SPtrans[6][(u>>24)&0x3f];
#endif

unsigned char con_salt[128]={
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,
0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
0x0A,0x0B,0x05,0x06,0x07,0x08,0x09,0x0A,
0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,
0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,
0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,
0x23,0x24,0x25,0x20,0x21,0x22,0x23,0x24,
0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,
0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,
0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,
0x3D,0x3E,0x3F,0x00,0x00,0x00,0x00,0x00,
};

unsigned char cov_2char[64]={
0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,
0x36,0x37,0x38,0x39,0x41,0x42,0x43,0x44,
0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,
0x4D,0x4E,0x4F,0x50,0x51,0x52,0x53,0x54,
0x55,0x56,0x57,0x58,0x59,0x5A,0x61,0x62,
0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,
0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,
0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A
};

/*POD
=H file_fcrypt
@c Calculate encrypted password

This function implements the password encryption algorithm
using the DES function. The first argument is the clear text
password, the second argument is the two character salt value.
This need not be zero terminated. The third argument should
point to a 13 characters char array to get the encoded
password. T<buff[13]> will contain the terminating zchar upon return.

/*FUNCTION*/
char *file_fcrypt(char *buf, char *salt, char *buff
  ){
/*noverbatim
CUT*/
	unsigned int i,j,x,y;
	unsigned long Eswap0=0,Eswap1=0;
	unsigned long out[2],ll;
	des_cblock key;
	des_key_schedule ks;
	unsigned char bb[9];
	unsigned char *b=bb;
	unsigned char c,u;

	/* eay 25/08/92
	 * If you call crypt("pwd","*") as often happens when you
	 * have * as the pwd field in /etc/passwd, the function
	 * returns *\0XXXXXXXXX
	 * The \0 makes the string look like * so the pwd "*" would
	 * crypt to "*".  This was found when replacing the crypt in
	 * our shared libraries.  People found that the disbled
	 * accounts effectively had no passwd :-(. */
	x=buff[0]=((salt[0] == '\0')?'A':salt[0]);
	Eswap0=con_salt[x];
	x=buff[1]=((salt[1] == '\0')?'A':salt[1]);
	Eswap1=con_salt[x]<<4;

	for (i=0; i<8; i++)
		{
		c= *(buf++);
		if (!c) break;
		key[i]=(c<<1);
		}
	for (; i<8; i++)
		key[i]=0;

	des_set_key((des_cblock *)(key),ks);
	body(&out[0],&out[1],ks,Eswap0,Eswap1);

	ll=out[0]; l2c(ll,b);
	ll=out[1]; l2c(ll,b);
	y=0;
	u=0x80;
	bb[8]=0;
	for (i=2; i<13; i++)
		{
		c=0;
		for (j=0; j<6; j++)
			{
			c<<=1;
			if (bb[y] & u) c|=1;
			u>>=1;
			if (!u)
				{
				y++;
				u=0x80;
				}
			}
		buff[i]=cov_2char[c];
		}
	buff[13]='\0';
	return buff;
	}

static int 
body(	unsigned long *out0,
	unsigned long *out1,
	des_key_schedule ks,
	unsigned long Eswap0,
	unsigned long Eswap1)
	{
	register unsigned long l,r,t,u,v;
#ifdef ALT_ECB
	register unsigned char *des_SP=(unsigned char *)SPtrans;
#endif
	register unsigned long *s;
	register int i,j;
	register unsigned long E0,E1;

	l=0;
	r=0;

	s=(unsigned long *)ks;
	E0=Eswap0;
	E1=Eswap1;

	for (j=0; j<25; j++)
		{
		for (i=0; i<(ITERATIONS*2); i+=4)
			{
			D_ENCRYPT(l,r,  i);	/*  1 */
			D_ENCRYPT(r,l,  i+2);	/*  2 */
			}
		t=l;
		l=r;
		r=t;
		}
	t=r;
	r=(l>>1)|(l<<31);
	l=(t>>1)|(t<<31);
	/* clear the top bits on machines with 8byte longs */
	l&=0xffffffff;
	r&=0xffffffff;

	PERM_OP(r,l,t, 1,0x55555555);
	PERM_OP(l,r,t, 8,0x00ff00ff);
	PERM_OP(r,l,t, 2,0x33333333);
	PERM_OP(l,r,t,16,0x0000ffff);
	PERM_OP(r,l,t, 4,0x0f0f0f0f);

	*out0=l;
	*out1=r;
	return(0);
	}

/*POD
=H file_CreateProcess
@c Run a new program

This function creates a new process using the argument as command line.
The function does NOT wait the new process to be finished but returns
the pid of the new process.

If the new process can not be started the return value is zero.

The success of the new process however can not be determined by the
return value. On UNIX this value is generated by the fork system call
and it still may fail to replace the executeable image calling T<exevp>.
By that time the new program creation is already in the newprocess and
is not able to send back any error information to the caller.

The caller of this function should also check other outputs of the
created process that of the pid is returned. For example if the
T<execv> call failed the process exit code is T<1>. This is usually an
error information of a process.

/*FUNCTION*/
long file_CreateProcess(char *pszCommandLine
  ){
/*noverbatim
CUT*/
#ifdef WIN32
  STARTUPINFO SupInfo;
  PROCESS_INFORMATION ProcInfo;

  SupInfo.cb = sizeof(SupInfo);
  SupInfo.lpReserved = NULL;
  SupInfo.lpDesktop = NULL;
  SupInfo.lpTitle = NULL;
  SupInfo.dwFlags = 0;
  SupInfo.cbReserved2 = 0;
  SupInfo.lpReserved2 = NULL;

  if( ! CreateProcess(NULL,           /* application name */
                      pszCommandLine, /* command line */
                      NULL,           /* process security attributes */
                      NULL,           /* thread security attributes */
                      0,              /* no handle inheritance */
                      NORMAL_PRIORITY_CLASS,
                      NULL,           /* environment is same as of caller */
                      NULL,           /* curdir is same as caller */
                      &SupInfo,
                      &ProcInfo ) )return 0;
  CloseHandle(ProcInfo.hProcess);
  return ProcInfo.dwProcessId;
#elif defined(__MACOS__)
  /* Macintosh doesn't support starting a program with command line parameters */
  return 0;
#else
  char *pszMyCommandLine;
  char **argv;
  long i,argc;
  int ThePreviousCharacterWasSpace;

  if( i=fork() )return i;

  /* calculate the size of the result array with quoting */
  argc = 1;
  for( i = 0 ; pszCommandLine[i] ; i++ ){
     if( pszCommandLine[i] == '"' ){
       i ++;
       while( ( pszCommandLine[i] ) && ( pszCommandLine[i] != '"' ) ) i++;
       if( pszCommandLine[i] == '"' ) i++;
       }
     if( pszCommandLine[i] == ' ' ) {
       argc++;
       i++;
       while( ( pszCommandLine[i] ) && ( pszCommandLine[i] == ' ' ) ) i++;
       }
    }

  pszMyCommandLine = (char *)malloc(i+1);
  if( pszMyCommandLine == NULL )return 0;
  memcpy(pszMyCommandLine,pszCommandLine,i+1);
  argv = (char **)malloc((argc+1)*sizeof(char *));
  if( argv == NULL ){
    free(pszMyCommandLine);
    return 0;
    }

/* generate result array with quoting */
  ThePreviousCharacterWasSpace = 1;
  argc = 0;
  for( i = 0 ; pszMyCommandLine[i] ; i++ ){
     if( pszMyCommandLine[i] == '"' ) {
       i++;
       if( ThePreviousCharacterWasSpace ){
         ThePreviousCharacterWasSpace = 0;
         argv[argc++] = pszMyCommandLine+i;
         }
       while( ( pszMyCommandLine[i] ) && ( pszMyCommandLine[i] != '"' ) ) i++;
       if( pszMyCommandLine[i] == '"' ) pszMyCommandLine[i] = (char)0;
     }
     if( ThePreviousCharacterWasSpace ){
       ThePreviousCharacterWasSpace = 0;
       argv[argc++] = pszMyCommandLine+i;
       }
     if( pszMyCommandLine[i] == ' ' ){
       ThePreviousCharacterWasSpace = 1;
       pszMyCommandLine[i] = (char)0;
       }
     }
  argv[argc] = NULL;

  execvp(argv[0],argv); /* If this returns, we should exit the forked process. */
  exit(1);
#endif
  }

/*
TO_HEADER:
#define FILESYSE_SUCCESS    0
#define FILESYSE_NOTSTARTED 1
#define FILESYSE_TIMEOUT    2
#define FILESYSE_NOCODE     3
*/

/*POD
=H file_CreateProcessEx
@c Run a new program and wait for it

This function starts a new process and starts to wait for the process.
The caller can specify a timeout period in seconds until the function
waits.

When the process terminates or the timeout period is over the function returns.
/*FUNCTION*/
int file_CreateProcessEx(char *pszCommandLine,
                          long lTimeOut,
                          unsigned long *plPid,
                          unsigned long *plExitCode
  ){
/*noverbatim
Arguments:
=itemize
=item T<pszCommandLine> the command to execute
=item T<lTimeOut> the maximum number of seconds to wait for the process to finish. If this is zero the
function will not wait for the process. If the value is T<-1> the function wait without limit until the created
process finishes.
=item T<plPid> pointer to variable where the PID of the new process is placed.
This parameter can be T<NULL>. If the function returns after the new process has terminated this
value is more or less useless. However this parameter can be used to kill processes that reach the
timeout period and do not terminate.
=item T<plExitCode> pointer to a variable where the exit code of the new process is placed. If the
process is still running when the function returns this parameter is unaltered.
=noitemize

The return value indicates the success of the execution of the new process:

=itemize
=item T<FILESYSE_SUCCESS> The process was started and terminated within the specified timeout period.
=item T<FILESYSE_NOTSTARTED> The function could not start the new process. (not used under UNIX)
=item T<FILESYSE_TIMEOUT> The process was started but did not finish during the timeout period.
=item T<FILESYSE_NOCODE> The process was started and finished within the timeout period but
it was not possible to retrieve the exit code.
=noitemize

Note that the behaviour of this function is slightly different on Windows NT and on UNIX. On Windows NT
the function will return T<FILESYSE_NOTSTARTED> when the new process can not be started. Under UNIX
the process performs a T<fork()> and then an T<execv>. The T<fork()> does not return an error value. When the
T<execvp> fails it is already in the new process and can not return an error code. It exists using the
exit code 1. This may not be distinguished from the program started and returning an exit code 1.

CUT*/
#ifdef WIN32
  STARTUPINFO SupInfo;
  PROCESS_INFORMATION ProcInfo;
  int iError;
  unsigned long ulExitCode;

  SupInfo.cb = sizeof(SupInfo);
  SupInfo.lpReserved = NULL;
  SupInfo.lpDesktop = NULL;
  SupInfo.lpTitle = NULL;
  SupInfo.dwFlags = 0;
  SupInfo.cbReserved2 = 0;
  SupInfo.lpReserved2 = NULL;

  if( ! CreateProcess(NULL,           /* application name */
                      pszCommandLine, /* command line */
                      NULL,           /* process security attributes */
                      NULL,           /* thread security attributes */
                      0,              /* no handle inheritance */
                      NORMAL_PRIORITY_CLASS,
                      NULL,           /* environment is same as of caller */
                      NULL,           /* curdir is same as caller */
                      &SupInfo,
                      &ProcInfo ) )return FILESYSE_NOTSTARTED;

  if( plPid )*plPid = ProcInfo.dwProcessId;
  if( WaitForSingleObject(ProcInfo.hProcess, lTimeOut == -1 ? INFINITE : lTimeOut*1000) == WAIT_TIMEOUT ){
    CloseHandle(ProcInfo.hProcess);
    return FILESYSE_TIMEOUT;
    }
  iError = ! GetExitCodeProcess(ProcInfo.hProcess,&ulExitCode);
  if( plExitCode )*plExitCode = ulExitCode;
  CloseHandle(ProcInfo.hProcess);
  return iError ? FILESYSE_NOCODE : FILESYSE_SUCCESS;
#elif defined(__MACOS__)
  /* Macintosh doesn't support starting a program with command line parameters */
  return 0;
#else
  char *pszMyCommandLine;
  char **argv;
  long i,argc;
  int ThePreviousCharacterWasSpace;
  int status;

  if( i=fork() ){
    if( plPid )*plPid = i;
    while( lTimeOut-- ){
      sleep(1);
      waitpid(i,&status,WNOHANG);
      if( WIFEXITED(status) ){
        *plExitCode = WEXITSTATUS(status);
         return FILESYSE_SUCCESS;
         }
      }
    return FILESYSE_TIMEOUT;
    }

/* calculate the size of the result array with quoting */
  argc = 1;
  for( i = 0 ; pszCommandLine[i] ; i++ ){
     if( pszCommandLine[i] == '"' ){
       i ++;
       while( ( pszCommandLine[i] ) && ( pszCommandLine[i] != '"' ) ) i++;
       if( pszCommandLine[i] == '"' ) i++;
       }
     if( pszCommandLine[i] == ' ' ) {
     		argc++;
     		i++;
     		while( ( pszCommandLine[i] ) && ( pszCommandLine[i] == ' ' ) ) i++;
       }
    }

  pszMyCommandLine = (char *)malloc(i+1);
  if( pszMyCommandLine == NULL )return 0;
  memcpy(pszMyCommandLine,pszCommandLine,i+1);
  argv = (char **)malloc((argc+1)*sizeof(char *));
  if( argv == NULL ){
    free(pszMyCommandLine);
    return 0;
    }

/* generate result array with quoting */
  ThePreviousCharacterWasSpace = 1;
  argc = 0;
  for( i = 0 ; pszMyCommandLine[i] ; i++ ){
     if( pszMyCommandLine[i] == '"' ) {
       i++;
       if( ThePreviousCharacterWasSpace ){
         ThePreviousCharacterWasSpace = 0;
         argv[argc++] = pszMyCommandLine+i;
       }
       while( ( pszMyCommandLine[i] ) && ( pszMyCommandLine[i] != '"' ) ) i++;
       if( pszMyCommandLine[i] == '"' ) pszMyCommandLine[i] = (char)0;
     }
     if( ThePreviousCharacterWasSpace ){
       ThePreviousCharacterWasSpace = 0;
       argv[argc++] = pszMyCommandLine+i;
       }
     if( pszMyCommandLine[i] == ' ' ){
       ThePreviousCharacterWasSpace = 1;
       pszMyCommandLine[i] = (char)0;
       }
     }
  argv[argc] = NULL;

  execvp(argv[0],argv); /* If this returns, we should exit the forked process. */
  exit(1);
#endif
  }
/*POD
=H file_waitpid
@c Check on a process


This function checks if a process identified by the
process ID (PID) is still running.

If the process is live the return value is zero (FALSE), otherwise
a positive value (TRUE) is returned and the second parameter contains
the exited process's final status.
/*FUNCTION*/
int file_waitpid(long pid,
                 unsigned long *plExitCode
   ){
/*noverbatim
CUT*/
#ifdef WIN32
  HANDLE hProcess;
  unsigned long ulExitCode;

  hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,0,(DWORD)pid);
  if( hProcess == NULL ){
    *plExitCode = 0; /* This is just to have some exit code instead of garbage. */
    return 1;
    }

  if( ! GetExitCodeProcess(hProcess,&ulExitCode) ){
    /* Failed */
    CloseHandle(hProcess);
    return FILESYSE_NOCODE;
  }

  if( plExitCode )*plExitCode = ulExitCode;
  CloseHandle(hProcess);

  return (ulExitCode == STILL_ACTIVE) ? 0 : 1;

#else
  int status;

  waitpid(pid,&status,WNOHANG);
  if( WIFEXITED(status) ){
    *plExitCode = WEXITSTATUS(status);
    return 1;
    }
  return 0;
#endif
  }

