// SPDX-License-Identifier: GPL-3.0-or-later
// Uninstaller v0.1.14: single-use local action broker for existing elfldr.
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
// Use direct AppInstUtil SDK imports, consistent with tested PS5 title-remover.
// The Sony uninstall API takes a single title ID argument.
int sceAppInstUtilInitialize(void);
int sceAppInstUtilAppUnInstall(const char *title_id);

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
static int progress_fd=-1;
static void progress_ping(unsigned count,int applying){
  if(progress_fd>=0 && count>0 && count%512==0){
    char text[98];
    snprintf(text,sizeof text,"UREM2_STAGE %s_%u\n",
             applying?"removing_files":"checking_files",count);
    send(progress_fd,text,strlen(text),0);
  }
}
// Validate the *entire* tree before any deletion. Refuse symlinks and
// special nodes to avoid unexpected traversal. Bounded 15000 entries.
static int tree_walk(const char* path,int apply,unsigned depth,unsigned* count){
  if(depth>64||*count>=500000)return E2BIG;
  struct stat st;
  if(lstat(path,&st))return errno;
  if(S_ISLNK(st.st_mode))return EPERM;
  if(S_ISREG(st.st_mode)){
    (*count)++;
    progress_ping(*count,apply);
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
  progress_ping(*count,apply);
  if(!apply)return 0;
  return rmdir(path)==0?0:errno;
}
static void report_stage(int client,const char* phase){
  char msg[96];
  snprintf(msg,sizeof msg,"UREM2_STAGE %s\n",phase);
  send(client,msg,strlen(msg),0);
  printf("UREM2_STAGE %s\n",phase);
}

// ShadowMount's raw USB source folders are not registered PKG installs.
// Only top-level /mnt/usbN/<title_id>-app folders may be considered, and
// only when their contents look like a game. Never delete app.db or mount
// directories under /user/app or /system_ex/app.
static int source_syntax(const char* path,const char* title){
  char expected[100];
  if(!title_ok(title)||!safe_path(path))return 0;
  if(path[9]!='/'||strchr(path+10,'/'))return 0;
  if(snprintf(expected,sizeof expected,"%s-app",title)>= (int)sizeof expected)
    return 0;
  if(!strcmp(path+10,expected))return 1;
  if(snprintf(expected,sizeof expected,"%s-app0",title)>= (int)sizeof expected)
    return 0;
  return !strcmp(path+10,expected);
}
static int marker_exists(const char* base,const char* tail){
  char file[960];struct stat st;
  if(snprintf(file,sizeof file,"%s/%s",base,tail)>=(int)sizeof file)
    return 0;
  return lstat(file,&st)==0 && S_ISREG(st.st_mode) && st.st_nlink==1;
}
static int title_tracker_matches(const char* path,const char* id){
  char tracker[128];snprintf(tracker,sizeof tracker,"/user/app/%s/mount.lnk",id);
  int fd=open(tracker,O_RDONLY|O_NOFOLLOW);
  if(fd<0)return errno==ENOENT?1:0; // source not registered / no tracker
  char line[960]={0};ssize_t n=read(fd,line,sizeof line-1);
  close(fd);
  if(n<=0)return 0;
  line[n]=0;
  size_t len=strcspn(line,"\r\n");
  line[len]=0;
  size_t p=strlen(path);
  // ShadowMount may store an app contents subdirectory instead of base.
  return !strcmp(line,path) || (strncmp(line,path,p)==0 && line[p]=='/');
}
static int source_ok(const char* path,const char* title){
  if(!source_syntax(path,title))return 0;
  struct stat st;
  if(lstat(path,&st)||!S_ISDIR(st.st_mode))return 0;
  if(!title_tracker_matches(path,title))return 0;
  if(marker_exists(path,"eboot.bin")||
     marker_exists(path,"sce_sys/param.json")||
     marker_exists(path,"sce_sys/param.sfo")||
     marker_exists(path,"contents/eboot.bin")||
     marker_exists(path,"contents/sce_sys/param.json"))return 1;
  return 0;
}
static void diagnostic(int client,const char* type,const char* detail){
  // Diagnostics are read-only and must not modify or traverse source roots.
  char clean[660]={0};size_t n=0;
  for(const char* p=detail;p&&*p&&n+1<sizeof clean;p++){
    unsigned char ch=(unsigned char)*p;
    clean[n++]=(ch<32||ch>126)?'_':(char)ch;
  }
  char message[800];
  snprintf(message,sizeof message,"UREM2_DIAG %s %s\n",type,clean);
  send(client,message,strlen(message),0);
}
static void diagnose_tracker(const char* title,int client){
  char tracker[130];
  snprintf(tracker,sizeof tracker,"/user/app/%s/mount.lnk",title);
  int fd=open(tracker,O_RDONLY|O_NOFOLLOW);
  if(fd>=0){
    char line[700]={0};
    ssize_t n=read(fd,line,sizeof line-1);close(fd);
    if(n>0){
      line[n]=0;line[strcspn(line,"\r\n")]=0;
      diagnostic(client,"tracker",line);
    }else diagnostic(client,"tracker","unreadable");
  }else if(errno!=ENOENT)diagnostic(client,"tracker","inaccessible");
  else diagnostic(client,"tracker","missing");
  snprintf(tracker,sizeof tracker,"/user/app/%s/mount_img.lnk",title);
  fd=open(tracker,O_RDONLY|O_NOFOLLOW);
  if(fd>=0){
    char line[700]={0};
    ssize_t n=read(fd,line,sizeof line-1);close(fd);
    if(n>0){
      line[n]=0;line[strcspn(line,"\r\n")]=0;
      diagnostic(client,"image_tracker",line);
    }
  }
}
static int detect_source(const char* title,char out[700],int client){
  out[0]=0;
  if(!title_ok(title))return EINVAL;
  diagnose_tracker(title,client);
  unsigned found=0;
  for(unsigned i=0;i<8;i++)for(unsigned form=0;form<2;form++){
    char candidate[700];
    snprintf(candidate,sizeof candidate,form?
             "/mnt/usb%u/%s-app0":"/mnt/usb%u/%s-app",i,title);
    struct stat st;
    if(lstat(candidate,&st)!=0)continue;
    if(source_ok(candidate,title)){
      diagnostic(client,"matched_source",candidate);
      if(++found>1){out[0]=0;return EEXIST;}
      strcpy(out,candidate);
    }else{
      // Explain why a concrete matching folder was refused, rather than
      // telling the user "none" when game bytes remain on the USB drive.
      diagnostic(client,!S_ISDIR(st.st_mode)?"rejected_not_directory":
                 !title_tracker_matches(candidate,title)?"rejected_tracker_mismatch":
                 "rejected_missing_game_files",candidate);
    }
  }
  if(!found)diagnostic(client,"result","no_verified_usb_game_directory");
  return 0;
}
static int do_system_uninstall(const char* title,int client){
  if(!installed(title))return ENOENT;
  report_stage(client,"appinst_initialize");
  int rc=sceAppInstUtilInitialize();
  if(rc!=0){report_stage(client,"appinst_init_rejected");return rc;}
  report_stage(client,"installer_authid");
  uint64_t saved=kernel_get_ucred_authid(-1);
  kernel_set_ucred_authid(-1,AUTHID_INSTALLER);
  if(kernel_get_ucred_authid(-1)!=AUTHID_INSTALLER){
    report_stage(client,"installer_authid_rejected");
    kernel_set_ucred_authid(-1,saved);
    return EPERM;
  }
  report_stage(client,"appinst_uninstall");
  rc=sceAppInstUtilAppUnInstall(title);
  report_stage(client,"appinst_uninstall_returned");
  kernel_set_ucred_authid(-1,saved);
  return rc;
}
static void send_source(int client,const char* source){
  char payload[850];
  snprintf(payload,sizeof payload,"UREM2_SOURCE %s\n",source&&source[0]?source:"-");
  send(client,payload,strlen(payload),0);
}
static int remove_game_source(const char* src,const char* id,int client){
  if(!source_ok(src,id))return EPERM;
  char staged[850];
  // Remove from ShadowMount's scan roots atomically before recursive
  // cleanup so it cannot immediately re-add a half-removed game.
  char nonce[33];
  if(!strong_random(nonce))return EIO;
  char prefix[32];
  memcpy(prefix,nonce,12);prefix[12]=0;
  if(snprintf(staged,sizeof staged,"%.*s/.uninstaller-%s-%s",
       9,src,id,prefix)>= (int)sizeof staged)return ENAMETOOLONG;
  // /mnt/usb0 is length 9; /mnt/usb1 etc are the only allowed roots.
  struct stat exists;
  if(lstat(staged,&exists)==0)return EEXIST;
  report_stage(client,"source_quarantine");
  if(rename(src,staged)!=0)return errno;
  report_stage(client,"source_delete");
  unsigned inspected=0,removed=0;
  // Pre-flight whole renamed folder; aborts on symlinks and special files.
  progress_fd=client;
  int err=tree_walk(staged,0,0,&inspected);
  if(err==0)err=tree_walk(staged,1,0,&removed);
  progress_fd=-1;
  if(err!=0){
    char error[900];
    snprintf(error,sizeof error,"UREM2_STAGED %s\n",staged);
    send(client,error,strlen(error),0);
    return err; // remains hidden for manual recovery, never claim success
  }
  return 0;
}

static void perform(const char* kind,const char* target,unsigned* code,unsigned* state,int client){
  *code=EINVAL;*state=0;
  if(!strcmp(kind,"TITLEPROBE")||!strcmp(kind,"TITLEAUTO")){
    report_stage(client,"source_inspect");
    char id[16]={0}, expected[700]={0}, source[700]={0};
    const char* split=strchr(target,'|');
    if(split){
      size_t n=(size_t)(split-target);
      if(n>=sizeof id){*code=EINVAL;return;}
      memcpy(id,target,n);id[n]=0;
      snprintf(expected,sizeof expected,"%s",split+1);
    }else snprintf(id,sizeof id,"%s",target);
    if(!title_ok(id)){*code=EINVAL;return;}
    int detected=detect_source(id,source,client);
    if(detected){*code=(unsigned)detected;return;}
    if(!strcmp(kind,"TITLEPROBE")){
      send_source(client,source);
      *code=0;*state=3;return;
    }
    // A source path selected in the UI must remain identical on disk.
    if(strcmp(expected,source)) { *code=ESTALE;return; }
    if(source[0])send_source(client,source);
    // Try normal PS5 registration removal; ShadowMount-only sources can
    // outlive this API, so a failed/missing registered entry is not fatal
    // to removing a separately confirmed backing folder.
    int native=ENOENT;
    if(installed(id))native=do_system_uninstall(id,client);
    if(source[0]){
      int sourceError=remove_game_source(source,id,client);
      *code=(unsigned)sourceError;
      *state=sourceError==0?(native==0?2:4):0;
      if(sourceError==0 && native!=0)
        report_stage(client,"source_deleted_title_cleanup_pending");
      return;
    }
    *code=(unsigned)native;
    *state=native==0?1:0;
    return;
  }
  if(!strcmp(kind,"TITLE")){
    report_stage(client,"title_validate");
    if(!title_ok(target)||!installed(target)){
      report_stage(client,"invalid_installed_title");
      return;
    }
    report_stage(client,"appinst_initialize");
    int rc=sceAppInstUtilInitialize();
    if(rc!=0){
      report_stage(client,"appinst_init_rejected");
      *code=(unsigned)rc;
      return;
    }
    // Initialize AppInstUtil before switching to the installer authid.
    report_stage(client,"installer_authid");
    uint64_t saved=kernel_get_ucred_authid(-1);
    kernel_set_ucred_authid(-1,AUTHID_INSTALLER);
    if(kernel_get_ucred_authid(-1)!=AUTHID_INSTALLER){
      report_stage(client,"installer_authid_rejected");
      *code=EPERM;
      kernel_set_ucred_authid(-1,saved);
      return;
    }
    report_stage(client,"appinst_uninstall");
    rc=sceAppInstUtilAppUnInstall(target);
    report_stage(client,"appinst_uninstall_returned");
    kernel_set_ucred_authid(-1,saved);
    *code=(unsigned)rc;
    if(rc==0)*state=1; // accepted; asynchronous native operation may remain
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
  perform(kind,target,&code,&state,client);
  char report[160];
  snprintf(report,sizeof report,"UREM2_RESULT %s %08x %u\n",token,code,state);
  send_full(client,report);
  close(client);
  printf("UREM2_DONE code=%08x state=%u\n",code,state);
  return code==0?0:8;
}
