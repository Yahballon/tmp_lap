// SPDX-License-Identifier: GPL-3.0-or-later
// Uninstaller by YAHBALLON - one-shot *opt-in* native uninstall request.
// This helper is NEVER launched by a scan. Main app must show two-step
// confirmation and write the private request first.
// Registered applications: Sony AppInstUtil only, never unlink app directories.
// USB PKG archives: only lstat-verified regular .pkg files on /mnt/usbN.
// No save-data removal, no shell commands, no arbitrary paths.
#include <ps5/kernel.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BASE "/data/Uninstaller/actions"
#define REQUEST BASE "/request"
#define AUTHID_INSTALLER 0x4800000000000006ull
// Resolve the PS5 installer library dynamically: direct imports do not link
// with the homebrew payload SDK, and can prevent loading on some firmwares.
int sceKernelLoadStartModule(const char *, size_t, const void *, unsigned, void *, int *);
typedef int (*init_fn)(void);
typedef int (*term_fn)(void);
typedef int (*uninstall_fn)(const char *, void *, void *);
static init_fn app_init=NULL;
static term_fn app_term=NULL;
static uninstall_fn app_uninstall=NULL;
static int load_installer(void){
    int h=sceKernelLoadStartModule("/system/common/lib/libSceAppInstUtil.sprx",
                                   0,NULL,0,NULL,NULL);
    if(h<0)return h;
    app_init=(init_fn)kernel_dynlib_dlsym(getpid(),(uint32_t)h,
                                          "sceAppInstUtilInitialize");
    app_term=(term_fn)kernel_dynlib_dlsym(getpid(),(uint32_t)h,
                                          "sceAppInstUtilTerminate");
    app_uninstall=(uninstall_fn)kernel_dynlib_dlsym(getpid(),(uint32_t)h,
                                          "sceAppInstUtilAppUnInstall");
    return app_init&&app_uninstall?0:ENOSYS;
}

static int private_file(const char* path,uid_t owner,int wantDir){
    struct stat st;
    if(lstat(path,&st)!=0||st.st_uid!=owner||(st.st_mode&0077)!=0)return 0;
    if(wantDir)return S_ISDIR(st.st_mode);
    return S_ISREG(st.st_mode)&&st.st_size>0&&st.st_size<900;
}
static int read_value(const char* txt,const char* key,char* dst,size_t limit){
    size_t klen=strlen(key);
    for(const char* line=txt;*line;){
        const char* end=strchr(line,'\n');
        size_t n=end?(size_t)(end-line):strlen(line);
        if(n>klen+1&&strncmp(line,key,klen)==0&&line[klen]=='='){
            size_t len=n-klen-1;
            if(len>=limit)return 0;
            memcpy(dst,line+klen+1,len);dst[len]=0;return 1;
        }
        if(!end)break;line=end+1;
    }
    return 0;
}
static int nonce_ok(const char* nonce){
    if(strlen(nonce)!=32)return 0;
    for(int i=0;i<32;i++)if(!((nonce[i]>='a'&&nonce[i]<='f')||
                                (nonce[i]>='0'&&nonce[i]<='9')))return 0;
    return 1;
}
static int title_ok(const char* id){
    if(strlen(id)!=9||!strcmp(id,"PPSA99986"))return 0;
    if(strncmp(id,"PPSA",4)&&strncmp(id,"CUSA",4)&&
       strncmp(id,"LAPY",4)&&strncmp(id,"FAKE",4))return 0;
    for(int i=4;i<9;i++)if(id[i]<'0'||id[i]>'9')return 0;
    return 1;
}
static int installed(const char* title){
    char dir[96],meta[120],sfo[120],exec[120];
    snprintf(dir,sizeof(dir),"/user/app/%s",title);
    struct stat st;
    if(lstat(dir,&st)!=0||!S_ISDIR(st.st_mode))return 0;
    snprintf(meta,sizeof(meta),"%s/sce_sys/param.json",dir);
    snprintf(sfo,sizeof(sfo),"%s/sce_sys/param.sfo",dir);
    snprintf(exec,sizeof(exec),"%s/eboot.bin",dir);
    return lstat(meta,&st)==0||lstat(sfo,&st)==0||lstat(exec,&st)==0;
}
static int pkg_path_ok(const char* path){
    if(strlen(path)<17||strlen(path)>700)return 0;
    if(strstr(path,"/../")||strstr(path,"//")||strchr(path,'\\')||strchr(path,'\n'))return 0;
    int usb=0;
    if(strncmp(path,"/mnt/usb",8)!=0)return 0;
    if(path[8]<'0'||path[8]>'7'||path[9]!='/')return 0;
    const char* name=strrchr(path,'/');
    if(!name||strlen(++name)<5||name[0]=='.')return 0;
    const char* ext=strrchr(name,'.');
    if(!ext||(strcmp(ext,".pkg")&&strcmp(ext,".PKG")))return 0;
    struct stat st;
    if(lstat(path,&st)!=0||!S_ISREG(st.st_mode))return 0;
    (void)usb;return 1;
}
static void result(uid_t owner,const char* nonce,const char* kind,int code,int state){
    // Deliberately no 'success' label here. The foreground app must rescan.
    (void)kernel_set_ucred_uid(-1,owner);
    char file[200];
    const int len=snprintf(file,sizeof(file),
          "nonce=%s\nkind=%s\ncode=%08x\nstate=%d\n",nonce,kind,(unsigned)code,state);
    int fd=open(BASE "/result.tmp",O_CREAT|O_TRUNC|O_WRONLY|O_NOFOLLOW,0600);
    if(fd<0)return;
    int written=(write(fd,file,(size_t)len)==len);
    fsync(fd);close(fd);
    if(written)rename(BASE "/result.tmp",BASE "/result");
    else unlink(BASE "/result.tmp");
}
int main(void){
    setvbuf(stdout,NULL,_IONBF,0);
    struct stat dir;
    if(lstat(BASE,&dir)!=0||!S_ISDIR(dir.st_mode)||
       !private_file(BASE,dir.st_uid,1)||!private_file(REQUEST,dir.st_uid,0)){
       puts("UREM fail request_permissions");return 2;
    }
    int fd=open(REQUEST,O_RDONLY|O_NOFOLLOW);
    if(fd<0){puts("UREM fail request_read");return 2;}
    char request[901]={0};
    ssize_t n=read(fd,request,sizeof(request)-1);close(fd);
    if(n<=0 || n>=900){puts("UREM fail request_size");return 2;}
    char nonce[40]={0},kind[12]={0},title[24]={0},path[720]={0};
    if(!read_value(request,"nonce",nonce,sizeof(nonce))||!nonce_ok(nonce)||
       !read_value(request,"kind",kind,sizeof(kind))){
        puts("UREM fail request_format");return 2;
    }
    int rc=EINVAL,state=0;
    if(!strcmp(kind,"TITLE")){
        if(!read_value(request,"title",title,sizeof(title)) || !title_ok(title) ||
           !installed(title)){
            puts("UREM fail invalid_title");result(dir.st_uid,nonce,kind,EINVAL,0);return 3;
        }
        // Init must precede installer authid, following proven Playden behavior.
        rc=load_installer();
        if(rc==0)rc=app_init();
        if(rc==0){
            const uint64_t original=kernel_get_ucred_authid(-1);
            kernel_set_ucred_authid(-1,AUTHID_INSTALLER);
            if(kernel_get_ucred_authid(-1)==AUTHID_INSTALLER)
                rc=app_uninstall(title,NULL,NULL);
            else rc=EPERM;
            kernel_set_ucred_authid(-1,original);
            if(app_term)app_term();
            state=rc==0?1:0; // request accepted; asynchronous system work may remain
        }
    }else if(!strcmp(kind,"USBPKG")){
        if(!read_value(request,"path",path,sizeof(path))||!pkg_path_ok(path)){
            puts("UREM fail invalid_pkg");result(dir.st_uid,nonce,kind,EINVAL,0);return 3;
        }
        rc=unlink(path)==0?0:errno;
        state=rc==0?2:0; // physical file was removed, independent of installed games
    }else{
        puts("UREM fail invalid_kind");result(dir.st_uid,nonce,kind,EINVAL,0);return 3;
    }
    result(dir.st_uid,nonce,kind,rc,state);
    printf("UREM completed code=0x%08x state=%d\n",(unsigned)rc,state);
    return rc==0?0:5;
}
