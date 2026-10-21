// SPDX-License-Identifier: GPL-3.0-or-later
// Uninstaller v0.1.8: single-use local action broker for existing elfldr.
// NO shared writable request folder, NO arbitrary paths or system-dir unlink.
// The helper prints a random capability only to its elfldr stdout connection,
// then accepts one confirmed operation over 127.0.0.1:9176 for <=15 seconds.
#include <ps5/kernel.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define AUTHID_INSTALLER 0x4800000000000006ull
#define PORT 9176
int sceKernelLoadStartModule(const char*,size_t,const void*,unsigned,void*,int*);
typedef int (*init_fn)(void);
typedef int (*term_fn)(void);
typedef int (*uninstall_fn)(const char*,void*,void*);
static init_fn init_app=NULL;
static term_fn term_app=NULL;
static uninstall_fn uninstall_app=NULL;
static int load_installer(void){
  int h=sceKernelLoadStartModule("/system/common/lib/libSceAppInstUtil.sprx",
                                 0,NULL,0,NULL,NULL);
  if(h<0)return h;
  init_app=(init_fn)kernel_dynlib_dlsym(getpid(),(uint32_t)h,"sceAppInstUtilInitialize");
  term_app=(term_fn)kernel_dynlib_dlsym(getpid(),(uint32_t)h,"sceAppInstUtilTerminate");
  uninstall_app=(uninstall_fn)kernel_dynlib_dlsym(getpid(),(uint32_t)h,"sceAppInstUtilAppUnInstall");
  return init_app&&uninstall_app?0:ENOSYS;
}
static int strong_random(char out[33]){
  unsigned char bytes[16];
  int f=open("/dev/urandom",O_RDONLY);
  if(f<0)return 0;
  size_t done=0;
  while(done<sizeof bytes){
    ssize_t n=read(f,bytes+done,sizeof bytes-done);
    if(n<=0)break;
    done+=(size_t)n;
  }
  close(f);
  if(done!=sizeof bytes)return 0;
  for(unsigned i=0;i<16;i++)snprintf(out+2*i,3,"%02x",(unsigned)bytes[i]);
  out[32]=0;
  return 1;
}
static int title_ok(const char* id){
  if(strlen(id)!=9||!strcmp(id,"PPSA99986"))return 0;
  if(strncmp(id,"PPSA",4)&&strncmp(id,"CUSA",4)&&
     strncmp(id,"FAKE",4)&&strncmp(id,"LAPY",4))return 0;
  for(int k=4;k<9;k++)if(id[k]<'0'||id[k]>'9')return 0;
  return 1;
}
static int installed(const char* id){
  char p[190];struct stat st;
  snprintf(p,sizeof p,"/user/app/%s",id);
  if(lstat(p,&st)||!S_ISDIR(st.st_mode))return 0;
  const char* parts[]={"/eboot.bin","/sce_sys/param.json","/sce_sys/param.sfo"};
  for(unsigned i=0;i<3;i++){
    char sub[220];snprintf(sub,sizeof sub,"%s%s",p,parts[i]);
    if(lstat(sub,&st)==0&&S_ISREG(st.st_mode))return 1;
  }
  return 0;
}
static int safe_path(const char* path){
  size_t n=strlen(path);
  if(n<18||n>690||strncmp(path,"/mnt/usb",8)||
     path[8]<'0'||path[8]>'7'||path[9]!='/'||
     strstr(path,"//")||strstr(path,"/../")||strstr(path,"/./")||
     strchr(path,'\\')||path[n-1]=='/')return 0;
  for(size_t i=0;i<n;i++)
    if((unsigned char)path[i]<32||(unsigned char)path[i]==127)return 0;
  return 1;
}
static int pkg_ok(const char* path){
  if(!safe_path(path))return 0;
  size_t n=strlen(path);
  if(n<5||(strcmp(path+n-4,".pkg")&&strcmp(path+n-4,".PKG")))return 0;
  const char* name=strrchr(path,'/');
  if(!name||name[1]=='.')return 0;
  struct stat st;
  return lstat(path,&st)==0&&S_ISREG(st.st_mode)&&st.st_nlink==1;
}
static int homebrew_ok(const char* path){
  if(!safe_path(path))return 0;
  const char* first=strchr(path+10,'/');
  if(!first||strncmp(path+10,"homebrew/",9)!=0)return 0;
  const char* name=path+19;
  if(!*name||strchr(name,'/')||name[0]=='.'||!strcmp(name,".."))return 0;
  struct stat st;
  return lstat(path,&st)==0&&S_ISDIR(st.st_mode);
}
// Validate the *entire* tree before any deletion. Refuse symlinks and
// special nodes to avoid unexpected traversal. Bounded 15000 entries.
static int tree_walk(const char* path,int apply,unsigned depth,unsigned* count){
  if(depth>20||*count>=15000)return E2BIG;
  struct stat st;
  if(lstat(path,&st))return errno;
  if(S_ISLNK(st.st_mode))return EPERM;
  if(S_ISREG(st.st_mode)){
    (*count)++;
    if(st.st_nlink!=1)return EPERM;
    if(!apply)return 0;
    return unlink(path)==0?0:errno;
  }
  if(!S_ISDIR(st.st_mode))return EPERM;
  DIR* dir=opendir(path);
  if(!dir)return errno;
  int error=0;struct dirent* entry;
  while((entry=readdir(dir))!=NULL){
    if(!strcmp(entry->d_name,".")||!strcmp(entry->d_name,".."))continue;
    if(strchr(entry->d_name,'/')||strlen(entry->d_name)>250){error=EINVAL;break;}
    char child[1000];
    int n=snprintf(child,sizeof child,"%s/%s",path,entry->d_name);
    if(n<=0||(size_t)n>=sizeof child){error=ENAMETOOLONG;break;}
    error=tree_walk(child,apply,depth+1,count);
    if(error)break;
  }
  closedir(dir);
  if(error)return error;
  (*count)++;
  if(!apply)return 0;
  return rmdir(path)==0?0:errno;
}
static void perform(const char* kind,const char* target,unsigned* code,unsigned* state){
  *code=EINVAL;*state=0;
  if(!strcmp(kind,"TITLE")){
    if(!title_ok(target)||!installed(target))return;
    int rc=load_installer();
    if(rc==0)rc=init_app();
    if(rc==0){
      uint64_t saved=kernel_get_ucred_authid(-1);
      kernel_set_ucred_authid(-1,AUTHID_INSTALLER);
      rc=kernel_get_ucred_authid(-1)==AUTHID_INSTALLER
          ?uninstall_app(target,NULL,NULL):EPERM;
      kernel_set_ucred_authid(-1,saved);
      if(term_app)term_app();
    }
    *code=(unsigned)rc;
    if(rc==0)*state=1; // accepted, system removal may be asynchronous
  }else if(!strcmp(kind,"USBPKG")){
    if(!pkg_ok(target))return;
    *code=unlink(target)==0?0:(unsigned)errno;
    if(!*code)*state=2;
  }else if(!strcmp(kind,"USBHOME")){
    if(!homebrew_ok(target))return;
    unsigned inspected=0,removed=0;
    int rc=tree_walk(target,0,0,&inspected);
    if(rc==0)rc=tree_walk(target,1,0,&removed);
    *code=(unsigned)rc;
    if(rc==0)*state=2;
  }
}
static int send_full(int fd,const char* s){
  size_t n=strlen(s);
  while(n){ssize_t v=send(fd,s,n,0);if(v<=0)return 0;s+=v;n-=(size_t)v;}
  return 1;
}
int main(void){
  setvbuf(stdout,NULL,_IONBF,0);
  char token[33]={0};
  if(!strong_random(token)){puts("UREM2_ERROR no_random");return 2;}
  int server=socket(AF_INET,SOCK_STREAM,0);
  if(server<0){puts("UREM2_ERROR socket");return 2;}
  int reuse=1;
  setsockopt(server,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof reuse);
  struct sockaddr_in addr={0};
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  addr.sin_port=htons(PORT);
  if(bind(server,(struct sockaddr*)&addr,sizeof addr)!=0||listen(server,1)!=0){
    puts("UREM2_ERROR bind");close(server);return 3;
  }
  printf("UREM2_READY %s\n",token);
  fd_set readfds;FD_ZERO(&readfds);FD_SET(server,&readfds);
  struct timeval wait={15,0};
  if(select(server+1,&readfds,NULL,NULL,&wait)<=0){
    puts("UREM2_ERROR timeout");close(server);return 4;
  }
  int client=accept(server,NULL,NULL);
  close(server);
  if(client<0)return 5;
  struct timeval shortwait={5,0};
  setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,&shortwait,sizeof shortwait);
  setsockopt(client,SOL_SOCKET,SO_SNDTIMEO,&shortwait,sizeof shortwait);
  char command[800]={0};size_t used=0;char ch=0;
  while(used+1<sizeof command && recv(client,&ch,1,0)==1){
    if(ch=='\n')break;
    command[used++]=ch;
  }
  if(ch!='\n'||strncmp(command,"ACT ",4)!=0||
     strlen(command)<4+32+1+5+1||
     strncmp(command+4,token,32)!=0||command[36]!=' '){
    send_full(client,"UREM2_ERROR unauthorized\n");close(client);return 6;
  }
  char* kind=command+37;char* sep=strchr(kind,' ');
  if(!sep){send_full(client,"UREM2_ERROR format\n");close(client);return 7;}
  *sep=0;const char* target=sep+1;
  unsigned code=0,state=0;
  perform(kind,target,&code,&state);
  char report[160];
  snprintf(report,sizeof report,"UREM2_RESULT %s %08x %u\n",token,code,state);
  send_full(client,report);
  close(client);
  printf("UREM2_DONE code=%08x state=%u\n",code,state);
  return code==0?0:8;
}
