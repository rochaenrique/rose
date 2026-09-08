/* date = August 3rd 2026 5:28 pm */

#ifndef BASE_TIME_H
#define BASE_TIME_H

// NOTE(erb): functions use unsigned arithmetic so its necessary because C is trash
typedef U32 Calendar_Week_Day;
enum Calendar_Week_Day
{
  CalendarWeekDay_MIN       = 0,
  CalendarWeekDay_Sunday    = 0,
  CalendarWeekDay_Monday    = 1,
  CalendarWeekDay_Tuesday   = 2,
  CalendarWeekDay_Wednesday = 3,
  CalendarWeekDay_Thursday  = 4,
  CalendarWeekDay_Friday    = 5,
  CalendarWeekDay_Saturday  = 6,
  CalendarWeekDay_MAX       = 6,
};

// NOTE(erb): functions use unsigned arithmetic so its necessary because C is trash
typedef U32 Calendar_Month;
enum Calendar_Month
{
  CalendarMonth_MIN       = 1,
  CalendarMonth_January   = 1,
  CalendarMonth_February  = 2,
  CalendarMonth_March     = 3,
  CalendarMonth_April     = 4,
  CalendarMonth_May       = 5,
  CalendarMonth_June      = 6,
  CalendarMonth_July      = 7,
  CalendarMonth_August    = 8,
  CalendarMonth_September = 9,
  CalendarMonth_October   = 10,
  CalendarMonth_November  = 11,
  CalendarMonth_December  = 12,
  CalendarMonth_MAX       = 12,
};

typedef struct Calendar_Date Calendar_Date;
struct Calendar_Date
{
  I32 year;             // allows negative before bc
  Calendar_Month month; // 1-12
  U32 day;              // 1-31
};

typedef struct Clock_Time Clock_Time;
struct Clock_Time
{
  U32 hour;        // 0-23
  U32 minute;      // 0-59
  U32 second;      // 0-59
  U32 ms;          // 0-999
};

typedef struct Date_Time Date_Time;
struct Date_Time
{
  Calendar_Date date;
  Clock_Time time;
};

// NOTE(erb): duration

typedef enum Time_Duration_Kind
{
  TimeDurationKind_Minute,
  TimeDurationKind_Hour,
  TimeDurationKind_Day,
  TimeDurationKind_Week,
  TimeDurationKind_Month,
  TimeDurationKind_Year,
  TimeDurationKind_COUNT,
} Time_Duration_Kind;

typedef struct Time_Duration Time_Duration;
struct Time_Duration
{
  I32 value;
  Time_Duration_Kind kind;
};

#endif //BASE_TIME_H
