// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// ShadowMountPlus 1.7 official HTTP API: verified game sources only.
// No arbitrary filesystem deletion, shell commands, or direct mount operations.
#include "inventory.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
namespace uninstaller::shadow {
struct Reply {int http=0;std::string body,error;};
inline bool validId(const std::string& id){
  return validTitleId(id) && id!="PPSA99986";
}
inline int number(const std::string& json,const char* key,int fallback=-1){
  std::string tag=std::string("\"")+key+"\"";
  size_t pos=json.find(tag);
  if(pos==std::string::npos)return fallback;
  pos=json.find(':',pos+tag.size());
  if(pos==std::string::npos)return fallback;
  const char* p=json.c_str()+pos+1;
  while(*p==' '||*p=='\t')++p;
  if(*p<'0'||*p>'9')return fallback;
  const long n=std::strtol(p,nullptr,10);
  return n>=0&&n<=2147483647?(int)n:fallback;
}
inline bool flag(const std::string& json,const char* key){
  std::string tag=std::string("\"")+key+"\"";
  size_t p=json.find(tag);
  if(p==std::string::npos)return false;
  p=json.find(':',p+tag.size());
  if(p==std::string::npos)return false;
  p=json.find_first_not_of(" \t",p+1);
  return p!=std::string::npos&&json.compare(p,4,"true")==0;
}
inline Reply post(const char* route,const std::string& json){
  Reply r;
  if(!route||std::strlen(route)>92||route[0]!='/'||
     json.size()>250)return {0,"","Unsupported API request"};
  int fd=socket(AF_INET,SOCK_STREAM,0);
  if(fd<0)return {0,"","Cannot open local API socket"};
  timeval timeout{3,0};
  setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof timeout);
  setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof timeout);
  sockaddr_in addr{};addr.sin_family=AF_INET;
  addr.sin_port=htons(10101);addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  if(connect(fd,(sockaddr*)&addr,sizeof addr)!=0){
    close(fd);return {0,"","ShadowMountPlus 1.7 API not reachable on 127.0.0.1:10101"};
  }
  const std::string req=std::string("POST ")+route+
     " HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Type: application/json\r\n"+
     "Connection: close\r\nContent-Length: "+std::to_string(json.size())+
     "\r\n\r\n"+json;
  size_t off=0;
  while(off<req.size()){
    ssize_t n=send(fd,req.data()+off,req.size()-off,0);
    if(n<=0){close(fd);return {0,"","Cannot send ShadowMount request"};}
    off+=(size_t)n;
  }
  std::string response;response.reserve(4096);
  char buffer[4096];
  while(response.size()<128*1024){
    const ssize_t n=recv(fd,buffer,sizeof buffer,0);
    if(n<=0)break;
    response.append(buffer,(size_t)n);
  }
  close(fd);
  const size_t split=response.find("\r\n\r\n");
  if(split==std::string::npos||
     std::sscanf(response.c_str(),"HTTP/%*d.%*d %d",&r.http)!=1){
    r.error="Invalid ShadowMount HTTP response";return r;
  }
  r.body=response.substr(split+4);
  if(r.http<200||r.http>=300){
    const std::string detail=field(r.body,"error");
    r.error="ShadowMount HTTP "+std::to_string(r.http)+(detail.empty()?"":": "+detail);
  }
  return r;
}
struct Source {
  bool valid=false;
  std::string title,path,type,error;
};
inline Source parseSource(const std::string& body,const std::string& requested){
  Source s;
  s.title=field(body,"title_id");
  s.path=field(body,"path");
  s.type=field(body,"source_type");
  if(number(body,"status")!=0){s.error="Game info returned an error";return s;}
  if(s.title!=requested || !validId(requested)){
    s.error="ShadowMount title ID mismatch";return s;
  }
  if(!flag(body,"can_manage_source") || !flag(body,"source_available") ||
     (s.type!="folder"&&s.type!="image") || s.path.empty()||
     s.path[0]!='/' || s.path.size()>900){
    s.error="No deletable ShadowMount source exists for this title";return s;
  }
  s.valid=true;return s;
}
inline Source inspect(const std::string& title){
  if(!validId(title))return {false,"","","","Invalid title ID"};
  Reply v=post("/api/v1/version","{}");
  if(v.http!=200||number(v.body,"status")!=0||
     v.body.find("\"delete_game_source\"")==std::string::npos)
    return {false,"","","","ShadowMountPlus 1.7 with delete_game_source API required"};
  Reply info=post("/api/v1/games/info",std::string("{\"title_id\":\"")+title+"\"}");
  if(info.http!=200)return {false,"","","",info.error};
  return parseSource(info.body,title);
}
struct DeleteResult{
  bool queued=false,absent=false;
  int job=-1;
  std::string message;
};
inline DeleteResult remove(const std::string& title,const std::string& expected){
  DeleteResult out;
  // Re-validate the physical source immediately before issuing a destructive job.
  const Source source=inspect(title);
  if(!source.valid){out.message=source.error;return out;}
  if(expected.empty()||source.path!=expected){
    out.message="Source path changed. No deletion submitted; inspect again";return out;
  }
  Reply response=post("/api/v1/games/delete",
       std::string("{\"title_id\":\"")+title+"\",\"confirm\":true}");
  if(response.http==202&&number(response.body,"status")==0){
    out.job=number(response.body,"job_id");
    out.queued=out.job>0;
    out.message=out.queued?"ShadowMount source deletion queued (job "+
        std::to_string(out.job)+")":"No valid ShadowMount job identifier";
  }else if(response.http==200&&number(response.body,"status")==0){
    out.absent=true;
    out.message="Source already absent. ShadowMount library reconciliation queued";
  }else out.message=response.error.empty()?"ShadowMount refused source deletion":response.error;
  return out;
}
struct Job{
  bool finished=false,success=false;
  int progress=0;
  std::string label;
};
inline Job status(int job){
  Job result;
  if(job<=0){result.finished=true;result.label="Invalid ShadowMount job";return result;}
  Reply r=post("/api/v1/games/storage/status",
     std::string("{\"job_id\":")+std::to_string(job)+"}");
  if(r.http!=200||number(r.body,"status")!=0){
    result.label=r.error.empty()?"ShadowMount status temporarily unavailable":r.error;
    return result;
  }
  const std::string state=field(r.body,"state");
  result.progress=number(r.body,"progress_percent",0);
  if(result.progress>100)result.progress=100;
  result.finished=(state=="completed"||state=="failed"||state=="cancelled");
  result.success=state=="completed"&&number(r.body,"result_status",1)==0;
  result.label=result.success?"ShadowMount source deleted; library rescan pending":
    state=="failed"?"ShadowMount delete failed: "+field(r.body,"result_error"):
    state=="cancelled"?"ShadowMount source delete cancelled":
    "ShadowMount source: "+state+" "+std::to_string(result.progress)+"%";
  return result;
}
} // namespace uninstaller::shadow
