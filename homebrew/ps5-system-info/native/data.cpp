// SPDX-License-Identifier: GPL-3.0-or-later
// Read-only diagnostics: NEVER substitute nominal console specifications for sampled telemetry.
#include "data.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
namespace info {
namespace {
constexpr const char* dir="/data/ps5-system-info";
constexpr const char* settingsFile="/data/ps5-system-info/settings.ini";
constexpr const char* logFile="/data/ps5-system-info/system-info.log";
constexpr const char* exportFile="/data/ps5-system-info/report.txt";
void row(std::vector<Row>& dst,const char* key,std::string val,bool ok=true){
 dst.push_back({key,std::move(val),ok});
}
void na(std::vector<Row>& dst,const char* key){row(dst,key,"Unavailable",false);}
std::string number(long long n){return std::to_string(n);}
bool ensureDir(){return mkdir(dir,0755)==0 || errno==EEXIST;}
std::string readText(const std::string& p,size_t max=16384){
 FILE* f=std::fopen(p.c_str(),"rb");if(!f)return {};
 std::string s;char b[512];size_t count=0;
 while(count<max){size_t n=std::fread(b,1,std::min(sizeof(b),max-count),f);if(!n)break;s.append(b,n);count+=n;}
 std::fclose(f);return s;
}
std::string jsonTitle(const std::string& s){
 size_t at=s.find("\"titleName\"");if(at==std::string::npos)return {};
 at=s.find(':',at);if(at==std::string::npos)return {};
 at=s.find('"',at);if(at==std::string::npos)return {};
 ++at;std::string name;
 for(;at<s.size()&&name.size()<100;at++){
   if(s[at]=='"')break;
   if(s[at]=='\\'&&at+1<s.size()){++at;if(s[at]=='n'||s[at]=='r')name+=' ';else name+=s[at];}
   else if((unsigned char)s[at]>=32)name+=s[at];
 }
 return name;
}
bool isFolder(const std::string& path){
 struct stat st{};return stat(path.c_str(),&st)==0 && S_ISDIR(st.st_mode);
}
std::string activeIp(){
 // UDP connect only asks the routing table for an outgoing interface; no datagram is sent.
 int fd=socket(AF_INET,SOCK_DGRAM,0);if(fd<0)return {};
 sockaddr_in target{};target.sin_family=AF_INET;target.sin_port=htons(53);
 target.sin_addr.s_addr=htonl(0x01010101u);
 std::string ip;
 if(connect(fd,(const sockaddr*)&target,sizeof(target))==0){
   sockaddr_in local{};socklen_t n=sizeof(local);
   if(getsockname(fd,(sockaddr*)&local,&n)==0){
     const unsigned char* p=(const unsigned char*)&local.sin_addr.s_addr;
     char buf[40];std::snprintf(buf,sizeof(buf),"%u.%u.%u.%u",p[0],p[1],p[2],p[3]);
     if(std::strcmp(buf,"0.0.0.0"))ip=buf;
   }
 }
 close(fd);return ip;
}
std::string formatTime(){
 std::time_t now=std::time(nullptr);if(now<=0)return {};
 std::tm tm{};if(localtime_s(&tm,&now)!=0)return {};
 char b[80];if(!std::strftime(b,sizeof(b),"%Y-%m-%d %H:%M:%S",&tm))return {};
 return b;
}
std::uint64_t mono(){
 timespec ts{};if(clock_gettime(CLOCK_MONOTONIC,&ts)!=0||ts.tv_sec<0)return 0;
 return (std::uint64_t)ts.tv_sec;
}
std::string formatUptime(std::uint64_t v){
 if(!v)return {};
 char b[90];std::snprintf(b,sizeof(b),"%llud %02lluh %02llum %02llus",
  (unsigned long long)(v/86400),(unsigned long long)((v/3600)%24),
  (unsigned long long)((v/60)%60),(unsigned long long)(v%60));
 return b;
}
}
std::string unavailable(){return "Unavailable";}
std::string compactBytes(std::uint64_t x){
 char b[60];double n=(double)x;int unit=0;const char* units[]={"B","KiB","MiB","GiB","TiB"};
 while(n>=1024&&unit<4){n/=1024;unit++;}
 std::snprintf(b,sizeof(b),"%.1f %s",n,units[unit]);return b;
}
Snapshot collect(int w,int h,bool connected){
 Snapshot s;
 s.time=formatTime();
 s.uptimeSeconds=mono();s.uptime=formatUptime(s.uptimeSeconds);
 s.clockSource="CLOCK_MONOTONIC";
 s.controller=connected;
 s.ownPid=(int)getpid();
 rusage usage{};
 if(getrusage(RUSAGE_SELF,&usage)==0){
  s.ownCpuSeconds=usage.ru_utime.tv_sec+usage.ru_utime.tv_usec/1e6+
    usage.ru_stime.tv_sec+usage.ru_stime.tv_usec/1e6;
  s.ownPeakRss=usage.ru_maxrss;
 }
 s.localIp=activeIp();
 s.route=s.localIp.empty()?"No route detected":"Local route detected";
 row(s.system,"Console family","PlayStation 5 (homebrew platform)");
 na(s.system,"Model / SKU");na(s.system,"Hardware region");
 na(s.system,"System software / firmware");na(s.system,"Firmware build");
 na(s.system,"System SDK version");na(s.system,"Kernel version");
 row(s.system,"System time",s.time.empty()?unavailable():s.time,!s.time.empty());
 row(s.system,"Uptime (monotonic)",s.uptime.empty()?unavailable():s.uptime,!s.uptime.empty());
 row(s.system,"App Title ID","PPSA99786");
 row(s.system,"App version","v0.1.0");
 na(s.system,"Jailbreak status (verified)");
 row(s.system,"Execution","Native homebrew process");
 row(s.hardware,"CPU design","AMD Zen 2 / 8 cores (hardware specification)");
 row(s.hardware,"GPU design","AMD RDNA 2 (hardware specification)");
 row(s.hardware,"Memory design","16 GiB GDDR6 (hardware specification)");
 // CPU/GPU hardware descriptions above are nominal specs, not direct hardware probes.
 na(s.hardware,"CPU usage (system)");na(s.hardware,"CPU clock (live)");
 na(s.hardware,"GPU usage / frequency");na(s.hardware,"Total RAM used / free");
 na(s.hardware,"Dedicated VRAM usage");
 na(s.hardware,"CPU temperature");na(s.hardware,"GPU temperature");
 na(s.hardware,"Mainboard temperature");
 row(s.hardware,"This app CPU time",std::to_string((int)s.ownCpuSeconds)+" s");
 if(s.ownPeakRss>0)row(s.hardware,"This app max RSS (OS units)",number(s.ownPeakRss));
 else na(s.hardware,"This app max RSS");
 const char* roots[]={"/data","/data/homebrew","/mnt/usb0","/mnt/usb1","/mnt/ext0","/download0"};
 const char* labels[]={"Internal data","Homebrew directory","USB0 mount","USB1 mount","External mount","Download0 mount"};
 for(int i=0;i<6;i++){
  if(isFolder(roots[i])){row(s.storage,labels[i],std::string("Accessible: ")+roots[i]);s.storageNodes++;}
  else row(s.storage,labels[i],"Not accessible",false);
 }
 na(s.storage,"Internal SSD total / free");na(s.storage,"M.2 total / free");
 na(s.storage,"USB capacity / used");na(s.storage,"Filesystem capacity (no verified API)");
 row(s.network,"IPv4 egress interface",s.localIp.empty()?unavailable():s.localIp,!s.localIp.empty());
 row(s.network,"Route probe",s.route,!s.localIp.empty());
 na(s.network,"Network type (Wi-Fi / Ethernet)");na(s.network,"Gateway");
 na(s.network,"Subnet mask");na(s.network,"DNS servers");
 na(s.network,"MAC address");na(s.network,"Public IP");
 na(s.network,"NAT type");na(s.network,"Download speed");
 na(s.network,"Upload speed");
 if(w>0&&h>0)row(s.video,"App framebuffer",number(w)+" x "+number(h));
 else na(s.video,"App framebuffer");
 na(s.video,"Connected TV / monitor");na(s.video,"HDMI EDID");
 na(s.video,"Output color format / depth");na(s.video,"Actual TV refresh rate");
 na(s.video,"HDR enabled");na(s.video,"VRR enabled");
 na(s.video,"HDCP version");
 na(s.audio,"HDMI output device");na(s.audio,"Output channels");
 na(s.audio,"Output sample rate");na(s.audio,"Codec / bitrate");
 na(s.audio,"3D Audio status");na(s.audio,"Mic / headset status");
 row(s.peripherals,"Primary gamepad",connected?"Connected":"Disconnected",connected);
 na(s.peripherals,"Controller battery %");
 na(s.peripherals,"USB device enumeration");na(s.peripherals,"Bluetooth devices");
 na(s.peripherals,"Connected keyboard / mouse");
 na(s.power,"Current power draw");na(s.power,"Console power mode");
 na(s.power,"Fan speed / RPM");na(s.power,"Fan control mode");
 na(s.power,"Power rail voltage");na(s.power,"Thermal throttling");
 na(s.power,"Standby timer");na(s.power,"CE error state");
 row(s.processes,"Current application PID",number(s.ownPid));
 row(s.processes,"Current application CPU time",std::to_string((int)s.ownCpuSeconds)+" s");
 na(s.processes,"Other processes / PIDs");na(s.processes,"Global process CPU %");
 na(s.processes,"Global process memory usage");
 row(s.kernel,"App runtime","PS5 native homebrew");
 na(s.kernel,"Exact running kernel version");na(s.kernel,"Kernel module list");
 na(s.kernel,"Kernel patches / offsets");na(s.kernel,"Privilege / exploit status");
 na(s.kernel,"Kernel ring buffer");
 na(s.kernel,"Debug UART / console access");
 // Enumerate only user-visible Homebrew installations, never silently enumerate Sony titles.
 DIR* root=opendir("/data/homebrew");
 if(root){
  struct dirent* e;
  while((e=readdir(root))&&s.homebrewFolders<350){
   const std::string id=e->d_name;
   if(id=="."||id==".."||id.size()>110||id.find('/')!=std::string::npos)continue;
   const std::string path="/data/homebrew/"+id;
   if(!isFolder(path))continue;
   s.homebrewFolders++;
   struct stat st{};std::uint64_t bytes=0;
   if(stat((path+"/eboot.bin").c_str(),&st)==0&&S_ISREG(st.st_mode)){
    bytes=(std::uint64_t)st.st_size;
    s.homebrewExecutables++;
   }else continue;
   std::string title=jsonTitle(readText(path+"/sce_sys/param.json"));
   if(title.empty())title=id;
   s.apps.push_back({title,id,bytes});
  }
  closedir(root);
 }
 std::sort(s.apps.begin(),s.apps.end(),[](const App& a,const App& b){return a.title<b.title;});
 for(const auto& line:recentLogs())row(s.logs,"Log",line);
 return s;
}
Settings loadSettings(){
 Settings s;FILE* f=std::fopen(settingsFile,"rb");if(!f)return s;
 char line[128];int format=0,interval=s.refreshSeconds,unavailable=s.showUnavailable,theme=s.theme;
 if(std::fgets(line,sizeof(line),f)&&std::sscanf(line,"format=%d",&format)==1&&format==1){
  while(std::fgets(line,sizeof(line),f)){
   int v=0;
   if(std::sscanf(line,"refresh=%d",&v)==1)interval=v;
   if(std::sscanf(line,"show_unavailable=%d",&v)==1)unavailable=v;
   if(std::sscanf(line,"theme=%d",&v)==1)theme=v;
  }
  if(interval==0||interval==5||interval==10||interval==30||interval==60)s.refreshSeconds=interval;
  s.showUnavailable=unavailable!=0;
  if(theme>=0&&theme<=1)s.theme=theme;
 }
 std::fclose(f);return s;
}
bool saveSettings(const Settings& s){
 if(!ensureDir())return false;
 std::string p=std::string(settingsFile)+".tmp";
 FILE* f=std::fopen(p.c_str(),"wb");if(!f)return false;
 bool ok=std::fprintf(f,"format=1\nrefresh=%d\nshow_unavailable=%d\ntheme=%d\n",s.refreshSeconds,s.showUnavailable?1:0,s.theme)>0;
 if(std::fflush(f)!=0||fsync(fileno(f))!=0)ok=false;
 if(std::fclose(f)!=0)ok=false;
 if(!ok||std::rename(p.c_str(),settingsFile)!=0){std::remove(p.c_str());return false;}
 return true;
}
bool appendLog(const std::string& line){
 if(!ensureDir())return false;
 struct stat st{};
 if(stat(logFile,&st)==0&&st.st_size>256*1024){
  std::remove("/data/ps5-system-info/system-info.log.1");
  std::rename(logFile,"/data/ps5-system-info/system-info.log.1");
 }
 FILE* f=std::fopen(logFile,"ab");if(!f)return false;
 std::string clean=line.substr(0,240);
 for(char& c:clean)if(c=='\n'||c=='\r')c=' ';
 const bool ok=std::fprintf(f,"%s %s\n",formatTime().c_str(),clean.c_str())>0;
 std::fclose(f);return ok;
}
std::vector<std::string> recentLogs(){
 std::string s=readText(logFile,128*1024);std::vector<std::string> lines;
 size_t at=0;while(at<s.size()){
  size_t n=s.find('\n',at);if(n==std::string::npos)n=s.size();
  if(n>at)lines.push_back(s.substr(at,std::min(n-at,(size_t)150)));
  at=n+1;
 }
 if(lines.size()>160)lines.erase(lines.begin(),lines.end()-160);
 return lines;
}
bool exportReport(const Snapshot& s,const Settings& settings,std::string& outPath){
 if(!ensureDir())return false;
 const std::string tmp=std::string(exportFile)+".tmp";
 FILE* f=std::fopen(tmp.c_str(),"wb");if(!f)return false;
 std::fprintf(f,"PS5 SYSTEM INFO by YAHBALLON - v0.1.0\nTime: %s\nRead-only diagnostics; unavailable means no verified native API.\n\n",s.time.c_str());
 const std::pair<const char*,const std::vector<Row>*> groups[]={
  {"SYSTEM",&s.system},{"HARDWARE",&s.hardware},{"STORAGE",&s.storage},{"NETWORK",&s.network},
  {"VIDEO",&s.video},{"AUDIO",&s.audio},{"POWER",&s.power},{"PERIPHERALS",&s.peripherals},
  {"PROCESSES",&s.processes},{"KERNEL",&s.kernel}};
 for(const auto& g:groups){
  std::fprintf(f,"[%s]\n",g.first);
  for(const auto& r:*g.second)std::fprintf(f,"%s: %s\n",r.label.c_str(),r.value.c_str());
  std::fputs("\n",f);
 }
 std::fprintf(f,"[HOMEBREW: %d discovered]\n",s.homebrewExecutables);
 for(const auto& app:s.apps)std::fprintf(f,"%s | %s | eboot %s\n",app.id.c_str(),app.title.c_str(),compactBytes(app.bytes).c_str());
 std::fprintf(f,"\n[SETTINGS]\nRefresh: %d sec\nShow unavailable: %s\nTheme: %d\n",settings.refreshSeconds,settings.showUnavailable?"yes":"no",settings.theme);
 bool ok=(std::fflush(f)==0);
 if(fsync(fileno(f))!=0)ok=false;
 if(std::fclose(f)!=0)ok=false;
 if(ok&&std::rename(tmp.c_str(),exportFile)==0){outPath=exportFile;return true;}
 std::remove(tmp.c_str());return false;
}
} // namespace info
