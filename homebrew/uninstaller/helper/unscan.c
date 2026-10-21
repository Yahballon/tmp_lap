// SPDX-License-Identifier: GPL-3.0-or-later
// Read-only inventory bridge for an existing PS5 elfldr. Contains no deletion,
// write, mount, install, kernel-access or network-listener functionality.
// stdout is sent back through the existing 127.0.0.1:9021 ELF loader channel.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_RECORDS 550
#define MAX_VISITS 3200
#define MAX_DEPTH 2
static unsigned records=0, visits=0;

static int safe(const char *s) {
    if (!s||!*s||s[0]=='.'||strlen(s)>160 || !strcmp(s,".."))return 0;
    for(const unsigned char *p=(const unsigned char*)s;*p;p++)
        if(*p<32||*p==127||*p=='/'||*p==92)return 0;
    return 1;
}
static int stat_regular(const char *p,off_t *size) {
    struct stat st;
    if(lstat(p,&st)!=0||!S_ISREG(st.st_mode))return 0;
    if(size)*size=st.st_size;
    return 1;
}
static int stat_directory(const char *p) {
    struct stat st;
    return lstat(p,&st)==0&&S_ISDIR(st.st_mode);
}
static int package(const char *name) {
    const size_t n=strlen(name);
    return n>4&&(!strcmp(name+n-4,".pkg")||!strcmp(name+n-4,".PKG"));
}
static void status(const char *root,int code,unsigned visited,unsigned found) {
    printf("R %d %u %u %zu\n",code,visited,found,strlen(root));
    fwrite(root,1,strlen(root),stdout);
}
static int join(char *dst,size_t limit,const char *root,const char *name) {
    const int n=snprintf(dst,limit,"%s/%s",root,name);
    return n>0&&(size_t)n<limit;
}
// ITEM binary payload contains path, folder name, optional param.json bytes.
// Size is valid only for raw .pkg archives (kind 'P').
static void item(char kind,const char *path,const char *name,const char *meta,
                 size_t meta_len,unsigned long long size) {
    if(records>=MAX_RECORDS)return;
    const size_t pl=strlen(path), nl=strlen(name);
    if(pl==0||pl>900||nl==0||nl>160||meta_len>8192)return;
    printf("I %c %zu %zu %zu %llu\n",kind,pl,nl,meta_len,size);
    fwrite(path,1,pl,stdout);
    fwrite(name,1,nl,stdout);
    if(meta_len)fwrite(meta,1,meta_len,stdout);
    records++;
}
static int read_json(const char *path,char *dest,size_t limit,size_t *out){
    *out=0;
    FILE* f=fopen(path,"rb");
    if(!f)return 0;
    *out=fread(dest,1,limit,f);
    if(ferror(f)){fclose(f);*out=0;return 0;}
    fclose(f);
    return 1;
}
static int app_folder(const char *path,const char *folder,char kind){
    char metaPath[1050],elf[1050],sfo[1050];
    if(!join(metaPath,sizeof(metaPath),path,"sce_sys/param.json") ||
       !join(elf,sizeof(elf),path,"eboot.bin") ||
       !join(sfo,sizeof(sfo),path,"sce_sys/param.sfo"))return 0;
    const int hasJson=stat_regular(metaPath,NULL);
    if(!hasJson && !stat_regular(elf,NULL) && !stat_regular(sfo,NULL))return 0;
    char metadata[8192];size_t n=0;
    if(hasJson)read_json(metaPath,metadata,sizeof(metadata),&n);
    // Filesystem discovery only. Nothing here is an uninstall authorization.
    item(kind,path,folder,metadata,n,0);
    return 1;
}
static void walk(const char* path,char kind,unsigned depth,int pkgs,
                 unsigned *localVisits,unsigned *localFound){
    if(records>=MAX_RECORDS||visits>=MAX_VISITS)return;
    DIR* dir=opendir(path);
    if(!dir){status(path,errno,*localVisits,*localFound);return;}
    struct dirent* entry;
    errno=0;
    while((entry=readdir(dir)) && visits<MAX_VISITS&&records<MAX_RECORDS){
        const char* name=entry->d_name;
        if(!safe(name))continue;
        char child[1024];if(!join(child,sizeof(child),path,name))continue;
        visits++;(*localVisits)++;
        struct stat st;
        if(lstat(child,&st)!=0||S_ISLNK(st.st_mode))continue;
        if(S_ISREG(st.st_mode)){
            if(pkgs&&package(name)){
                item('P',child,name,NULL,0,(unsigned long long)st.st_size);
                (*localFound)++;
            }
            continue;
        }
        if(!S_ISDIR(st.st_mode))continue;
        if(!pkgs){
            const unsigned before=records;
            app_folder(child,name,kind);
            if(records>before){(*localFound)++;continue;}
        }
        if(depth>=MAX_DEPTH)continue;
        if(!strcmp(name,"sce_sys") || !strcmp(name,"sce_module") ||
           !strcmp(name,"system_data") || !strcmp(name,"user") ||
           !strcmp(name,"trophy") || !strcmp(name,"patch") ||
           (pkgs && (!strcmp(name,"homebrew")||!strcmp(name,"etaHEN"))))
            continue;
        walk(child,kind,depth+1,pkgs,localVisits,localFound);
    }
    const int error=errno;
    closedir(dir);
    if(error)status(path,error,*localVisits,*localFound);
}
static void scan_root(const char* path,char kind,int pkgs) {
    unsigned visited=0,found=0;
    DIR* d=opendir(path);
    if(!d){status(path,errno,0,0);return;}
    closedir(d);
    walk(path,kind,0,pkgs,&visited,&found);
    status(path,0,visited,found);
}
int main(void){
    setvbuf(stdout,NULL,_IONBF,0);
    puts("UNSCN1");
    // Keep in sync with /homebrew/uninstaller/native/inventory.hpp.
    scan_root("/data/homebrew",'H',0);
    scan_root("/user/app",'G',0);
    scan_root("/system_ex/app",'G',0);
    scan_root("/data/etaHEN/games",'G',0);
    scan_root("/user/data/etaHEN/games",'G',0);
    scan_root("/data/games",'G',0);
    for(int i=0;i<8;i++){
        char p[96],base[40];snprintf(base,sizeof(base),"/mnt/usb%d",i);
        snprintf(p,sizeof(p),"%s/homebrew",base);scan_root(p,'H',0);
        snprintf(p,sizeof(p),"%s/etaHEN/games",base);scan_root(p,'G',0);
        scan_root(base,'P',1);
    }
    for(int i=0;i<4;i++){
        char p[96],base[40];snprintf(base,sizeof(base),"/mnt/ext%d",i);
        snprintf(p,sizeof(p),"%s/homebrew",base);scan_root(p,'H',0);
        snprintf(p,sizeof(p),"%s/etaHEN/games",base);scan_root(p,'G',0);
        snprintf(p,sizeof(p),"%s/user/app",base);scan_root(p,'G',0);
        scan_root(base,'P',1);
    }
    printf("DONE %u %u\n",records,visits);
    return 0;
}
