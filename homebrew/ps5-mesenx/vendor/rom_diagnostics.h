// PS5MesenX debug logger. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
namespace mesen_diag {
inline void write(const char* fmt,...) {
    char line[1024];
    va_list args;
    va_start(args,fmt);
    int n=std::vsnprintf(line,sizeof(line)-2,fmt,args);
    va_end(args);
    if(n<0)return;
    size_t len=static_cast<size_t>(n);
    if(len>sizeof(line)-2)len=sizeof(line)-2;
    line[len++]='\n';line[len]=0;
    constexpr const char* roots[]={"/data/PS5MesenX","/download0/PS5MesenX"};
    for(const char* root:roots) {
        mkdir(root,0777);
        char dir[256],file[320];
        std::snprintf(dir,sizeof(dir),"%s/logs",root);
        mkdir(dir,0777);
        std::snprintf(file,sizeof(file),"%s/boot.log",dir);
        int fd=open(file,O_WRONLY|O_CREAT|O_APPEND,0666);
        if(fd>=0) {
            (void)::write(fd,line,len);
            (void)fsync(fd);
            close(fd);
            return;
        }
    }
    // One final destination in the installed application directory.
    int fd=open("/data/homebrew/PPSA99896/mesenx-debug.log",O_WRONLY|O_CREAT|O_APPEND,0666);
    if(fd>=0){(void)::write(fd,line,len);close(fd);}
}
}
