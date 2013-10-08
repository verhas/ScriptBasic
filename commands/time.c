/* time.c
*/
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../command.h"

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

 Instead here is a modified version that requests the caller to pass a pointer to an allocated and
 available struct tm buffer, and the returned pointer will point to this buffer.

 --------------------------------------------------------------------------------------------*/

#define _DAY_SEC           (24L * 60L * 60L)    /* secs in a day */
#define _YEAR_SEC          (365L * _DAY_SEC)    /* secs in a year */
#define _FOUR_YEAR_SEC     (1461L * _DAY_SEC)   /* secs in a 4 year interval */
#define _BASE_DOW          4                    /* 01-01-70 was a Thursday */
#define _BASE_YEAR         70L                  /* 1970 is the base year */
#define _MAX_YEAR          138L                 /* 2038 is the max year */
#define _LEAP_YEAR_ADJUST  17L                  /* Leap years 1900 - 1970 */
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

static struct tm * mygmtime (time_t *timp, struct tm *ptb);

static long mygmktime(struct tm *tb){
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

static struct tm * mygmtime (time_t *timp, struct tm *ptb){
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

#define MONTH_NAME_LEN 9
static char *MonthName[] = {
  "January", "February", "March", "April", "May", "June", 
  "July", "August", "September", "October", "November", "December"
  };
#define WEEK_DAY_NAME_LEN 9
static char *WeekDayName[] ={
  "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
  };


/**FORMATDATE
=section time
=display FORMATDATE()

=verbatim
FormatDate("format",time)
=noverbatim

Formats a time value (or date) according to the format string. The format string may contain placeholders. The first argument is the format string the second argument is the time value to convert. If the second argument is missing or T<undef> then the local time is converted.

=details
This function uses the first argument as a formatting string. This string may contain the following character strings:

=verbatim
     YEAR         four digit year
     YY           two digit year
     MON          three letter abbreviation of the month name
     MM           month
     0M           month with leading zero if needed
     *MONTH-NAME* name of the month
     DD           day of the month
     0D           day of the month with leading zero if needed
     WD           week day on a single digit starting with sunday=0
     WEEKDAY-NAME the name of the weekday
     WDN          three letter abbreviation fo the week day name
     HH           hours (24 hours notation)
     0H           hours with leading zero if needed (24 hours notation)
     hh           hours (12 hours notation)
     0h           hours with leading zero if needed (12 hours notation)
     mm           minutes
     0m           minutes with leading zero if needed
     am
     pm   is am or pm
=noverbatim

Any other character in the format string will get into the result verbatim.
*/
COMMAND(FORMATDATE)
#if NOTIMP_FORMATDATE
NOTIMPLEMENTED;
#else

  VARIABLE vFormatString,vTimeValue;
  time_t lTimeValue;
  NODE nItem;
  char *pszFormatString;
  struct tm *pGmTime,GmTime;
  char *s,*r,
        szNumberBuffer[5]; /* four digits + terminating zero */
  int hour,len;

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;

  nItem = PARAMETERLIST;

  vFormatString = CONVERT2STRING(_EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;
  if( vFormatString == NULL )ERROR(COMMAND_ERROR_INVALID_TIME_FORMAT);

  CONVERT2ZCHAR(vFormatString,pszFormatString);

  nItem = CDR(nItem);
  if( nItem ){
    vTimeValue = EVALUATEEXPRESSION(CAR(nItem));
    ASSERTOKE;
    }else vTimeValue = NULL;

  if( vTimeValue )
    lTimeValue = LONGVALUE(CONVERT2LONG(vTimeValue));
  else
    lTimeValue = (long)time(NULL)+ TimeDifference();

  /*
     Note that MONTHNAME and WEEKDAYNAME are long enough to be longer than the
     longest month name or week-day name. In case the month and week-day names are
     changed they should not be longer than 12 characters.
     YEAR         four digit year
     YY           two digit year
     MON          three letter abbreviation of the month name
     MM           month
     0M           month with leading zero if needed
     *MONTH-NAME* name of the month
     DD           day of the month
     0D           day of the month with leading zero if needed
     WD           week day on a single digit starting with sunday=0
     WEEKDAY-NAME the name of the weekday
     WDN          three letter abbreviation fo the week day name
     HH           hours (24 hours notation)
     0H           hours with leading zero if needed (24 hours notation)
     hh           hours (12 hours notation)
     0h           hours with leading zero if needed (12 hours notation)
     mm           minutes
     0m           minutes with leading zero if needed
     am
     pm   is am or pm
  
  */
  pGmTime = mygmtime(&lTimeValue,&GmTime);

  s = pszFormatString;
  while( *s ){

    if( !memcmp(s,"WDN",3) ){
      len = strlen(r = WeekDayName[pGmTime->tm_wday]);
      memcpy(s,r,3);
      s+=3;
      continue;
      }

    if( !memcmp(s,"WEEKDAY-NAME",12) ){
      len = strlen(r = WeekDayName[pGmTime->tm_wday]);
      memcpy(s,r,len);
      r = s + 12;
      s = s + len;
      len = s-r;
      for( ; r[len] = *r ; r++ );
      continue;
      }

    if( !memcmp(s,"*MONTH-NAME*",12) ){
      len = strlen(r = MonthName[pGmTime->tm_mon]);
      memcpy(s,r,len);
      r = s + 12;
      s = s + len;
      len = s-r;
      for( ; r[len] = *r ; r++ );
      continue;
      }

    if( !memcmp(s,"MON",3) ){
      len = strlen(r = MonthName[pGmTime->tm_mon]);
      memcpy(s,r,3);
      s += 3;
      continue;
      }

    if( !memcmp(s,"YEAR",4) ){
      sprintf(szNumberBuffer,"%04d",pGmTime->tm_year+1900);
      memcpy(s,szNumberBuffer,4);
      s += 4;
      continue;
      }

    if( !memcmp(s,"WD",2) ){
      *s = pGmTime->tm_wday + '0';
      s ++;
      for( r = s+1 ; r[-1] = *r ; r++ );
      continue;
      }

    if( !memcmp(s,"YY",2) ){
      sprintf(szNumberBuffer,"%04d",pGmTime->tm_year+1900);
      memcpy(s,szNumberBuffer+2,2);
      s += 2;
      continue;
      }

    if( !memcmp(s,"MM",2) ){
      if( pGmTime->tm_mon+1 < 10 ){
        *s = pGmTime->tm_mon+1 + '0';
        s ++;
        for( r = s+1 ; r[-1] = *r ; r++ );
        continue;
        }else{
        sprintf(szNumberBuffer,"%02d",pGmTime->tm_mon+1);
        memcpy(s,szNumberBuffer,2);
        s += 2;
        continue;
        }
      }

    if( !memcmp(s,"0M",2) ){
      sprintf(szNumberBuffer,"%02d",pGmTime->tm_mon+1);
      memcpy(s,szNumberBuffer,2);
      s += 2;
      continue;
      }

    if( !memcmp(s,"DD",2) ){
      if( pGmTime->tm_mday < 10 ){
        *s = pGmTime->tm_mday + '0';
        s ++;
        for( r = s+1 ; r[-1] = *r ; r++ );
        continue;
        }else{
        sprintf(szNumberBuffer,"%02d",pGmTime->tm_mday);
        memcpy(s,szNumberBuffer,2);
        s += 2;
        continue;
        }
      }

    if( !memcmp(s,"0D",2) ){
      sprintf(szNumberBuffer,"%02d",pGmTime->tm_mday);
      memcpy(s,szNumberBuffer,2);
      s += 2;
      continue;
      }

    if( !memcmp(s,"HH",2) ){
      if( pGmTime->tm_hour < 10 ){
        *s = pGmTime->tm_hour + '0';
        s ++;
        for( r = s+1 ; r[-1] = *r ; r++ );
        continue;
        }else{
        sprintf(szNumberBuffer,"%02d",pGmTime->tm_hour);
        memcpy(s,szNumberBuffer,2);
        s += 2;
        continue;
        }
      }

    if( !memcmp(s,"0H",2) ){
      sprintf(szNumberBuffer,"%02d",pGmTime->tm_hour);
      memcpy(s,szNumberBuffer,2);
      s += 2;
      continue;
      }

    if( !memcmp(s,"hh",2) ){
      hour = pGmTime->tm_hour;
      if( hour > 12 )hour -= 12;
      if( hour < 10 ){
        *s = hour + '0';
        s ++;
        for( r = s+1 ; r[-1] = *r ; r++ );
        continue;
        }else{
        sprintf(szNumberBuffer,"%02d",hour);
        memcpy(s,szNumberBuffer,2);
        s += 2;
        continue;
        }
      }

    if( !memcmp(s,"0h",2) ){
      hour = pGmTime->tm_hour;
      if( hour > 12 )hour -= 12;
      sprintf(szNumberBuffer,"%02d",hour);
      memcpy(s,szNumberBuffer,2);
      s += 2;
      continue;
      }

    if( !memcmp(s,"mm",2) ){
      if( pGmTime->tm_min < 10 ){
        *s = pGmTime->tm_min + '0';
        s ++;
        for( r = s+1 ; r[-1] = *r ; r++ );
        continue;
        }else{
        sprintf(szNumberBuffer,"%02d",pGmTime->tm_min);
        memcpy(s,szNumberBuffer,2);
        s += 2;
        continue;
        }
      }

    if( !memcmp(s,"0m",2) ){
      sprintf(szNumberBuffer,"%02d",pGmTime->tm_min);
      memcpy(s,szNumberBuffer,2);
      s += 2;
      continue;
      }

    if( !memcmp(s,"ss",2) ){
      if( pGmTime->tm_sec < 10 ){
        *s = pGmTime->tm_sec + '0';
        s ++;
        for( r = s+1 ; r[-1] = *r ; r++ );
        continue;
        }else{
        sprintf(szNumberBuffer,"%02d",pGmTime->tm_sec);
        memcpy(s,szNumberBuffer,2);
        s += 2;
        continue;
        }
      }

    if( !memcmp(s,"0s",2) ){
      sprintf(szNumberBuffer,"%02d",pGmTime->tm_sec);
      memcpy(s,szNumberBuffer,2);
      s += 2;
      continue;
      }

    if( (!memcmp(s,"am",2)) || (!memcmp(s,"pm",2)) ){
      if( pGmTime->tm_hour >= 12 )r = "pm"; else r = "am";
      memcpy(s,r,2);
      s += 2;
      continue;
      }

    s++;
    }
  RESULT = NEWMORTALSTRING(strlen(pszFormatString));
  ASSERTNULL(RESULT)
  memcpy(STRINGVALUE(RESULT),pszFormatString,strlen(pszFormatString));

#endif
END

/**NOW
=section time
=display Now()

This function returns the local time expressed as seconds since January 1, 1970, 00:00am. The function does not accept any argument. This function is similar to the function R<GMTIME> but returns the local time instead of the actual GMT.
*/
COMMAND(NOW)
#if NOTIMP_NOW
NOTIMPLEMENTED;
#else

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;
  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  LONGVALUE(RESULT) = (long)time(NULL)+ TimeDifference();
#endif
END

/**GMTIME
=section time
=display GmTime()

This function returns the GMT time expressed as seconds since January 1, 1970, 00:00am. The function does not accept any argument. This function is similar to the function R<NOW> but returns the GMT time instead of the actual local time. 
*/
COMMAND(GMTIME)
#if NOTIMP_GMTIME
NOTIMPLEMENTED;
#else

  /* this is an operator and not a command, therefore we do not have our own mortal list */
  USE_CALLER_MORTALS;
  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  LONGVALUE(RESULT) = (long)time(NULL);
#endif
END

#define NOCOMMAND(XXX) \
COMMAND(XXX)\
NOTIMPLEMENTED;\
END

#ifdef __sun
#undef SEC
#endif

#define TIMEFUN(NAME,FIELD) \
COMMAND(NAME)\
\
 VARIABLE vTime;\
 time_t lTime;\
 NODE nItem;\
 struct tm *pGmTime,GmTime;\
\
  USE_CALLER_MORTALS;\
  nItem = PARAMETERLIST;\
  if( nItem ){\
    vTime = EVALUATEEXPRESSION(CAR(nItem));\
    ASSERTOKE;\
  }else\
    vTime = NULL;\
\
  RESULT = NEWMORTALLONG;\
  ASSERTNULL(RESULT)\
\
  if( memory_IsUndef(vTime) )\
    lTime = (long)time(NULL)+ TimeDifference();\
  else\
    lTime = LONGVALUE(CONVERT2LONG(vTime));\
\
  pGmTime = mygmtime(&lTime,&GmTime);\
  LONGVALUE(RESULT) = pGmTime->FIELD;\
END

/**YEAR
=section time
=display Year()

This function accepts one argument that should express the time in number of seconds since January 1, 1970 0:00 am and returns the year value of that time. If the argument is missing it uses the actual local time to calculate the year value. In other words it returns the actual year.
*/
#if NOTIMP_YEAR
NOCOMMAND(YEAR)
#else
TIMEFUN(YEAR,tm_year+1900)
#endif

/**MONTH
=section time
=display Month()

This function accepts one argument that should express the time in number of seconds since January 1, 1970 0:00 am and returns the month (1 to 12) value of that time. If the argument is missing it uses the actual local time. In other words it returns the actual month in this latter case. The months are numbered so that January is 1 and December is 12.
*/
#if NOTIMP_MONTH
NOCOMMAND(MONTH)
#else
TIMEFUN(MONTH,tm_mon+1)
#endif

/**DAY
=section time
=display Day()

This function accepts one argument that should express the time in number of seconds since January 1, 1970 0:00 am and returns the day of the month (1 to 31) value of that time. If the argument is missing the function uses the actual local time to calculate the day of the month value. In other words it returns the day value of the actual date.
*/
#if NOTIMP_DAY
NOCOMMAND(DAY)
#else
TIMEFUN(DAY,tm_mday)
#endif

/**WEEKDAY
=section time
=display WeekDay()

This function accepts one argument that should express the time in number of seconds since January 1, 1970 0:00 am and returns the week day value of that time. If the argument is missing the function uses the actual local time. In other words it returns what day it is at the moment.
*/
#if NOTIMP_WDAY
NOCOMMAND(WDAY)
#else
TIMEFUN(WDAY,tm_wday)
#endif

/**YEARDAY
=section time
=display YearDay()

This function accepts one argument that should express the time in number of seconds since January 1, 1970 0:00 am and returns the year-day value of that time. This is actually the number of the day inside the year so that January 1st is #1 and December 31 is #365 (or 366 in leap years). If the argument is missing the function uses the actual local time.
*/
#if NOTIMP_YDAY
NOCOMMAND(YDAY)
#else
TIMEFUN(YDAY,tm_yday)
#endif

/**HOUR
=section time
=display Hour()

This function accepts one argument that should express the time in number of seconds since January 1, 1970 0:00 am and returns the hour value of that time. If the argument is missing the function uses the actual local time.
*/
#if NOTIMP_HOUR
NOCOMMAND(HOUR)
#else
TIMEFUN(HOUR,tm_hour)
#endif

/**MINUTE
=section time
=display Minute()

This function accepts one argument that should express the time in number of seconds since January 1, 1970 0:00 am and returns the minute value of that time. If the argument is missing it uses the actual local time.
*/
#if NOTIMP_MINUTE
NOCOMMAND(MINUTE)
#else
TIMEFUN(MINUTE,tm_min)
#endif

/**SEC
=section time
=display Sec()

This function accepts one argument that should express the time in number of seconds since January 1, 1970 0:00 am and returns the seconds value of that time. If the argument is missing the function uses the actual local time.
*/
#if NOTIMP_SEC
NOCOMMAND(SEC)
#else
TIMEFUN(SEC,tm_sec)
#endif

#undef TIMEFUN


/**TIMEVALUE
=section time
=display TimeValue()

This function gets zero or more, at most six arguments and interprets them as year, month, day, hour, minute and seconds and calculates the number of seconds elapsed since January 1, 1970 till the time specified. If some arguments are missing or T<undef> the default values are the following:
=itemize
=item year = 1970
=item month = January
=item day = 1st
=item hours = 0
=item minutes = 0
=item seconds = 0
=noitemize

*/
COMMAND(TIMEVALUE)
#if NOTIMP_TIMEVALUE
NOTIMPLEMENTED;
#else

  VARIABLE vTime;
  long lTime;
  NODE nItem;
  struct tm GmTime;

/* Set the default values in case some of the parameters are missing */
  GmTime.tm_year = 1970;
  GmTime.tm_mon = 1;
  GmTime.tm_mday = 1;
  GmTime.tm_hour = 0;
  GmTime.tm_min = 0;
  GmTime.tm_sec =  0;
  GmTime.tm_isdst = -1;

  USE_CALLER_MORTALS;
  nItem = PARAMETERLIST;
  if( nItem == 0 )goto NoMoreTime;

#define TAKE_ARGUMENT(x) \
  vTime = CONVERT2LONG(EVALUATEEXPRESSION(CAR(nItem)));\
  ASSERTOKE;\
  if( vTime )GmTime.x = LONGVALUE(vTime);\
  nItem = CDR(nItem);\
  if( nItem == 0 )goto NoMoreTime;\

  TAKE_ARGUMENT(tm_year)
  TAKE_ARGUMENT(tm_mon)
  TAKE_ARGUMENT(tm_mday)
  TAKE_ARGUMENT(tm_hour)
  TAKE_ARGUMENT(tm_min)
  TAKE_ARGUMENT(tm_sec)

NoMoreTime:;
  GmTime.tm_year -= 1900;
  GmTime.tm_mon --;
  lTime = mygmktime(&GmTime);
  if( lTime == -1 )ERROR(COMMAND_ERROR_INVALID_TIME);
  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  LONGVALUE(RESULT) = lTime;
#endif
END

/**GMTOLOCALTIME
=section time
=display GmToLocalTime()

This function accepts one argument that has to be the number of seconds elapsed since January 1, 1970 0:00 am in GMT. The function returns the same number of seconds in local time. In other words the function converts a GMT time value to local time value.
*/
COMMAND(GM2LOCAL)
#if NOTIMP_GM2LOCAL
NOTIMPLEMENTED;
#else

  VARIABLE vTime;
  long lTime;
  NODE nItem;

  USE_CALLER_MORTALS;
  nItem = PARAMETERLIST;
  if( nItem == 0 ){
    RESULT = NULL;
    RETURN;
    }
  vTime = CONVERT2LONG(EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;
  if( memory_IsUndef(vTime) ){
    RESULT = NULL;
    RETURN;
    }
  lTime = LONGVALUE(vTime);
  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  LONGVALUE(RESULT) = lTime + TimeDifference();
#endif
END

/**LOCATLTOGMTIME
=section time
=display LocalToGmTime()

This function accepts one argument that has to be the number of seconds elapsed since January 1, 1970 0:00 am in local time. The function returns the same number of seconds in GMT. In other words the function converts a local time value to GMT time value.
*/
COMMAND(LOCAL2GM)
#if NOTIMP_LOCAL2GM
NOTIMPLEMENTED;
#else

  VARIABLE vTime;
  long lTime;
  NODE nItem;

  USE_CALLER_MORTALS;
  nItem = PARAMETERLIST;
  if( nItem == 0 ){
    RESULT = NULL;
    RETURN;
    }
  vTime = CONVERT2LONG(EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;
  if( memory_IsUndef(vTime) ){
    RESULT = NULL;
    RETURN;
    }
  lTime = LONGVALUE(vTime);
  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  LONGVALUE(RESULT) = lTime - TimeDifference();
#endif
END

/**ADDYEAR
=section time
=display AddYear()

This function takes two arguments. The first argument is a time value, the second is an integer value. The function increments the year of the time value by the second argument and returns the time value for the same month, day, hour and minute but some years later or sooner in case the second argument is negative.

This is a bit more complex than just adding 365*24*60*60 to the value, because leap-years are longer and in case you add several years to the time value you should consider adding these longer years extra days. This is calculated correct in this function.

If the original time value is February 29 on a leap-year and the resulting value is in a year, which is not leap year the function will return February 28.

Note that because of this correction using the function in a loop is not the same as using it once. For example:

=verbatim
print AddYear(TimeValue(2000,02,29),4),"\n"
print AddYear(AddYear(TimeValue(2000,02,29),2),2),"\n"
=noverbatim

will print two different values.
*/
COMMAND(ADDYEAR)
#if NOTIMP_ADDYEAR
NOTIMPLEMENTED;
#else

  VARIABLE vTime,vOffset;
  time_t lTime;
  NODE nItem;
  struct tm *pGmTime,GmTime;

  nItem = PARAMETERLIST;
  if( nItem == 0 ){
    RESULT = NULL;
    RETURN;
    }
  vTime = CONVERT2LONG(EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;
  nItem = CDR(nItem);
  vOffset = CONVERT2LONG(EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;
  if( memory_IsUndef(vTime) || memory_IsUndef(vOffset) ){
    RESULT = NULL;
    RETURN;
    }
  lTime = LONGVALUE(vTime);
  pGmTime = mygmtime(&lTime,&GmTime);
  pGmTime->tm_year += LONGVALUE(vOffset);
  /* Note that this is a very simple leap year calculation because
     all UNIX-es calculate time from 1970 to 2038 only.
     This is rubbish, but that is the way it is under UNIX. */
  if( pGmTime->tm_mday > 28 && pGmTime->tm_mon == 1 && (pGmTime->tm_year%4) )
    pGmTime->tm_mday = 28;
  pGmTime->tm_isdst = -1;
  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  LONGVALUE(RESULT) = mygmktime(&GmTime);
#endif
END

/**ADDMONTH
=section time
=display AddMonth()

This function takes two arguments. The first argument is a time value, the second is an integer value. The function increments the month by the second argument and returns the time value for the same day, hour and minute but some months later or sooner in case the second argument is negative.

If the resulting value is on a day that does not exist on the result month then the day part of the result is decreased. For example:

=verbatim
print FormatTime("MONTH DAY, YEAR",AddMonth(TimeValue(2000,03,31),1))
=noverbatim

will print

=verbatim
April 30, 2000
=noverbatim
*/
COMMAND(ADDMONTH)
#if NOTIMP_ADDMONTH
NOTIMPLEMENTED;
#else

  VARIABLE vTime,vOffset;
  time_t lTime;
  NODE nItem;
  struct tm *pGmTime,GmTime;

  nItem = PARAMETERLIST;
  if( nItem == 0 ){
    RESULT = NULL;
    RETURN;
    }
  vTime = CONVERT2LONG(EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;
  nItem = CDR(nItem);
  vOffset = CONVERT2LONG(EVALUATEEXPRESSION(CAR(nItem)));
  ASSERTOKE;
  if( memory_IsUndef(vTime) || memory_IsUndef(vOffset) ){
    RESULT = NULL;
    RETURN;
    }
  lTime = LONGVALUE(vTime);
  pGmTime = mygmtime(&lTime,&GmTime);
  pGmTime->tm_mon += LONGVALUE(vOffset);
  if( pGmTime->tm_mday == 31 && (pGmTime->tm_mon == 3 ||
                                 pGmTime->tm_mon == 5 ||
                                 pGmTime->tm_mon == 7 ||
                                 pGmTime->tm_mon == 8 ||
                                 pGmTime->tm_mon == 10 ))
    pGmTime->tm_mday = 30;
  if( pGmTime->tm_mday > 29 && pGmTime->tm_mon == 1 )
    pGmTime->tm_mday = 29;
  /* Note that this is a very simple leap year calculation because
     all UNIX-es calculate time from 1970 to 2038 only.
     This is rubbish, but that the way it is under UNIX. */
  if( pGmTime->tm_mday > 28 && pGmTime->tm_mon == 1 && (pGmTime->tm_year%4) )
    pGmTime->tm_mday = 28;

  pGmTime->tm_isdst = -1;
  RESULT = NEWMORTALLONG;
  ASSERTNULL(RESULT)
  LONGVALUE(RESULT) = mygmktime(&GmTime);
#endif
END


#undef TIMEFUN
#define TIMEFUN(XXX,YYY) \
COMMAND(XXX)\
  VARIABLE vTime,vOffset;\
  long lTime;\
  NODE nItem;\
\
  nItem = PARAMETERLIST;\
  if( nItem == 0 ){\
    RESULT = NULL;\
    RETURN;\
    }\
  vTime = CONVERT2LONG(EVALUATEEXPRESSION(CAR(nItem)));\
  ASSERTOKE;\
  nItem = CDR(nItem);\
  vOffset = CONVERT2LONG(EVALUATEEXPRESSION(CAR(nItem)));\
  ASSERTOKE;\
  if( memory_IsUndef(vTime) || memory_IsUndef(vOffset) ){\
    RESULT = NULL;\
    RETURN;\
    }\
  lTime = LONGVALUE(vTime);\
  RESULT = NEWMORTALLONG;\
  ASSERTNULL(RESULT)\
  LONGVALUE(RESULT) = lTime + LONGVALUE(vOffset)*YYY;\
END

/**ADDWEEK
=section time
=display AddWeek()

This function takes two arguments. The first argument is a time value, the second is an integer value. The function increments the week by the second argument and returns the time value for the same hour and minute but some weeks later or sooner in case the second argument is negative.

This function is very simple from the arithmetic's point of view, because it simply adds 604800 times the second argument to the first argument and returns the result.
*/
#if NOTIMP_ADDWEEK
NOCOMMAND(ADDWEEK)
#else
TIMEFUN(ADDWEEK,604800)
#endif

/**ADDDAY
=section time
=display AddDay()

This function takes two arguments. The first argument is a time value, the second is an integer value. The function increments the day by the second argument and returns the time value for the same hour and minute but some days later or sooner in case the second argument is negative.

This function is very simple from the arithmetic's point of view, because it simply adds 86400 times the second argument to the first argument and returns the result.
*/
#if NOTIMP_ADDDAY
NOCOMMAND(ADDDAY)
#else
TIMEFUN(ADDDAY,86400)
#endif

/**ADDHOUR
=section time
=display AddHour()

This function takes two arguments. The first argument is a time value, the second is an integer value. The function increments the hours by the second argument and returns the time value for the same minute and seconds but some hours later or sooner in case the second argument is negative.

This function is very simple from the arithmetic's point of view, because it simply adds 3600 times the second argument to the first argument and returns the result.
*/
#if NOTIMP_ADDHOUR
NOCOMMAND(ADDHOUR)
#else
TIMEFUN(ADDHOUR,3600)
#endif

/**ADDMINUTE
=section time
=display AddMinute()

This function takes two arguments. The first argument is a time value, the second is an integer value. The function increments the minutes by the second argument and returns the time value for the same seconds but some minutes later or sooner in case the second argument is negative.

This function is very simple from the arithmetic's point of view, because it simply adds 60 times the second argument to the first argument and returns the result.
*/
#if NOTIMP_ADDMINUTE
NOCOMMAND(ADDMINUTE)
#else
TIMEFUN(ADDMINUTE,60)
#endif

/**ADDSECOND
=section time
=display AddSecond()

This function takes two arguments. The first argument is a time value, the second is an integer value. The function increments the seconds by the second argument and returns the time value.

This function is the simplest from the arithmetic's point of view, because it simply adds the second argument to the first argument and returns the result.
*/
#if NOTIMP_ADDSECOND
NOCOMMAND(ADDSECOND)
#else
TIMEFUN(ADDSECOND,1);
#endif

#undef TIMEFUN
