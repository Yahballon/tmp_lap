// SPDX-License-Identifier: GPL-3.0-or-later
// PS5 MicroPython v0.2.1: POSIX-compatible script discovery without stat-only gate.
#pragma once
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace pyscan {
constexpr size_t kMaxScriptBytes=64*1024;
constexpr size_t kMaxUserScripts=64;
inline bool valid_name(const std::string& name) {
 if(name.size()<=3 || name.size()>96 || name[0]=='.' ||
    name.compare(name.size()-3,3,".py")!=0) return false;
 for(unsigned char c:name) {
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
       (c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return false;
 }
 return true;
}
struct File {std::string name,path;};
struct Report {
 std::vector<File> files;
 unsigned seen=0,candidates=0,empty=0,oversize=0,unreadable=0,unsafe=0;
 int directory_errno=0,last_file_errno=0;
 bool folder_opened=false;
 std::string status() const {
  if(!folder_opened)
   return "SCAN ERROR: folder inaccessible (errno "+std::to_string(directory_errno)+")";
  return "SCAN: "+std::to_string(files.size())+" local, "+std::to_string(candidates)+
   " .py; empty "+std::to_string(empty)+", big "+std::to_string(oversize)+
   ", unreadable "+std::to_string(unreadable)+
   (last_file_errno?", errno "+std::to_string(last_file_errno):"");
 }
};
enum class Probe {ok,empty,oversize,unreadable,unsafe};
// Do not gate discovery on stat() success or st_mode/st_size: on some
// homebrew POSIX layers the structure is unreliable even when fopen works.
// We still reject symlinks when lstat() is usable, and bound all reads.
inline Probe probe(const std::string& path,size_t* size_out=nullptr) {
 struct stat st{};
 if(lstat(path.c_str(),&st)==0 &&
    (S_ISLNK(st.st_mode)||S_ISDIR(st.st_mode)))return Probe::unsafe;
 FILE* f=std::fopen(path.c_str(),"rb");
 if(!f)return Probe::unreadable;
 char buffer[4096];
 size_t total=0;
 bool error=false;
 while(total<=kMaxScriptBytes) {
  const size_t request=sizeof(buffer) < (kMaxScriptBytes+1-total)
          ?sizeof(buffer):(kMaxScriptBytes+1-total);
  if(request==0)break;
  const size_t n=std::fread(buffer,1,request,f);
  total+=n;
  if(n<request){
   if(std::ferror(f))error=true;
   break;
  }
 }
 const int result=std::fclose(f);
 if(error||result!=0)return Probe::unreadable;
 if(total==0)return Probe::empty;
 if(total>kMaxScriptBytes)return Probe::oversize;
 if(size_out)*size_out=total;
 return Probe::ok;
}
inline Report scan(const std::string& folder) {
 Report report;
 errno=0;
 DIR* d=opendir(folder.c_str());
 if(!d){report.directory_errno=errno;return report;}
 report.folder_opened=true;
 errno=0;
 while(dirent* item=readdir(d)) {
  ++report.seen;
  const std::string name=item->d_name;
  if(!valid_name(name))continue;
  ++report.candidates;
  if(report.files.size()>=kMaxUserScripts)break;
#ifdef DT_LNK
  if(item->d_type==DT_LNK){++report.unsafe;continue;}
#endif
  const auto path=folder+"/"+name;
  size_t size=0;
  const Probe result=probe(path,&size);
  switch(result){
   case Probe::ok:report.files.push_back({name,path});break;
   case Probe::empty:++report.empty;break;
   case Probe::oversize:++report.oversize;break;
   case Probe::unsafe:++report.unsafe;break;
   case Probe::unreadable:
    ++report.unreadable;
    report.last_file_errno=errno;
    break;
  }
  errno=0;
 }
 if(errno!=0)report.last_file_errno=errno;
 closedir(d);
 return report;
}
// Read through fread() rather than ftell/fseek; some target libc shims
// expose a readable file but cannot report its size reliably.
inline std::string read_script(const std::string& path,size_t limit) {
 FILE* f=std::fopen(path.c_str(),"rb");
 if(!f)return {};
 std::string data;
 data.reserve(4096);
 char block[4096];
 bool failed=false;
 while(data.size()<=limit){
  const size_t want=sizeof(block) < limit+1-data.size()?
      sizeof(block):limit+1-data.size();
  if(want==0)break;
  const size_t read=std::fread(block,1,want,f);
  data.append(block,read);
  if(read<want){
   if(std::ferror(f))failed=true;
   break;
  }
 }
 std::fclose(f);
 if(failed||data.empty()||data.size()>limit)return {};
 return data;
}
}
