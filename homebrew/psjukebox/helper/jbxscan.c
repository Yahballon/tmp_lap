// SPDX-License-Identifier: GPL-3.0-or-later
// Read-only, loopback-only PSJukebox media bridge. Launched by existing elfldr.
#define _DEFAULT_SOURCE 1
#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#ifndef JUKEBOX_PORT
#define JUKEBOX_PORT 9188
#endif
#ifndef JUKEBOX_TEST_ROOT
static const char *roots[]={
 "/mnt/usb0/PSJukebox/Music","/mnt/usb1/PSJukebox/Music",
 "/mnt/usb2/PSJukebox/Music","/mnt/usb3/PSJukebox/Music",
 "/usb0/PSJukebox/Music","/usb1/PSJukebox/Music",
 "/mnt/usb0/Music","/mnt/usb1/Music",
 "/data/PSJukebox/Music","/download0/PSJukebox/Music"
};
#else
static const char *roots[]={JUKEBOX_TEST_ROOT};
#endif
#define ROOTS (sizeof(roots)/sizeof(roots[0]))
#define LIMIT_FILES 12000
#define LIMIT_DIRS 25000
#define MAX_REL 512
#define MAX_MEDIA (8ULL*1024*1024*1024)
#define MAX_ART (12ULL*1024*1024)
static unsigned catalog_files=0,catalog_dirs=0,catalog_artists=0;
static int send_all(int fd,const void *p,size_t n){
 const char *s=(const char*)p;
 while(n){ssize_t w=send(fd,s,n,0);if(w<=0)return 0;n-=(size_t)w;s+=w;}return 1;
}
static int print_all(int fd,const char *s){return send_all(fd,s,strlen(s));}
static int valid_part(const char *s){
 size_t n=strlen(s);
 if(!n||n>180||s[0]=='.')return 0;
 for(const unsigned char *p=(const unsigned char*)s;*p;p++)
  if(*p<32||*p==127||*p=='/'||*p=='\\')return 0;
 return 1;
}
static int safe_relative(const char *path){
 size_t n=strlen(path);if(!n||n>MAX_REL-1||path[0]=='/'||path[n-1]=='/')return 0;
 const char *p=path;
 while(*p){
  const char *e=strchr(p,'/');size_t length=e?(size_t)(e-p):strlen(p);
  if(!length||length>180||*p=='.')return 0;
  for(size_t i=0;i<length;i++)if((unsigned char)p[i]<32||p[i]==127||p[i]=='\\')return 0;
  if(!e)break;
  p=e+1;
 }
 return 1;
}
static int ends(const char *s,const char *ext){
 size_t a=strlen(s),b=strlen(ext);return a>=b&&!strcasecmp(s+a-b,ext);
}
static int kind(const char *n){
 if(ends(n,".mp3")||ends(n,".flac")||ends(n,".m4a")||
    ends(n,".aac")||ends(n,".wav")||ends(n,".ogg")||
    ends(n,".opus")||ends(n,".wma"))return 2;
 if(ends(n,".png")||ends(n,".jpg")||ends(n,".jpeg"))return 1;
 if(!strcasecmp(n,"theme.ini"))return 3;
 if(ends(n,".album"))return 4;
 return 0;
}
static int secure_file(unsigned root,const char *relative,uint64_t *size){
 if(root>=ROOTS||!safe_relative(relative))return 0;
 char full[1100];size_t base=strlen(roots[root]);
 if(base+strlen(relative)+2>sizeof(full))return 0;
 memcpy(full,roots[root],base+1);
 char copy[MAX_REL];strcpy(copy,relative);
 char *save=NULL,*p=strtok_r(copy,"/",&save);
 unsigned depth=0;struct stat st;
 while(p){
  if(!valid_part(p))return 0;
  size_t len=strlen(full);
  if(len+strlen(p)+2>sizeof(full))return 0;
  full[len]='/';strcpy(full+len+1,p);
  if(lstat(full,&st)!=0||S_ISLNK(st.st_mode))return 0;
  p=strtok_r(NULL,"/",&save);
  if(p&&!S_ISDIR(st.st_mode))return 0;
  ++depth;
 }
 if(depth<2||depth>3||!S_ISREG(st.st_mode)||st.st_size<1)return 0;
 const char *name=strrchr(relative,'/');name=name?name+1:relative;
 int t=kind(name);
 if(!t||(t==2&&depth!=2&&depth!=3)||(t==3&&depth!=2)||(t==4&&depth!=1&&depth!=2))return 0;
 uint64_t limit=t==2?MAX_MEDIA:(t==1?MAX_ART:(t==4?1024ULL*1024*1024:4096));
 if((uint64_t)st.st_size>limit)return 0;
 *size=(uint64_t)st.st_size;return 1;
}
static void enumerate(int fd,unsigned root,const char *dir,const char *rel,unsigned depth){
 if(depth>2||catalog_dirs++>LIMIT_DIRS||catalog_files>=LIMIT_FILES)return;
 DIR *d=opendir(dir);if(!d)return;
 struct dirent *e;
 while((e=readdir(d))&&catalog_files<LIMIT_FILES){
  if(!valid_part(e->d_name))continue;
  char path[1100],next[MAX_REL];
  int n=snprintf(path,sizeof(path),"%s/%s",dir,e->d_name);
  int q=snprintf(next,sizeof(next),"%s%s%s",rel,*rel?"/":"",e->d_name);
  if(n<=0||n>=(int)sizeof(path)||q<=0||q>=(int)sizeof(next))continue;
  struct stat st;if(lstat(path,&st)!=0||S_ISLNK(st.st_mode))continue;
  if(S_ISDIR(st.st_mode)&&depth<2){
   char marker[700];
   // Enumerate directories separately: never hide an artist with no tracks.
   int sz=snprintf(marker,sizeof(marker),"%c %u %s\n",depth==0?'A':'D',root,next);
   if(sz>0&&sz<(int)sizeof(marker)){
    send_all(fd,marker,(size_t)sz);
    if(depth==0)catalog_artists++;
   }
   enumerate(fd,root,path,next,depth+1);
   continue;
  }
  if(!S_ISREG(st.st_mode))continue;
  uint64_t size=0;
  if(!secure_file(root,next,&size))continue;
  char line[720];
  n=snprintf(line,sizeof(line),"F %u %llu %s\n",root,(unsigned long long)size,next);
  if(n>0&&n<(int)sizeof(line)&&send_all(fd,line,(size_t)n))catalog_files++;
 }
 closedir(d);
}
static void listing(int fd){
 catalog_files=catalog_dirs=catalog_artists=0;print_all(fd,"JBOX1\n");
 for(unsigned i=0;i<ROOTS;i++){
  struct stat st;if(lstat(roots[i],&st)!=0||!S_ISDIR(st.st_mode))continue;
  char line[700];int n=snprintf(line,sizeof(line),"R %u %s\n",i,roots[i]);
  if(n>0&&n<(int)sizeof(line))send_all(fd,line,(size_t)n);
  enumerate(fd,i,roots[i],"",0);
 }
 if(!catalog_artists)print_all(fd,"ERROR No accessible artist folders found\n");
 print_all(fd,"DONE\n");
}
static void transmit(int fd,unsigned root,const char *relative,uint64_t offset){
 uint64_t size=0;
 if(!secure_file(root,relative,&size)||offset>size){print_all(fd,"ERROR Not accessible\n");return;}
 char path[1100];int n=snprintf(path,sizeof(path),"%s/%s",roots[root],relative);
 if(n<1||n>=(int)sizeof(path)){print_all(fd,"ERROR Path\n");return;}
 FILE* f=fopen(path,"rb");
 if(!f){print_all(fd,"ERROR Open failed\n");return;}
 if(fseeko(f,(off_t)offset,SEEK_SET)){fclose(f);print_all(fd,"ERROR Seek\n");return;}
 char line[96];n=snprintf(line,sizeof(line),"DATA %llu\n",(unsigned long long)(size-offset));
 if(n<=0||!send_all(fd,line,(size_t)n)){fclose(f);return;}
 unsigned char bytes[16384];uint64_t left=size-offset;
 while(left){
  size_t ask=left<sizeof(bytes)?(size_t)left:sizeof(bytes);
  size_t got=fread(bytes,1,ask,f);
  if(!got||!send_all(fd,bytes,got))break;
  left-=got;
 }
 fclose(f);
}
static void serve(void){
 signal(SIGPIPE,SIG_IGN);
 int s=socket(AF_INET,SOCK_STREAM,0);if(s<0)return;
 int opt=1;setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
 struct sockaddr_in address={0};address.sin_family=AF_INET;
 address.sin_port=htons(JUKEBOX_PORT);address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
 if(bind(s,(struct sockaddr*)&address,sizeof(address))!=0||listen(s,4)!=0){close(s);return;}
 for(unsigned count=0;count<4000;count++){
  fd_set set;FD_ZERO(&set);FD_SET(s,&set);
  struct timeval tv={600,0};
  if(select(s+1,&set,NULL,NULL,&tv)<=0)break;
  int fd=accept(s,NULL,NULL);if(fd<0)continue;
  struct timeval short_timeout={40,0};
  setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&short_timeout,sizeof(short_timeout));
  setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&short_timeout,sizeof(short_timeout));
  char command[800]={0};size_t used=0;int done=0;
  while(used+1<sizeof(command)){
   ssize_t n=recv(fd,command+used,1,0);if(n<=0)break;
   if(command[used]=='\n'){command[used]=0;done=1;break;}
   used++;
  }
  if(done){
   if(!strcmp(command,"LIST"))listing(fd);
   else{
    unsigned root=0;unsigned long long offset=0;int n=0;
    if(sscanf(command,"GET %u %n",&root,&n)==1&&n>0)
     transmit(fd,root,command+n,0);
    else if(sscanf(command,"FETCH %u %llu %n",&root,&offset,&n)==2&&n>0)
     transmit(fd,root,command+n,(uint64_t)offset);
    else print_all(fd,"ERROR Unsupported command\n");
   }
  }
  close(fd);
 }
 close(s);
}
int main(void){
 setvbuf(stdout,NULL,_IONBF,0);
 // Existing elfldr accepts a launched ELF and remains outside the sandbox.
 // Its stdout is not a reliable transport: serve a dedicated localhost port.
 puts("PSJukebox helper starting on loopback");
 serve();
 return 0;
}
