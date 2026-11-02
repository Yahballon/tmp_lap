// SPDX-License-Identifier: MIT
// Weather response decoding, small and dependency-free for PS5 host tests.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <string_view>
namespace standby::weather {
struct Data {
 bool valid=false, stale=false, busy=false;
 int temperature=0,code=-1,humidity=0,windKmh=0;
 std::string city,condition="OFFLINE",status="WAITING FOR WEATHER";
};
struct Location {
 bool valid=false;
 double latitude=0,longitude=0;
 std::string city;
};
inline std::string_view field(std::string_view json,std::string_view name){
 const std::string wanted="\""+std::string(name)+"\"";
 size_t p=json.find(wanted);
 while(p!=std::string_view::npos){
  size_t v=p+wanted.size();
  while(v<json.size()&&(json[v]==' '||json[v]=='\r'||json[v]=='\n'))++v;
  if(v<json.size()&&json[v]==':'){
   ++v;while(v<json.size()&&(json[v]==' '||json[v]=='\r'||json[v]=='\n'))++v;
   return json.substr(v);
  }
  p=json.find(wanted,p+wanted.size());
 }
 return {};
}
inline bool number(std::string_view json,std::string_view key,double& output,double low,double high){
 auto v=field(json,key);
 if(v.empty())return false;
 std::string b(v.substr(0,std::min<size_t>(64,v.size())));
 char* end=nullptr;double n=std::strtod(b.c_str(),&end);
 if(end==b.c_str()||!std::isfinite(n)||n<low||n>high)return false;
 if(*end!='\0'&&*end!=','&&*end!='}'&&*end!=' '&&*end!='\n'&&*end!='\r')return false;
 output=n;return true;
}
inline std::string city_name(std::string_view json){
 auto v=field(json,"city");
 if(v.empty()||v.front()!='"')return {};
 std::string result;
 for(size_t i=1;i<v.size()&&result.size()<24;++i){
  unsigned char c=(unsigned char)v[i];
  if(c=='"')return result;
  if(c=='\\'){
   if(++i>=v.size())return {};
   unsigned char e=(unsigned char)v[i];
   if(e=='u')return result.empty()?std::string("LOCAL"):result;
   if(e=='"'||e=='\\')result+=(char)e;
  }else if(c>=32&&c<127 && c!='<' && c!='>')result+=(char)c;
 }
 return {};
}
inline bool parse_location(std::string_view json,Location& out){
 double lat=0,lon=0;
 if(!number(json,"latitude",lat,-90,90)||!number(json,"longitude",lon,-180,180))return false;
 std::string city=city_name(json);
 if(city.empty())city="LOCAL AREA";
 out={true,lat,lon,city};return true;
}
inline const char* condition(int code){
 if(code==0)return "CLEAR";
 if(code==1)return "MOSTLY CLEAR";
 if(code==2)return "PARTLY CLOUDY";
 if(code==3)return "CLOUDY";
 if(code==45||code==48)return "FOGGY";
 if(code==51||code==53||code==55||code==56||code==57)return "DRIZZLE";
 if(code==61||code==63||code==65||code==66||code==67)return "RAIN";
 if(code==71||code==73||code==75||code==77)return "SNOW";
 if(code==80||code==81||code==82)return "SHOWERS";
 if(code==85||code==86)return "SNOW SHOWERS";
 if(code==95||code==96||code==99)return "THUNDERSTORM";
 return "WEATHER";
}
inline bool parse_current(std::string_view json,Data& out){
 // Limit matches to current object; fields in current_units must not count.
 size_t p=json.find("\"current\"");
 if(p==std::string_view::npos)return false;
 size_t open=json.find('{',p),close=json.find('}',open);
 if(open==std::string_view::npos||close==std::string_view::npos)return false;
 auto obj=json.substr(open,close-open+1);
 double temp=0,weatherCode=-1,humidity=0,wind=0;
 if(!number(obj,"temperature_2m",temp,-110,70)||
    !number(obj,"weather_code",weatherCode,0,99)||
    !number(obj,"relative_humidity_2m",humidity,0,100)||
    !number(obj,"wind_speed_10m",wind,0,400))return false;
 out.temperature=(int)std::lround(temp);
 out.code=(int)std::lround(weatherCode);
 out.humidity=(int)std::lround(humidity);
 out.windKmh=(int)std::lround(wind);
 out.condition=condition(out.code);
 out.valid=true;out.stale=false;
 out.status="LIVE WEATHER";
 return true;
}
} // namespace standby::weather
