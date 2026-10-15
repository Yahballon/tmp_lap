// SPDX-License-Identifier: GPL-3.0-or-later
// Read-only USB and local M4B bridge, launched by an already-running ELF loader.
// Does not patch the kernel, elevate the UI process, or write to the USB.
#define _DEFAULT_SOURCE 1
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#ifndef AUDIOBOOK_USB_ROOT
#define AUDIOBOOK_USB_ROOT "/mnt/usb0/ABooks"
#endif
#define MAX_BOOKS 256
#define MAX_PATH 512
#define MAX_FILE (8ULL * 1024ULL * 1024ULL * 1024ULL)
#ifndef PORT
#define PORT 9179
#endif
struct Item {char relative[MAX_PATH];off_t size;};
static struct Item items[MAX_BOOKS];
static unsigned item_count;
static const char *roots[] = {
 AUDIOBOOK_USB_ROOT, "/mnt/usb0/abooks", "/mnt/usb1/ABooks", "/mnt/usb1/abooks",
 "/usb0/ABooks", "/usb0/abooks", "/usb1/ABooks", "/usb1/abooks"
};
#ifndef AUDIOBOOK_LOCAL_ROOT
#define AUDIOBOOK_LOCAL_ROOT "/data/AudioBookReader/Books"
#endif
static const char *local_roots[] = {AUDIOBOOK_LOCAL_ROOT};
static char selected_root[MAX_PATH];
static char last_error[148];

static int write_all(int fd,const void *ptr,size_t bytes) {
 const char *p=(const char *)ptr;
 while(bytes){ssize_t n=write(fd,p,bytes);if(n<=0)return 0;p+=n;bytes-=(size_t)n;}
 return 1;
}
static int filename_ok(const char *s) {
 if(!s || !*s || s[0]=='.' || strlen(s)>180)return 0;
 for(const unsigned char *c=(const unsigned char*)s;*c;c++)
  if(*c<' ' || *c==127 || *c=='/' || *c=='\\')return 0;
 return 1;
}
static int is_book(const char *s){
 size_t len=strlen(s);
 return len>=4 && !strcasecmp(s+len-4,".m4b");
}

static void add_books(const char *dir,const char *rel,unsigned depth,unsigned *budget){
 if(depth>3 || item_count>=MAX_BOOKS || !*budget)return;
 DIR *d=opendir(dir);if(!d)return;
 struct dirent *e;
 while(*budget && item_count<MAX_BOOKS && (e=readdir(d))){
  --*budget;
  if(!filename_ok(e->d_name))continue;
  char sub[MAX_PATH], relative[MAX_PATH];
  int n=snprintf(sub,sizeof(sub),"%s/%s",dir,e->d_name);
  int k=*rel?snprintf(relative,sizeof(relative),"%s/%s",rel,e->d_name):
             snprintf(relative,sizeof(relative),"%s",e->d_name);
  if(n<0||n>=(int)sizeof(sub)||k<0||k>=(int)sizeof(relative))continue;
  struct stat st;
  if(lstat(sub,&st)!=0)continue; // never follow a symlink outside ABooks
  if(S_ISDIR(st.st_mode)&&depth<3){add_books(sub,relative,depth+1,budget);continue;}
  if(!S_ISREG(st.st_mode)||!is_book(e->d_name)||st.st_size<1||(uint64_t)st.st_size>MAX_FILE)continue;
  memcpy(items[item_count].relative,relative,(size_t)k+1);
  items[item_count].size=st.st_size;
  ++item_count;
 }
 closedir(d);
}
static int scan(int local){
 item_count=0;selected_root[0]='\0';last_error[0]='\0';
 const char **choices=local?local_roots:roots;
 const unsigned choiceCount=local?
   (unsigned)(sizeof(local_roots)/sizeof(local_roots[0])):
   (unsigned)(sizeof(roots)/sizeof(roots[0]));
 for(unsigned r=0;r<choiceCount;r++){
  DIR *d=opendir(choices[r]);
  if(!d)continue;
  closedir(d);
  snprintf(selected_root,sizeof(selected_root),"%s",choices[r]);
  unsigned budget=8192;
  add_books(selected_root,"",0,&budget);
  return 1;
 }
 if(local){
  int e=errno;
  snprintf(last_error,sizeof(last_error),
   "LOCAL_BOOKS_OPEN_FAILED path=%s errno=%d",AUDIOBOOK_LOCAL_ROOT,e);
  return 0;
 }
 // Report whether the USB mount exists separately from a missing ABooks folder.
 // Avoid enumerating or reading other folders; only inspect mount availability.
 struct stat mount;
 if(stat("/mnt/usb0",&mount)==0 && S_ISDIR(mount.st_mode))
  snprintf(last_error,sizeof(last_error),
    "ABooks missing under /mnt/usb0 (create /mnt/usb0/ABooks); also tried usb1");
 else if(stat("/mnt/usb1",&mount)==0 && S_ISDIR(mount.st_mode))
  snprintf(last_error,sizeof(last_error),
    "ABooks missing on USB; checked /mnt/usb0 and /mnt/usb1");
 else snprintf(last_error,sizeof(last_error),
    "USB mounts not accessible to helper (/mnt/usb0, /mnt/usb1)");
 return 0;
}
static int emit_catalog(int fd,int local){
 if(!scan(local)){
  char output[256];
  int n=snprintf(output,sizeof(output),"ABOOK1\nERROR %s\nDONE\n",last_error);
  return n>0 && n<(int)sizeof(output) && write_all(fd,output,(size_t)n);
 }
 if(!write_all(fd,"ABOOK1\n",7))return 0;
 char root_line[560];
 int rootlen=snprintf(root_line,sizeof(root_line),"ROOT %s\n",selected_root);
 if(rootlen<0||rootlen>=(int)sizeof(root_line)||!write_all(fd,root_line,(size_t)rootlen))return 0;
 for(unsigned i=0;i<item_count;i++){
  const struct Item *b=&items[i];char line[96];
  int n=snprintf(line,sizeof(line),"ITEM %zu %lld\n",strlen(b->relative),(long long)b->size);
  if(n<0||n>=(int)sizeof(line)||!write_all(fd,line,(size_t)n)||
     !write_all(fd,b->relative,strlen(b->relative)))return 0;
 }
 return write_all(fd,"DONE\n",5);
}
#ifndef AUDIOBOOK_USB_TEST
// Fetch only a small image adjacent to a listed M4B or in the same folder.
// Never expose an arbitrary file path to clients.
static void send_cover(int client,const char *bookname,int local){
 scan(local);
 const struct Item *item=NULL;
 for(unsigned i=0;i<item_count;i++)
  if(!strcmp(bookname,items[i].relative)){item=&items[i];break;}
 if(!item){write_all(client,"ERROR Unknown book\n",19);return;}
 char parent[MAX_PATH],stem[196],candidate[MAX_PATH];
 snprintf(parent,sizeof(parent),"%s",bookname);
 char *slash=strrchr(parent,'/');
 const char *filename=bookname;
 if(slash){*slash='\0';filename=slash+1;}
 else parent[0]='\0';
 snprintf(stem,sizeof(stem),"%s",filename);
 char *dot=strrchr(stem,'.');if(dot)*dot='\0';
 const char *patterns[]={"cover.jpg","folder.jpg","front.jpg","cover.jpeg",
                         "cover.png","folder.png","front.png"};
 for(unsigned i=0;i<sizeof(patterns)/sizeof(patterns[0])+3;i++){
  char generated[210];const char *name=NULL;
  if(i<sizeof(patterns)/sizeof(patterns[0]))name=patterns[i];
  else{
   const char *extension=i==7?".jpg":i==8?".jpeg":".png";
   if(snprintf(generated,sizeof(generated),"%s%s",stem,extension)>=(int)sizeof(generated))continue;
   name=generated;
  }
  if(!filename_ok(name))continue;
  int n=snprintf(candidate,sizeof(candidate),"%s/%s%s%s%s",selected_root,
      *parent?parent:"",*parent?"/":"",name,"");
  if(n<0||n>=(int)sizeof(candidate))continue;
  // Verify every intermediate directory, not just final file.
  char walk[MAX_PATH];snprintf(walk,sizeof(walk),"%s",selected_root);
  char copy[MAX_PATH];snprintf(copy,sizeof(copy),"%s",parent);
  char *save=NULL,*seg=strtok_r(copy,"/",&save);
  int secure=1;
  while(seg){
   if(!filename_ok(seg)){secure=0;break;}
   size_t len=strlen(walk);
   if(len+strlen(seg)+2>=sizeof(walk)){secure=0;break;}
   walk[len]='/';strcpy(walk+len+1,seg);
   struct stat d;
   if(lstat(walk,&d)!=0 || !S_ISDIR(d.st_mode)){secure=0;break;}
   seg=strtok_r(NULL,"/",&save);
  }
  if(!secure)continue;
  struct stat st;
  if(lstat(candidate,&st)!=0 || !S_ISREG(st.st_mode)||
     st.st_size<8||st.st_size>6*1024*1024)continue;
  FILE* image=fopen(candidate,"rb");if(!image)continue;
  unsigned char signature[8]={0};
  if(fread(signature,1,8,image)!=8){fclose(image);continue;}
  if(!((signature[0]==0xff&&signature[1]==0xd8) ||
    (signature[0]==0x89&&memcmp(signature+1,"PNG",3)==0))){
   fclose(image);continue;
  }
  rewind(image);
  char header[80];n=snprintf(header,sizeof(header),"DATA %lld\n",(long long)st.st_size);
  if(n>0&&write_all(client,header,(size_t)n)){
   char bytes[16384];size_t size;
   while((size=fread(bytes,1,sizeof(bytes),image))!=0)
    if(!write_all(client,bytes,size))break;
  }
  fclose(image);return;
 }
 write_all(client,"ERROR No companion cover found\n",31);
}
static void fetch(int client,const char *name,uint64_t offset,int local){
 scan(local);
 const struct Item *item=NULL;
 for(unsigned i=0;i<item_count;i++)if(strcmp(name,items[i].relative)==0){item=&items[i];break;}
 if(!item){write_all(client,"ERROR Unknown book\n",19);return;}
 char path[2*MAX_PATH];int n=snprintf(path,sizeof(path),"%s/%s",selected_root,name);
 if(n<0||n>=(int)sizeof(path))return;
 // Recheck directory components so paths that became symlinks are rejected.
 char walk[2*MAX_PATH];
 snprintf(walk,sizeof(walk),"%s",selected_root);
 char copy[MAX_PATH];snprintf(copy,sizeof(copy),"%s",name);
 char *save=NULL,*seg=strtok_r(copy,"/",&save);
 while(seg){
  if(!filename_ok(seg))return;
  size_t len=strlen(walk);
  if(len+1+strlen(seg)>=sizeof(walk))return;
  walk[len]='/';strcpy(walk+len+1,seg);
  struct stat st;if(lstat(walk,&st)!=0||S_ISLNK(st.st_mode))return;
  seg=strtok_r(NULL,"/",&save);
  if(seg && !S_ISDIR(st.st_mode))return;
 }
 struct stat st;if(lstat(path,&st)!=0||!S_ISREG(st.st_mode)||
                st.st_size!=item->size||st.st_size<1||(uint64_t)st.st_size>MAX_FILE)return;
 if(offset>(uint64_t)st.st_size)return;
 FILE *f=fopen(path,"rb");if(!f)return;
 if(fseeko(f,(off_t)offset,SEEK_SET)!=0){fclose(f);return;}
 char line[80];n=snprintf(line,sizeof(line),"DATA %lld\n",(long long)((uint64_t)st.st_size-offset));
 if(n>0&&write_all(client,line,(size_t)n)){
  char buf[16384];size_t got;size_t total=0;
  while((got=fread(buf,1,sizeof(buf),f))!=0){
   if(!write_all(client,buf,got))break;
   total+=got;
   if(total>MAX_FILE)break;
#ifdef AUDIOBOOK_USB_TEST_DROP_ONCE
   // Simulate a dropped connection in host integration tests. Retries
   // using a positive offset must still deliver the rest of the file.
   if(offset==0 && total>=4U*1024U*1024U)break;
#endif
  }
 }
 fclose(f);
}
static void serve(void){
 signal(SIGPIPE,SIG_IGN);
 int s=socket(AF_INET,SOCK_STREAM,0);if(s<0)return;
 int reuse=1;setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
 struct sockaddr_in addr={0};addr.sin_family=AF_INET;addr.sin_port=htons(PORT);
 addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
 if(bind(s,(struct sockaddr *)&addr,sizeof(addr))!=0||listen(s,3)!=0){close(s);return;}
 for(int i=0;i<120;i++){
  fd_set set;FD_ZERO(&set);FD_SET(s,&set);
  struct timeval idle={600,0};
  if(select(s+1,&set,NULL,NULL,&idle)<=0)break;
  int c=accept(s,NULL,NULL);if(c<0)continue;
  struct timeval timeout={90,0};
  setsockopt(c,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
  setsockopt(c,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
  char request[600];int n=recv(c,request,sizeof(request)-1,0);
  if(n>0){request[n]='\0';
   char *nl=strchr(request,'\n');if(nl)*nl='\0';
   if(!strcmp(request,"LIST"))emit_catalog(c,0);
   else if(!strcmp(request,"LISTLOCAL"))emit_catalog(c,1);
   else if(!strncmp(request,"FETCH ",6)&&strlen(request+6)<MAX_PATH)fetch(c,request+6,0,0);
   else if(!strncmp(request,"COVER ",6)&&strlen(request+6)<MAX_PATH)
     send_cover(c,request+6,0);
   else if(!strncmp(request,"COVERLOCAL ",11)&&strlen(request+11)<MAX_PATH)
     send_cover(c,request+11,1);
   else if(!strncmp(request,"FETCHLOCAL ",11) && strlen(request+11)<MAX_PATH)
     fetch(c,request+11,0,1);
   else if(!strncmp(request,"FETCHLOCALR ",12)){
     char *end=NULL;errno=0;
     const unsigned long long offset=strtoull(request+12,&end,10);
     if(!errno&&end&&*end==' '&&strlen(end+1)<MAX_PATH)
       fetch(c,end+1,(uint64_t)offset,1);
   }
   else if(!strncmp(request,"FETCHR ",7)){
     char *end=NULL;
     errno=0;
     const unsigned long long offset=strtoull(request+7,&end,10);
     if(!errno && end && *end==' ' && strlen(end+1)<MAX_PATH)
       fetch(c,end+1,(uint64_t)offset,0);
   }
  }
  close(c);
 }
 close(s);
}
#endif
int main(void){
 setvbuf(stdout,NULL,_IONBF,0);
 emit_catalog(STDOUT_FILENO,0);
#ifndef AUDIOBOOK_USB_TEST
 serve();
#endif
 return 0;
}
