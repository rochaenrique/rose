#include "marker.h"

// //////////////////////////////////////////////
// NOTE(erb): hash
// //////////////////////////////////////////////

// //////////////////////////////////////////////
// NOTE(erb): Date
// //////////////////////////////////////////////

B32 date_equals(Date a, Date b)
{
	B32 result = a.seconds == b.seconds
		&& a.minutes   == b.minutes
		&& a.hours     == b.hours
		&& a.month_day == b.month_day
		&& a.month     == b.month
		&& a.year      == b.year
		&& a.week_day  == b.week_day
		&& a.year_day  == b.year_day;
	return result;
}

Date add_day(Date date, U32 days)
{
	// TODO(enrique): Polish this to handle months with 31 days
	date.month_day = (date.month_day + days) % 31;
	date.month     = (date.month     + (days/31)) % 13 + 1;
	date.year      = (date.year      + (days/366));
	date.year_day  = (date.year_day  + days) % 366;
	date.week_day  = (date.week_day  + days) % 7;
	return date;
}

// //////////////////////////////////////////////
// NOTE(erb): math
// //////////////////////////////////////////////
