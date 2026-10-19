// SPDX-License-Identifier: GPL-3.0-or-later
#include "diagnostics.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <limits>
#include <sys/stat.h>
#include <sys/statvfs.h>
namespace meshps5 {
namespace {
bool directory(const std::string& path) {
 struct stat st{};
 return lstat(path.c_str(),&st)==0&&S_ISDIR(st.st_mode);
}
}
std::string human_bytes(std::uint64_t b) {
 const char* units[]={"B","KiB","MiB","GiB","TiB","PiB","EiB"};
 double value=static_cast<double>(b);int unit=0;
 while(value>=1024&&unit<6){value/=1024;unit++;}
 char text[80];
 std::snprintf(text,sizeof(text),"%.2f %s",value,units[unit]);
 return text;
}
Disk disk(const std::string& name,const std::string& path) {
 Disk d;d.name=name;d.path=path;
 if(!directory(path))return d;
 d.mounted=true;
 struct statvfs v{};
 if(statvfs(path.c_str(),&v)!=0||v.f_frsize==0)return d;
 // A POSIX mount capacity report is NOT proof the storage is dedicated to
 // MeshCloud or remotely auditable. It is only an initial local preflight.
 const auto blocks=static_cast<std::uint64_t>(v.f_bavail);
 const auto size=static_cast<std::uint64_t>(v.f_frsize);
 if(blocks>std::numeric_limits<std::uint64_t>::max()/size)return d;
 d.space_reported=true;
 d.available_bytes=blocks*size;
 d.possible_local_preflight=d.available_bytes>=minimum_contribution+safety_headroom;
 return d;
}
std::vector<File> list_regular_files(const std::string& folder,std::size_t cap) {
 std::vector<File> files;
 if(cap==0||cap>1500||!directory(folder))return files;
 DIR* root=opendir(folder.c_str());
 if(!root)return files;
 struct dirent* e=nullptr;
 while((e=readdir(root)) && files.size()<cap) {
  const std::string name=e->d_name;
  if(name=="."||name==".."||name.size()>192||name.find('/')!=std::string::npos ||
     name.find('\\')!=std::string::npos)continue;
  const auto path=folder+"/"+name;
  struct stat info{};
  if(lstat(path.c_str(),&info)!=0||!S_ISREG(info.st_mode)||info.st_size<0)continue;
  files.push_back({name,static_cast<std::uint64_t>(info.st_size)});
 }
 closedir(root);
 std::sort(files.begin(),files.end(),[](const File& a,const File& b){
  return a.name<b.name;
 });
 return files;
}
Snapshot collect(int active) {
 Snapshot s;
 s.disks.push_back(disk("Internal data","/data"));
 s.disks.push_back(disk("USB 0","/mnt/usb0"));
 s.disks.push_back(disk("USB 1","/mnt/usb1"));
 const int index=active>=0&&active<3?active:0;
 const auto& path=s.disks[static_cast<std::size_t>(index)].path;
 s.file_folder=path+"/PS5MeshCloud/files";
 s.files=list_regular_files(s.file_folder);
 if(!s.disks[static_cast<std::size_t>(index)].mounted)
  s.error="Selected storage mount is not available";
 else if(!s.disks[static_cast<std::size_t>(index)].space_reported)
  s.error="Free capacity could not be verified on this filesystem";
 else if(!s.disks[static_cast<std::size_t>(index)].possible_local_preflight)
  s.error="Not enough reported free space for 10 GiB plus 2 GiB margin";
 else
  s.error="Disk passes local preflight; network membership is still LOCKED";
 return s;
}
} // namespace meshps5
