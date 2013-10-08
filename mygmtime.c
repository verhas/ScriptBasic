/*
FILE: mygmtime.c
HEADER: mygmtime.h

TO_HEADER:
#define _DAY_SEC           (24L * 60L * 60L)
#define _YEAR_SEC          (365L * _DAY_SEC)
#define _FOUR_YEAR_SEC     (1461L * _DAY_SEC)
#define _BASE_DOW          4                
#define _BASE_YEAR         70L              
#define _MAX_YEAR          138L             
#define _LEAP_YEAR_ADJUST  17L              

*/

/*--------------------------------------------------------------------------------------------

 This file implements the time and date handling ScriptBasic functions. Before
 strating the real COMMANDs we have to implement a few standard functions. These are
 mktime and gmtime. Why should we reinvent the wheel?

 For two reasons: mktime converts GMT time to local time. We do not need this conversion.
 mktime adjust the year value in case month value is out of range (less than zero or larger
 than eleven). It is implemented this way on Windows NT, but man pages do not say anything
 about this feature. To be sure, I copied and a bit modified to our needs the code here.
 Just to be sure that it works on all system the same way.

 The problem with the function gmtime is more serious. NEVER USE POSIX gmtime() !!!!!!

 The reason: I am paranoid. The POSIX definition of the function is that it has to
 return a pointer to a structure containing the values. But where does the structure
 come from? Is it static or is it allocated? If this is static, then it is not thread
 safe. If this is allocated, who will release the memory.

 Instead here is a modified versio that requests the caller to pass a pointer to an allocated and
 available struct tm buffer, and the returned pointer will point to this buffer.

 --------------------------------------------------------------------------------------------*/
#include <time.h>
#include "mygmtime.h"
static int _lpdays[] = { -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
static int _days[] = { -1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364 };
/*
 * ChkAdd evaluates to TRUE if dest = src1 + src2 has overflowed
 */
#define ChkAdd(dest, src1, src2)   ( ((src1 >= 0L) && (src2 >= 0L) \
    && (dest < 0L)) || ((src1 < 0L) && (src2 < 0L) && (dest >= 0L)) )

/*
 * ChkMul evaluates to TRUE if dest = src1 * src2 has overflowed
 */
#define ChkMul(dest, src1, src2)   ( src1 ? (dest/src1 != src2) : 0 )

/*FUNCTION*/
long mygmktime(struct tm *tb
  ){
  time_t tmptm1, tmptm2, tmptm3;
  struct tm *tbtemp,Qtbtemp;

  /*
  * First, make sure tm_year is reasonably close to being in range.
  */
  if( ((tmptm1 = tb->tm_year) < _BASE_YEAR - 1) || (tmptm1 > _MAX_YEAR+ 1) )
     goto err_mktime;

  /*
  * Adjust month value so it is in the range 0 - 11.  This is because
  * we don't know how many days are in months 12, 13, 14, etc.
  */
  if( (tb->tm_mon < 0) || (tb->tm_mon > 11) ) {

  /* no danger of overflow because the range check above.  */
  tmptm1 += (tb->tm_mon / 12);
  if( (tb->tm_mon %= 12) < 0 ) {
    tb->tm_mon += 12;
    tmptm1--;
    }

  /*
  * Make sure year count is still in range.
  */
  if( (tmptm1 < _BASE_YEAR - 1) || (tmptm1 > _MAX_YEAR + 1) )
     goto err_mktime;
  }

  /***** HERE: tmptm1 holds number of elapsed years *****/

  /*
  * Calculate days elapsed minus one, in the given year, to the given
  * month. Check for leap year and adjust if necessary.
  */
  tmptm2 = _days[tb->tm_mon];
  if( !(tmptm1 & 3) && (tb->tm_mon > 1) )
    tmptm2++;

  /*
  * Calculate elapsed days since base date (midnight, 1/1/70, UTC)
  *
  * 365 days for each elapsed year since 1970, plus one more day for
  * each elapsed leap year. no danger of overflow because of the range
  * check (above) on tmptm1.
  */
  tmptm3 = (tmptm1 - _BASE_YEAR) * 365L + ((tmptm1 - 1L) >> 2)
          - _LEAP_YEAR_ADJUST;

  /* elapsed days to current month (still no possible overflow) */
  tmptm3 += tmptm2;

  /* elapsed days to current date. overflow is now possible. */
  tmptm1 = tmptm3 + (tmptm2 = (long)(tb->tm_mday));
  if( ChkAdd(tmptm1, tmptm3, tmptm2) ) goto err_mktime;

  /***** HERE: tmptm1 holds number of elapsed days *****/

  /* Calculate elapsed hours since base date */
  tmptm2 = tmptm1 * 24L;
  if( ChkMul(tmptm2, tmptm1, 24L) )goto err_mktime;

  tmptm1 = tmptm2 + (tmptm3 = (long)tb->tm_hour);
  if( ChkAdd(tmptm1, tmptm2, tmptm3) )goto err_mktime;

  /***** HERE: tmptm1 holds number of elapsed hours *****/

  /* Calculate elapsed minutes since base date */

  tmptm2 = tmptm1 * 60L;
  if( ChkMul(tmptm2, tmptm1, 60L) )goto err_mktime;

  tmptm1 = tmptm2 + (tmptm3 = (long)tb->tm_min);
  if ( ChkAdd(tmptm1, tmptm2, tmptm3) )goto err_mktime;

  /***** HERE: tmptm1 holds number of elapsed minutes *****/

  /* Calculate elapsed seconds since base date */

  tmptm2 = tmptm1 * 60L;
  if( ChkMul(tmptm2, tmptm1, 60L) )goto err_mktime;

  tmptm1 = tmptm2 + (tmptm3 = (long)tb->tm_sec);
  if ( ChkAdd(tmptm1, tmptm2, tmptm3) )goto err_mktime;

  /***** HERE: tmptm1 holds number of elapsed seconds *****/
  if( (tbtemp = mygmtime(&tmptm1,&Qtbtemp)) == NULL )goto err_mktime;

  *tb = *tbtemp;
  return tmptm1;

err_mktime:
  /* All errors come to here */
  return -1L;
}

/*FUNCTION*/
struct tm * mygmtime (time_t *timp, struct tm *ptb
  ){
  long caltim = *timp;            /* calendar time to convert */
  int islpyr = 0;                 /* is-current-year-a-leap-year flag */
  int tmptim;
  int *mdays;                /* pointer to days or lpdays */

  if( caltim < 0L )return NULL;

  /*
   * Determine years since 1970. First, identify the four-year interval
   * since this makes handling leap-years easy (note that 2000 IS a
   * leap year and 2100 is out-of-range).
   */
  tmptim = (int)(caltim / _FOUR_YEAR_SEC);
  caltim -= ((long)tmptim * _FOUR_YEAR_SEC);

  /*
   * Determine which year of the interval
   */
  tmptim = (tmptim * 4) + 70;         /* 1970, 1974, 1978,...,etc. */

  if( caltim >= _YEAR_SEC ) {
    tmptim++;                       /* 1971, 1975, 1979,...,etc. */
    caltim -= _YEAR_SEC;

    if( caltim >= _YEAR_SEC ) {
      tmptim++;                   /* 1972, 1976, 1980,...,etc. */
      caltim -= _YEAR_SEC;

      /*
      * Note, it takes 366 days-worth of seconds to get past a leap
      * year.
      */
      if( caltim >= (_YEAR_SEC + _DAY_SEC) ){
        tmptim++;           /* 1973, 1977, 1981,...,etc. */
        caltim -= (_YEAR_SEC + _DAY_SEC);
        }else {
        /*
        * In a leap year after all, set the flag.
        */
        islpyr++;
        }
      }
    }

  /*
  * tmptim now holds the value for tm_year. caltim now holds the
  * number of elapsed seconds since the beginning of that year.
  */
  ptb->tm_year = tmptim;

  /*
   * Determine days since January 1 (0 - 365). This is the tm_yday value.
   * Leave caltim with number of elapsed seconds in that day.
   */
  ptb->tm_yday = (int)(caltim / _DAY_SEC);
  caltim -= (long)(ptb->tm_yday) * _DAY_SEC;

  /*
  * Determine months since January (0 - 11) and day of month (1 - 31)
  */
  if( islpyr )mdays = _lpdays; else mdays = _days;

  for ( tmptim = 1 ; mdays[tmptim] < ptb->tm_yday ; tmptim++ ) ;

  ptb->tm_mon = --tmptim;

  ptb->tm_mday = ptb->tm_yday - mdays[tmptim];

  /*
   * Determine days since Sunday (0 - 6)
   */
  ptb->tm_wday = ((int)(*timp / _DAY_SEC) + _BASE_DOW) % 7;

  /*
   *  Determine hours since midnight (0 - 23), minutes after the hour
   *  (0 - 59), and seconds after the minute (0 - 59).
   */
  ptb->tm_hour = (int)(caltim / 3600);
  caltim -= (long)ptb->tm_hour * 3600L;

  ptb->tm_min = (int)(caltim / 60);
  ptb->tm_sec = (int)(caltim - (ptb->tm_min) * 60);

  ptb->tm_isdst = 0;
  return( (struct tm *)ptb );
  }

static long TimeDifference(void){
  time_t lTime;
  struct tm GmTime,*pGmTime;

/* calculate the time zone difference and day light saving hour together */
/* not too elegant, but works */
  lTime = (time_t)time(NULL);
  pGmTime = mygmtime(&lTime,&GmTime);
  pGmTime->tm_isdst = -1;
  return (long)(lTime - mktime(pGmTime));
  }
