//////////////////////////////////////////////////////////
// NOTE(erb): https://howardhinnant.github.io/date_algorithms.html#days_from_civil
B32
month_is_valid(U32 month)
{
  B32 result = (month >= CalendarMonth_MIN && month <= CalendarMonth_MAX);
  return(result);
}

B32
weekday_is_valid(U32 weekday)
{
  B32 result = (weekday >= CalendarWeekDay_MIN && weekday <= CalendarWeekDay_MAX);
  return(result);
}

B32
year_is_leap(I32 y)
{
  B32 result = (y%4 == 0 && (y%100 != 0 || y%400 == 0));
  return(result);
}

U32
last_day_of_month_common_year(Calendar_Month month)
{
  Assert(month_is_valid(month));
  U8 a[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  U32 result = (U32)a[month-1];
  Assert(result >= 28 && result <= 31);
  return(result);
}

U32
last_day_of_month_leap_year(Calendar_Month month)
{
  Assert(month_is_valid(month));
  U8 a[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  U32 result = (U32)a[month-1];
  Assert(result >= 29 && result <= 31);
  return(result);
}

U32
last_day_of_month(I32 year, Calendar_Month month)
{
  Assert(month_is_valid(month));
  U32 result = (month != 2 || !year_is_leap(year)) ? last_day_of_month_common_year(month) : 29;
  Assert(result >= 28 && result <= 31);
  return(result);
}

Calendar_Week_Day
weekday_from_days(I32 days)
{
  Assert(days >= i32_min && days <= i32_max-4);
  Calendar_Week_Day result = (Calendar_Week_Day)(days>=-4 ? (days+4)%7 : (days+5)%7+6);
  Assert(weekday_is_valid(result));
  return(result);
}

// NOTE(erb): returns the number of days from the weekday y to the weekday x.
U32
weekday_difference(Calendar_Week_Day x, Calendar_Week_Day y)
{
  Assert(weekday_is_valid(x));
  Assert(weekday_is_valid(y));
  x -= y;
  Calendar_Week_Day result = (x<=6 ? x : x+7);
  Assert(weekday_is_valid(result));
  return(result);
}

Calendar_Week_Day
next_weekday(Calendar_Week_Day weekday)
{
  Assert(weekday_is_valid(weekday));
  Calendar_Week_Day result = weekday<6 ? weekday+1: 0;
  Assert(weekday_is_valid(result));
  return(result);
}

Calendar_Week_Day
previous_weekday(Calendar_Week_Day weekday)
{
  Assert(weekday_is_valid(weekday));
  Calendar_Week_Day result = weekday>0 ? weekday-1: 6;
  Assert(weekday_is_valid(result));
  return(result);
}

B32 
is_day_of_month_valid(I32 year, Calendar_Month month, U32 day)
{
  B32 result = (day >= 1 && day <= last_day_of_month(year, month));
  return(result);
}

B32
date_is_valid(I32 year, Calendar_Month month, U32 day)
{
  B32 result = (month_is_valid(month) &&
                is_day_of_month_valid(year, month, day) &&
                (year >= i32_min && year <= i32_max-719468));
  return(result);
}
I32
days_from_civil(I32 year, Calendar_Month month, U32 day)
{
  date_is_valid(year, month, day);
  
  if(month <= 2)
  {
    year -= 1;
  }
  I32 adjusted_year = (year>=0 ? year : year-399);
  I32 era = adjusted_year / 400;
  U32 year_of_era = (U32)(year - era * 400);
  
  U32 adjusted_month = (month>2 ? month-3 : month+9); // [0,399]
  U32 day_of_year = (153*adjusted_month + 2)/5 + day-1; // [0, 365]
  U32 day_of_era = year_of_era*365 + year_of_era/4 - year_of_era/100 + day_of_year;
  
  I32 result = (era * 146097 + (I32)day_of_era - 719468);
  return(result);
}

void
civil_from_days(I32 civil, I32 *out_year, Calendar_Month *out_month, U32 *out_day)
{
  Assert(civil >= i32_min && civil <= i32_max-719468);
  
  civil += 719468;
  I32 adjusted_civil = (civil>=0 ? civil : civil-146097);
  I32 era = adjusted_civil/146097;
  
  U32 day_of_era = (U32)(civil - era*146097); // [0, 146097]
  U32 year_of_era = (day_of_era - day_of_era/1460 + day_of_era/36524 - day_of_era/146069)/365; // [0, 399]
  U32 year = (I32)year_of_era + era*400;
  
  U32 day_of_year = day_of_era - (365*year_of_era + year_of_era/4 - year_of_era/100);
  U32 month_p = (5*day_of_year + 2)/153;
  Calendar_Month month = (month_p<10 ? month_p+3 : month_p-9);
  U32 day = day_of_year - (153*month_p + 2)/5 + 1;
  
  *out_year = year;
  *out_month = month;
  *out_day = day;
}

//////////////////////////////////////////////////////////
// NOTE(erb): date_time utils

B32
cal_date_is_valid(Calendar_Date date)
{
  B32 result = date_is_valid(date.year, date.month, date.day);
  return(result);
}

Calendar_Date
cal_date(I32 year, Calendar_Month month, U32 day)
{
  Calendar_Date result = {0};
  result.year = year;
  result.month = month;
  result.day = day;
  Assert(cal_date_is_valid(result));
  return(result);
}

I32
cal_date_to_days(Calendar_Date date)
{
  I32 result = days_from_civil(date.year, date.month, date.day);
  return(result);
}

Calendar_Date
cal_date_from_days(I32 days)
{
  Calendar_Date result = {0};
  civil_from_days(days, &result.year, &result.month, &result.day);
  return(result);
}

//////////////////////////////////////////////////////////
// NOTE(erb): date_time arithmetic

Calendar_Date
cal_date_add_days(Calendar_Date date, I32 day_delta)
{
  Calendar_Date result = date;
  if(day_delta != 0)
  {
    I32 days = cal_date_to_days(date);
    days += day_delta;
    result = cal_date_from_days(days);
  }
  return(result);
}

Calendar_Date
cal_date_add_months(Calendar_Date date, I32 month_delta)
{
  I32 total = (I32)date.month - 1 + month_delta;
  
  I32 year = date.year + total/12;
  I32 remeinder = total % 12;
  if(remeinder < 0)
  {
    remeinder += 12;
    year -= 1;
  }
  U32 month = (U32)(remeinder + 1);
  U32 last_day = last_day_of_month(year, month);
  U32 day = (date.day <= last_day) ? date.day : last_day;
  Calendar_Date result = cal_date(year, month, day);
  return(result);
}

Calendar_Date
cal_date_nth_weekday_month_year(U32 n, Calendar_Week_Day weekday, Calendar_Month month, I32 year)
{
  Assert(n <= 5);
  Assert(weekday_is_valid(weekday));
  Assert(month_is_valid(month));
  Calendar_Week_Day first_weekday = weekday_from_days(days_from_civil(year, month, 1));
  U32 day = weekday_difference(weekday, first_weekday) + 1 + (n-1)*7;
  Assert(day >= 1 && day <= 31);
  Calendar_Date result = cal_date(year, month, day);
  return(result);
}

Calendar_Date
cal_date_last_weekday_month_year(Calendar_Week_Day weekday, Calendar_Month month, I32 year)
{
  Assert(weekday_is_valid(weekday));
  Assert(month_is_valid(month));
  U32 last = last_day_of_month(year, month);
  Calendar_Week_Day last_weekday = weekday_from_days(days_from_civil(year, month, last));
  U32 day = last - weekday_difference(last_weekday, weekday);
  Calendar_Date result = cal_date(year, month, day);
  return(result);
}

B32
date_time_greater_equal(Date_Time *test, Date_Time *ref)
{
  B32 result = (test->date.year  >= ref->date.year &&
                test->date.month >= ref->date.month && 
                test->date.day   >= ref->date.day &&
                
                test->time.hour   >= ref->time.hour &&
                test->time.minute >= ref->time.minute &&
                test->time.second >= ref->time.second &&
                test->time.ms     >= ref->time.ms);
  return(result);
}

//////////////////////////////////////////////////////////
// NOTE(erb): duration arithmetic

Date_Time
date_time_add_duration(Date_Time date_time, Time_Duration op)
{
  Date_Time result = date_time;
  
  // NOTE(erb): apply to the final date
  switch(op.kind)
  {
    // NOTE(erb): none defaults to day
    case TimeDurationKind_Minute:
    case TimeDurationKind_Hour:
    {
      I32 days_pre = cal_date_to_days(date_time.date);
      I32 minutes_pre = (days_pre*24*60 + date_time.time.hour*60 + date_time.time.minute);
      I32 minute_amount = ((op.kind == TimeDurationKind_Hour) ? 60*op.value : op.value);
      I32 minutes_total = (minutes_pre + minute_amount);
      
      I32 days = minutes_total / (24*60);
      result.date = cal_date_from_days(days);
      
      I32 minutes_remainder = minutes_total % (24*60);
      result.time.hour = (minutes_remainder / 60);
      result.time.minute = (minutes_remainder % 60);
      Assert(result.time.hour <= 23);
      Assert(result.time.minute <= 59);
    } break;
    
    case TimeDurationKind_Day: 
    case TimeDurationKind_Week: 
    {
      I32 days = cal_date_to_days(date_time.date);
      I32 day_amount = (op.kind == TimeDurationKind_Day) ? op.value : (7*op.value);
      result.date = cal_date_from_days(days + day_amount);
    } break;
    
    case TimeDurationKind_Month: 
    {
      result.date = cal_date_add_months(date_time.date, op.value);
    } break;
    
    case TimeDurationKind_Year: 
    {
      result.date.year += op.value;
    } break;
    
    InvalidDefaultCase;
  }
  return(result);
}
