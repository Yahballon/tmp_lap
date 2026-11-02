// SPDX-License-Identifier: MIT
// Standby Clock by YAHBALLON - portable date, alarm and settings logic.
#pragma once
#include <algorithm>
#include <array>
#include <cstdio>
#include <ctime>
#include <string>

namespace standby {
struct Settings {
  int theme=0;            // 0 red, 1 amber, 2 ice blue, 3 white
  int brightness=4;       // 1..5
  int use24h=1;
  int alarmOn=1;
  int alarmHour=6;
  int alarmMinute=20;
  int utcOffset=99;       // 99 = device-local, otherwise UTC hours
  int pixelDrift=1;
  int oledGuard=1;
  int autoDim=1;
  int weatherEnabled=1;
};
inline void validate(Settings& s) {
  s.theme=std::clamp(s.theme,0,3);
  s.brightness=std::clamp(s.brightness,1,5);
  s.use24h=(s.use24h!=0);
  s.alarmOn=(s.alarmOn!=0);
  s.alarmHour=std::clamp(s.alarmHour,0,23);
  s.alarmMinute=std::clamp(s.alarmMinute,0,59);
  s.utcOffset=(s.utcOffset==99)?99:std::clamp(s.utcOffset,-12,14);
  s.pixelDrift=(s.pixelDrift!=0);
  s.oledGuard=(s.oledGuard!=0);
  s.autoDim=(s.autoDim!=0);
  s.weatherEnabled=(s.weatherEnabled!=0);
}
struct ClockTime {
  int year=2026,month=1,day=1,weekday=4,yday=0,hour=0,minute=0,second=0;
};
inline ClockTime from_timestamp(std::time_t instant, int utcOffset) {
  // Copy the returned tm immediately: localtime/gmtime have static buffers.
  std::time_t adjusted=instant;
  if (utcOffset!=99) adjusted+=std::time_t(utcOffset)*3600;
  const std::tm* p=utcOffset==99?std::localtime(&instant):std::gmtime(&adjusted);
  if (!p) return {};
  return {p->tm_year+1900,p->tm_mon+1,p->tm_mday,p->tm_wday,p->tm_yday,
          p->tm_hour,p->tm_min,p->tm_sec};
}
inline int month_days(int year,int month) {
  static constexpr int days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  if(month<1||month>12) return 30;
  if(month==2 && year%4==0 && (year%100!=0 || year%400==0)) return 29;
  return days[month-1];
}
inline std::array<int,7> week_dates(const ClockTime& t) {
  std::array<int,7> dates{};
  int first=t.day-((t.weekday+6)%7);
  for(int i=0;i<7;i++) {
    int d=first+i,year=t.year,month=t.month;
    while(d<1) {
      if(--month<1){month=12;--year;}
      d+=month_days(year,month);
    }
    while(d>month_days(year,month)) {
      d-=month_days(year,month);
      if(++month>12){month=1;++year;}
    }
    dates[i]=d;
  }
  return dates;
}
inline bool alarm_due(const Settings& s,const ClockTime& t) {
  return s.alarmOn!=0 && t.hour==s.alarmHour && t.minute==s.alarmMinute;
}
inline int day_key(const ClockTime& t) {
  return t.year*1000+t.yday;
}
inline std::string digital(const ClockTime& t,bool use24h) {
  int hour=t.hour;
  if(!use24h){hour%=12;if(hour==0)hour=12;}
  char b[16];
  std::snprintf(b,sizeof(b),"%02d:%02d",hour,t.minute);
  return b;
}
inline std::string alarm_text(const Settings& s) {
  int h=s.alarmHour%12;
  if(h==0)h=12;
  char b[32];
  std::snprintf(b,sizeof(b),"%d:%02d %s",h,s.alarmMinute,s.alarmHour<12?"AM":"PM");
  return b;
}
inline const char* weekday_name(int weekday) {
  static constexpr const char* names[]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
  return names[std::clamp(weekday,0,6)];
}
inline const char* month_name(int month) {
  static constexpr const char* names[]={"JAN","FEB","MAR","APR","MAY","JUN",
    "JUL","AUG","SEP","OCT","NOV","DEC"};
  return names[std::clamp(month,1,12)-1];
}
} // namespace standby
