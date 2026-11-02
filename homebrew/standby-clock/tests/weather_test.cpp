#include "../native/weather_model.hpp"
#include "../native/clock_logic.hpp"
#include <cassert>
#include <string>
#include <iostream>
int main(){
 using namespace standby::weather;
 Location loc;
 assert(parse_location(R"({"city":"Schwerte","latitude":51.443,"longitude":7.567,"ip":"203.0.113.1"})",loc));
 assert(loc.valid && loc.city=="Schwerte" && loc.latitude>51 && loc.longitude>7);
 assert(!parse_location(R"({"city":"Bad","latitude":999,"longitude":7.5})",loc));
 assert(!parse_location(R"({"city":"Bad","latitude":51.1})",loc));
 Data out;
 const std::string ok=R"({"latitude":51.43,"current_units":{"temperature_2m":"°C"},"current":{"time":"2026-11-02T10:30","temperature_2m":8.6,"relative_humidity_2m":88,"weather_code":3,"wind_speed_10m":13.5}})";
 assert(parse_current(ok,out));
 assert(out.valid&&!out.stale&&out.temperature==9&&out.humidity==88&&out.windKmh==14);
 assert(out.condition=="CLOUDY");
 assert(std::string(condition(95))=="THUNDERSTORM");
 assert(std::string(condition(61))=="RAIN");
 assert(!parse_current(R"({"current":{"temperature_2m":null,"relative_humidity_2m":55,"weather_code":0,"wind_speed_10m":4}})",out));
 assert(!parse_current(R"({"current":{"temperature_2m":500,"relative_humidity_2m":55,"weather_code":0,"wind_speed_10m":4}})",out));
 assert(!parse_current(R"({"current":{"temperature_2m":10,"relative_humidity_2m":55,"weather_code":0}})",out));
 standby::Settings settings;
 assert(settings.oledGuard==1&&settings.autoDim==1&&settings.weatherEnabled==1);
 settings.oledGuard=-4;settings.autoDim=0;settings.weatherEnabled=7;
 standby::validate(settings);
 assert(settings.oledGuard==1&&settings.autoDim==0&&settings.weatherEnabled==1);
 std::cout<<"PASS: weather location/current parsing and OLED preferences\n";
}
