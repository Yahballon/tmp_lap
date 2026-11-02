#include "../native/clock_logic.hpp"
#include <cassert>
#include <iostream>
int main(){
 using namespace standby;
 Settings s;s.alarmHour=99;s.alarmMinute=-4;s.brightness=0;s.theme=88;
 validate(s);assert(s.alarmHour==23&&s.alarmMinute==0&&s.brightness==1&&s.theme==3);
 s.alarmHour=6;s.alarmMinute=20;s.alarmOn=1;
 ClockTime d;d.year=2026;d.month=11;d.day=2;d.weekday=1;d.hour=6;d.minute=20;
 assert(alarm_due(s,d));d.minute=21;assert(!alarm_due(s,d));
 assert(month_days(2024,2)==29&&month_days(2026,2)==28);
 d.day=1;d.weekday=0;d.month=11;
 auto days=week_dates(d);assert(days[0]==26&&days[6]==1);
 d.day=2;d.weekday=1;d.hour=0;d.minute=5;
 assert(digital(d,false)=="12:05"&&digital(d,true)=="00:05");
 d.hour=13;assert(digital(d,false)=="01:05");
 assert(day_key(d)==2026000);
 assert(std::string(month_name(11))=="NOV");
 std::cout<<"PASS standby clock calendar/alarm model\n";
}
