// SPDX-License-Identifier: GPL-3.0-or-later
// One-shot, read-only USB catalog for the existing PS5 ELF loader.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#define ROOT "/mnt/usb0/payloads"
static int safe(const char *s) {
  if(!s || !*s || s[0]=='.' || strlen(s)>127) return 0;
  for(const unsigned char *c=(const unsigned char*)s;*c;c++)
    if(*c<32 || *c==127 || *c=='/' || *c==':' || *c==92) return 0;
  return 1;
}
static int file_ok(const char *path) {
  struct stat s;
  return !lstat(path,&s) && S_ISREG(s.st_mode);
}
static int dir_ok(const char *path) {
  struct stat s;
  return !lstat(path,&s) && S_ISDIR(s.st_mode);
}
static int executable(const char *s) {
  size_t n=strlen(s);
  return n>4 && (!strcmp(s+n-4,".elf") || !strcmp(s+n-4,".efl") || !strcmp(s+n-4,".bin"));
}
static unsigned char *file_read(const char *path,size_t max,size_t *size) {
  *size=0;
  FILE *f=fopen(path,"rb");
  if(!f)return NULL;
  if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}
  long n=ftell(f);rewind(f);
  if(n<0||(unsigned long)n>max){fclose(f);return NULL;}
  unsigned char *s=(unsigned char *)malloc((size_t)n+1);
  if(!s){fclose(f);return NULL;}
  if(fread(s,1,(size_t)n,f)!=(size_t)n){free(s);fclose(f);return NULL;}
  fclose(f);s[n]=0;*size=(size_t)n;return s;
}
int main(void) {
  setvbuf(stdout,NULL,_IONBF,0);
  puts("PLUSB2");
  DIR *root=opendir(ROOT);
  if(!root){puts("ERROR USB directory is inaccessible");return 1;}
  struct dirent *item;
  unsigned count=0;
  while((item=readdir(root)) && count<128) {
    if(!safe(item->d_name))continue;
    char folder[512],path[640],executable_name[128]={0};
    if(snprintf(folder,sizeof(folder),"%s/%s",ROOT,item->d_name)>=(int)sizeof(folder) || !dir_ok(folder))continue;
    DIR *sub=opendir(folder);
    if(!sub)continue;
    struct dirent *e;
    while((e=readdir(sub))) {
      if(!safe(e->d_name)||!executable(e->d_name))continue;
      if(snprintf(path,sizeof(path),"%s/%s",folder,e->d_name)>=(int)sizeof(path))continue;
      if(file_ok(path) && (!*executable_name||strcmp(e->d_name,executable_name)<0))
        strcpy(executable_name,e->d_name);
    }
    closedir(sub);
    if(!*executable_name)continue;
    snprintf(path,sizeof(path),"%s/payload.json",folder);
    size_t js=0,ps=0;
    unsigned char *json=file_read(path,16384,&js);
    snprintf(path,sizeof(path),"%s/payload.png",folder);
    unsigned char *png=file_read(path,1048576,&ps);
    printf("ITEM %zu %zu %zu %zu\n",strlen(item->d_name),strlen(executable_name),js,ps);
    fwrite(item->d_name,1,strlen(item->d_name),stdout);
    fwrite(executable_name,1,strlen(executable_name),stdout);
    if(js)fwrite(json,1,js,stdout);
    if(ps)fwrite(png,1,ps,stdout);
    free(json);free(png);count++;
  }
  closedir(root);
  puts("DONE");
  return 0;
}
