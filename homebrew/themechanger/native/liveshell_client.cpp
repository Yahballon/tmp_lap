// SPDX-License-Identifier: GPL-3.0-or-later
// Bridge to ZiZc3/LiveShell v1.0.  LiveShell and this app are GPL-3.0 licensed.
#include "liveshell_client.hpp"
#include "theme_log.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
namespace theme {
const char* const kNames[kCount]={"Baseball","Anime","Gaming","Cyber City","Racing","Space","Samurai","Retro Neon","Mountains","Forest","Beach","Snow"};
namespace {
const char* const kFiles[kCount]={"baseball","anime","gaming","cyber-city","racing","space","samurai","retro-neon","mountains","forest","beach","snow"};
bool ensure_dir(const std::string& path){return mkdir(path.c_str(),0755)==0||errno==EEXIST;}
std::string http_get(const std::string& route){
 int fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0)return {};
 timeval tv{};tv.tv_sec=2;setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
 setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
 sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_port=htons(9050);addr.sin_addr.s_addr=htonl(0x7f000001);
 if(connect(fd,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))!=0){close(fd);return {};}
 const std::string req="GET "+route+" HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
 size_t sent=0;
 while(sent<req.size()){ssize_t n=send(fd,req.data()+sent,req.size()-sent,0);if(n<=0){close(fd);return {};}sent+=static_cast<size_t>(n);}
 std::string output;char chunk[4096];ssize_t got=0;
 while((got=recv(fd,chunk,sizeof(chunk),0))>0&&output.size()<262144)
   output.append(chunk,static_cast<size_t>(got));
 close(fd);
 if(output.rfind("HTTP/1.1 200",0)!=0)return {};
 const auto at=output.find("\r\n\r\n");
 return at==std::string::npos?std::string{}:output.substr(at+4);
}
bool copy_clip(const std::string& source,const std::string& dest,std::string& error){
 FILE* in=fopen(source.c_str(),"rb");
 if(!in){error="Wallpaper clip missing from application assets";return false;}
 if(!ensure_dir("/data/LiveShell")||!ensure_dir(kLiveRoot)){fclose(in);error="Cannot create LiveShell video folder";return false;}
 // Never overwrite another app's clip; all our files live under ThemeChanger-.
 const std::string temp=dest+".part";
 FILE* out=fopen(temp.c_str(),"wb");
 if(!out){fclose(in);error="Cannot write LiveShell video folder";return false;}
 char buf[65536];bool ok=true;size_t n;
 while((n=fread(buf,1,sizeof(buf),in))>0)
   if(fwrite(buf,1,n,out)!=n){ok=false;break;}
 if(ferror(in))ok=false;
 if(fflush(out)!=0)ok=false;
 if(fclose(out)!=0)ok=false;
 fclose(in);
 if(!ok||rename(temp.c_str(),dest.c_str())!=0){unlink(temp.c_str());error="Cannot finish wallpaper copy";return false;}
 return true;
}
int find_index(const std::string& json,const std::string& name){
 const auto videos=json.find("\"videos\":[");
 if(videos==std::string::npos)return -1;
 auto pos=videos+10;int idx=0;
 while((pos=json.find("\"name\":\"",pos))!=std::string::npos){
   pos+=8;const auto end=json.find('"',pos);
   if(end==std::string::npos)return -1;
   if(json.substr(pos,end-pos)==name)return idx;
   ++idx;pos=end+1;
 }
 return -1;
}
}
std::string theme_file(int index){
 if(index<0||index>=kCount)return {};
 return std::string("ThemeChanger-")+kFiles[index]+".mp4";
}
bool probe(){return !http_get("/api/state").empty();}
std::string current_file(){
 const std::string state=http_get("/api/state");
 const auto key=state.find("\"current\":\"");
 if(key==std::string::npos)return {};
 const auto first=key+11;
 const auto end=state.find('"',first);
 if(end==std::string::npos)return {};
 const std::string full=state.substr(first,end-first);
 const auto slash=full.find_last_of('/');
 return slash==std::string::npos?full:full.substr(slash+1);
}
int current_theme(){
 const auto state=http_get("/api/state");const auto key=state.find("\"current\":\"");
 if(key==std::string::npos)return -1;
 const auto last=state.find('"',key+11);
 if(last==std::string::npos)return -1;
 const auto selected=state.substr(key+11,last-key-11);
 for(int i=0;i<kCount;i++)if(selected.find(theme_file(i))!=std::string::npos)return i;
 return -1;
}
bool install_and_select(int index,const std::string& assets,std::string& result){
 if(index<0||index>=kCount){result="Invalid theme";return false;}
 const std::string filename=theme_file(index);
 const std::string output=std::string(kLiveRoot)+"/"+filename;
 struct stat st{};
 if(stat(output.c_str(),&st)!=0||st.st_size<1024){
   if(!copy_clip(assets+"/wallpapers/"+filename,output,result))return false;
 }
 std::string state=http_get("/api/state");
 if(state.empty()){result="Wallpaper installed. Start LiveShell payload, then press X again.";return false;}
 int found=find_index(state,filename);
 if(found<0){result="Wallpaper copied. Restart LiveShell to refresh its library.";return false;}
 const auto reply=http_get("/api/select?i="+std::to_string(found));
 if(reply.find("\"ok\":true")==std::string::npos){result="LiveShell rejected wallpaper selection";return false;}
 save_selected(index);result=std::string(kNames[index])+" applied by LiveShell";tc::log("APPLY_BUILTIN",filename.c_str());return true;
}
bool activate_file(const std::string& filename,std::string& result){
 if(filename.empty()||filename.size()>120||filename.find('/')!=std::string::npos||
   filename.find('\\')!=std::string::npos||filename.find("..")!=std::string::npos){
   result="Invalid wallpaper filename";return false;
 }
 const auto state=http_get("/api/state");
 if(state.empty()){result="LiveShell offline; wallpaper saved";return false;}
 int idx=find_index(state,filename);
 if(idx<0){result="LiveShell did not list the new MP4 yet";return false;}
 const auto reply=http_get("/api/select?i="+std::to_string(idx));
 if(reply.find("\"ok\":true")==std::string::npos){
   result="LiveShell could not activate this video";return false;
 }
 result="Wallpaper applied";tc::log("APPLY_ONLINE",filename.c_str());return true;
}
bool send_setting(const std::string& route,std::string& result){
 if(route.empty()||route[0]!='/'||route.find("..")!=std::string::npos){result="Invalid API route";return false;}
 const auto body=http_get(route);
 if(body.empty()){result="LiveShell not running (load liveshell.elf)";return false;}
 if(body.find("\"ok\":true")==std::string::npos){result="LiveShell rejected setting";return false;}
 result="LiveShell setting updated";return true;
}
int load_selected(){
 FILE* f=fopen("/data/ThemeChanger/selected.txt","rb");if(!f)return 1;
 int i=1;if(fscanf(f,"%d",&i)!=1)i=1;fclose(f);return i>=0&&i<kCount?i:1;
}
void save_selected(int index){
 if(index<0||index>=kCount||!ensure_dir(kDataRoot))return;
 FILE* f=fopen("/data/ThemeChanger/selected.txt","wb");if(!f)return;
 fprintf(f,"%d\n",index);fclose(f);
}
}
