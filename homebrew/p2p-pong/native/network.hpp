#pragma once
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
namespace pongnet {
class Client {
 int fd=-1;std::string pending,outgoing;
public:
 std::string status="Offline";
 ~Client(){close();}
 void close(){if(fd>=0)::close(fd);fd=-1;pending.clear();outgoing.clear();}
 bool connect_to(const std::string& host,int port){
  close();if(host.empty()||port<1||port>65535)return false;
  struct addrinfo hint{};hint.ai_family=AF_INET;hint.ai_socktype=SOCK_STREAM;
  struct addrinfo* list=nullptr;
  if(getaddrinfo(host.c_str(),std::to_string(port).c_str(),&hint,&list)!=0){status="DNS failed";return false;}
  for(auto* p=list;p;p=p->ai_next){
   int s=::socket(p->ai_family,p->ai_socktype,p->ai_protocol);
   if(s<0)continue;
   int flags=fcntl(s,F_GETFL,0);fcntl(s,F_SETFL,flags|O_NONBLOCK);
   int rc=::connect(s,p->ai_addr,p->ai_addrlen);
   if(rc==0){fd=s;status="Connected";break;}
   if(errno==EINPROGRESS){
    fd_set wf;FD_ZERO(&wf);FD_SET(s,&wf);timeval tv{2,0};
    if(select(s+1,nullptr,&wf,nullptr,&tv)>0){
     int err=0;socklen_t n=sizeof(err);
     if(getsockopt(s,SOL_SOCKET,SO_ERROR,&err,&n)==0&&err==0){fd=s;status="Connected";break;}
    }
   }
   ::close(s);
  }
  freeaddrinfo(list);
  if(fd<0)status="Connection failed";
  return fd>=0;
 }
 bool online()const{return fd>=0;}
 void send(const std::string& line){if(fd>=0&&line.size()<450)outgoing+=line+"\n";}
 std::vector<std::string> poll(){
  std::vector<std::string> result;
  if(fd<0)return result;
  if(!outgoing.empty()){
   auto n=::send(fd,outgoing.data(),outgoing.size(),MSG_NOSIGNAL|MSG_DONTWAIT);
   if(n>0)outgoing.erase(0,(size_t)n);
   else if(n<0&&errno!=EWOULDBLOCK&&errno!=EAGAIN){close();status="Disconnected";return result;}
  }
  char b[1024];
  for(int k=0;k<8;k++){
   auto n=::recv(fd,b,sizeof(b),MSG_DONTWAIT);
   if(n==0){close();status="Disconnected";break;}
   if(n<0){if(errno!=EWOULDBLOCK&&errno!=EAGAIN){close();status="Network error";}break;}
   pending.append(b,(size_t)n);
   if(pending.size()>8192){close();status="Frame too large";break;}
   size_t p;
   while((p=pending.find('\n'))!=std::string::npos){
    auto line=pending.substr(0,p);pending.erase(0,p+1);
    if(line.size()<512)result.push_back(line);
   }
  }
  return result;
 }
};
}
