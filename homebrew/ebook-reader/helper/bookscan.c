// SPDX-License-Identifier: GPL-3.0-or-later
// Read-only USB EPUB/PDF bridge, launched by an already-running ELF loader.
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

#ifndef EBOOK_USB_ROOT
#define EBOOK_USB_ROOT "/mnt/usb0/Books"
#endif
#define MAX_BOOKS 256
#define MAX_PATH 512
#define MAX_FILE (2ULL * 1024ULL * 1024ULL * 1024ULL)
#ifndef PORT
#define PORT 9176
#endif
struct Item {char relative[MAX_PATH];off_t size;};
static struct Item items[MAX_BOOKS];
static unsigned item_count;
static const char *roots[] = {EBOOK_USB_ROOT, "/mnt/usb0/books", "/usb0/Books", "/usb0/books"};
static char selected_root[MAX_PATH];

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
 size_t len=strlen(s);const char *ext=len>5?s+len-5:"";
 if(!strcasecmp(ext,".epub"))return 1;
 ext=len>4?s+len-4:"";return !strcasecmp(ext,".pdf") || !strcasecmp(ext,".cbr") || !strcasecmp(ext,".cbz");
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
  if(lstat(sub,&st)!=0)continue; // never follow a symlink outside Books
  if(S_ISDIR(st.st_mode)&&depth<3){add_books(sub,relative,depth+1,budget);continue;}
  if(!S_ISREG(st.st_mode)||!is_book(e->d_name)||st.st_size<1||(uint64_t)st.st_size>MAX_FILE)continue;
  memcpy(items[item_count].relative,relative,(size_t)k+1);
  items[item_count].size=st.st_size;
  ++item_count;
 }
 closedir(d);
}
static int scan(void){
 item_count=0;selected_root[0]='\0';
 for(unsigned r=0;r<sizeof(roots)/sizeof(roots[0]);r++){
  DIR *d=opendir(roots[r]);
  if(!d)continue;
  closedir(d);
  snprintf(selected_root,sizeof(selected_root),"%s",roots[r]);
  unsigned budget=8192;
  add_books(selected_root,"",0,&budget);
  return 1;
 }
 return 0;
}
static int emit_catalog(int fd){
 if(!scan())return write_all(fd,"EBOOK1\nERROR USB Books not accessible\n",38);
 if(!write_all(fd,"EBOOK1\n",7))return 0;
 for(unsigned i=0;i<item_count;i++){
  const struct Item *b=&items[i];char line[96];
  int n=snprintf(line,sizeof(line),"ITEM %zu %lld\n",strlen(b->relative),(long long)b->size);
  if(n<0||n>=(int)sizeof(line)||!write_all(fd,line,(size_t)n)||
     !write_all(fd,b->relative,strlen(b->relative)))return 0;
 }
 return write_all(fd,"DONE\n",5);
}
#ifndef EBOOK_USB_TEST
static void fetch(int client,const char *name,uint64_t offset){
 if(!*selected_root)scan();
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
#ifdef EBOOK_USB_TEST_DROP_ONCE
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
   if(!strcmp(request,"LIST"))emit_catalog(c);
   else if(!strncmp(request,"FETCH ",6)&&strlen(request+6)<MAX_PATH)fetch(c,request+6,0);
   else if(!strncmp(request,"FETCHR ",7)){
     char *end=NULL;
     errno=0;
     const unsigned long long offset=strtoull(request+7,&end,10);
     if(!errno && end && *end==' ' && strlen(end+1)<MAX_PATH)
       fetch(c,end+1,(uint64_t)offset);
   }
  }
  close(c);
 }
 close(s);
}
#endif
int main(void){
 setvbuf(stdout,NULL,_IONBF,0);
 emit_catalog(STDOUT_FILENO);
#ifndef EBOOK_USB_TEST
 serve();
#endif
 return 0;
}
