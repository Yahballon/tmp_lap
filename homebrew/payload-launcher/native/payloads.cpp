// SPDX-License-Identifier: GPL-3.0-or-later
#include "payloads.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <map>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
namespace launcher {
namespace {
constexpr std::size_t kMaxEntries = 512;
constexpr std::size_t kMaxJson = 16 * 1024;
bool regular_file(const std::string& p) {
    struct stat st{};
    return lstat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}
bool directory(const std::string& p) {
    struct stat st{};
    return lstat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}
bool suffix(std::string_view s, std::string_view end) {
    return s.size() >= end.size() &&
        std::equal(end.rbegin(), end.rend(), s.rbegin(), [](char a,char b) {
            return std::tolower((unsigned char)a) == std::tolower((unsigned char)b);
        });
}
bool executable_extension(const std::string& s) {
    return suffix(s, ".elf") || suffix(s, ".efl") || suffix(s, ".bin");
}
std::string trim(const std::string& s, std::size_t maxsize) {
    std::size_t b = s.find_first_not_of(" \r\n\t");
    if (b == std::string::npos) return {};
    std::size_t e = s.find_last_not_of(" \r\n\t");
    std::string r = s.substr(b, e - b + 1);
    if (r.size() > maxsize) r.resize(maxsize);
    return r;
}
// Strictly parse top-level JSON object with string values. Unknown values are
// skipped, never evaluated; malformed metadata falls back to folder labels.
class Parser {
public:
    explicit Parser(std::string_view s) : s_(s) {}
    std::map<std::string,std::string> fields() {
        std::map<std::string,std::string> out;
        ws(); if (!take('{')) return {};
        ws(); if (take('}')) return out;
        for (int n = 0; n < 96; ++n) {
            std::string key, value; if (!quoted(key)) return {};
            ws(); if (!take(':')) return {};
            ws(); if (!quoted(value)) {
                // Unknown non-string scalar/object is ignored safely.
                if (!skip()) return {};
            } else { out[key] = value; }
            ws(); if (take('}')) return out;
            if (!take(',')) return {};
        }
        return {};
    }
private:
    std::string_view s_; std::size_t p_ = 0;
    void ws(){while(p_<s_.size()&&(s_[p_]==' '||s_[p_]=='\t'||s_[p_]=='\r'||s_[p_]=='\n'))++p_;}
    bool take(char c){ws(); if(p_<s_.size()&&s_[p_]==c){++p_;return true;}return false;}
    static void unicode(std::string& o, unsigned cp){
        if(cp<0x80)o.push_back((char)cp);
        else if(cp<0x800){o.push_back((char)(0xc0|(cp>>6)));o.push_back((char)(0x80|(cp&63)));}
        else {o.push_back((char)(0xe0|(cp>>12)));o.push_back((char)(0x80|((cp>>6)&63)));o.push_back((char)(0x80|(cp&63)));}
    }
    bool quoted(std::string& o) {
        ws();if(p_>=s_.size()||s_[p_]!='"')return false;
        ++p_;
        while(p_<s_.size()) {
            unsigned char c=(unsigned char)s_[p_++];
            if(c=='"')return true;
            if(c<32)return false;
            if(c=='\\') {
                if(p_>=s_.size())return false;
                char esc=s_[p_++];
                if(esc=='n')c='\n'; else if(esc=='r')c='\r'; else if(esc=='t')c='\t';
                else if(esc=='"'||esc=='\\'||esc=='/')c=(unsigned char)esc;
                else if(esc=='u') {
                    if(p_+4>s_.size())return false;
                    unsigned n=0;for(int i=0;i<4;++i){char h=s_[p_++];unsigned d= h>='0'&&h<='9'?h-'0':h>='a'&&h<='f'?h-'a'+10:h>='A'&&h<='F'?h-'A'+10:16;if(d>15)return false;n=n*16+d;}
                    if(n>=0xd800&&n<=0xdfff)return false;
                    if(o.size()<4096)unicode(o,n);
                    continue;
                } else return false;
            }
            if(o.size()<4096)o.push_back((char)c);
        }
        return false;
    }
    bool skip(){
        ws();if(p_>=s_.size())return false;
        if(s_[p_]=='"'){std::string ignore;return quoted(ignore);}
        if(s_[p_]=='{'||s_[p_]=='['){char start=s_[p_++];char end=start=='{'?'}':']';int depth=1;bool in=false,esc=false;while(p_<s_.size()&&depth){char c=s_[p_++];if(in){if(esc)esc=false;else if(c=='\\')esc=true;else if(c=='"')in=false;}else if(c=='"')in=true;else if(c==start)++depth;else if(c==end)--depth;}return depth==0;}
        std::size_t begin=p_;while(p_<s_.size()&&s_[p_]!=','&&s_[p_]!='}'&&!std::isspace((unsigned char)s_[p_]))++p_;return p_>begin;
    }
};
std::map<std::string,std::string> read_meta(const std::string& path){
    FILE* f=fopen(path.c_str(),"rb");if(!f)return {};
    if(fseek(f,0,SEEK_END)){fclose(f);return {};}
    long size=ftell(f);rewind(f);
    if(size<2 || size>(long)kMaxJson){fclose(f);return {};}
    std::string s((std::size_t)size,'\0');
    bool good=fread(s.data(),1,s.size(),f)==s.size();fclose(f);
    return good?Parser(s).fields():std::map<std::string,std::string>{};
}
std::string get(const std::map<std::string,std::string>& m,const char* key,std::size_t limit=180){
    auto it=m.find(key);return it==m.end()?std::string():trim(it->second,limit);
}
} // namespace
bool is_safe_filename(const std::string& s) {
    if(s.empty() || s.size()>127 || s=="." || s==".." || s[0]=='.')return false;
    for(unsigned char c : s){if(c=='/' || c=='\\' || c==':' || c<32 || c==127)return false;}
    return true;
}
bool is_elf_file(const std::string& path){
    FILE* f=fopen(path.c_str(),"rb");if(!f)return false;
    unsigned char h[5]{};std::size_t n=fread(h,1,sizeof(h),f);fclose(f);
    return n==5 && h[0]==0x7f && h[1]=='E' && h[2]=='L' && h[3]=='F' && h[4]==2;
}
ScanResult scan(const std::string& root){
    ScanResult result;
    if(!directory(root)) {result.message="USB folder missing: " + root;return result;}
    DIR* d=opendir(root.c_str());if(!d){result.message="Cannot read USB payload folder";return result;}
    std::vector<std::string> dirs;
    while(auto* ent=readdir(d)){
        std::string name(ent->d_name);
        if(!is_safe_filename(name))continue;
        if(directory(root+"/"+name))dirs.push_back(name);
        if(dirs.size()>=kMaxEntries)break;
    }
    closedir(d);std::sort(dirs.begin(),dirs.end());
    for(const auto& folder:dirs){
        const std::string dir=root+"/"+folder;
        auto metadata=read_meta(dir+"/payload.json");
        std::string exe=get(metadata,"file",127);
        if(exe.empty())exe=get(metadata,"payload",127);
        if(!is_safe_filename(exe)||!executable_extension(exe)||!regular_file(dir+"/"+exe))exe.clear();
        if(exe.empty()){
            // Prefer a filename that matches the containing folder.
            for(auto ext:{".elf",".efl",".bin"}){
                std::string candidate=folder+ext;
                if(regular_file(dir+"/"+candidate)){exe=candidate;break;}
            }
        }
        if(exe.empty()){
            DIR* subs=opendir(dir.c_str());if(subs){std::vector<std::string> candidates;while(auto* e=readdir(subs)){
                std::string candidate=e->d_name;
                if(is_safe_filename(candidate)&&executable_extension(candidate)&&regular_file(dir+"/"+candidate))candidates.push_back(candidate);
            }closedir(subs);std::sort(candidates.begin(),candidates.end());if(!candidates.empty())exe=candidates.front();}
        }
        if(exe.empty())continue;
        Payload p;p.folder=folder;p.filename=exe;p.path=dir+"/"+exe;
        p.name=get(metadata,"name",64);if(p.name.empty())p.name=folder;
        p.version=get(metadata,"version",20);if(p.version.empty())p.version="Unversioned";
        p.description=get(metadata,"description",240);if(p.description.empty())p.description="Local USB payload";
        p.author=get(metadata,"author",60);if(p.author.empty())p.author="Unknown author";
        if(regular_file(dir+"/payload.png"))p.icon=dir+"/payload.png";
        result.payloads.push_back(std::move(p));
    }
    result.message=result.payloads.empty()?"No payloads found. Add a subfolder with a .elf and payload.json.":
                   "Loaded "+std::to_string(result.payloads.size())+" payload(s) from USB";
    return result;
}
std::string request_launch(const Payload& p,int port){
    if(p.path.rfind(std::string(kUsbRoot)+"/",0)!=0 || !regular_file(p.path) || !is_elf_file(p.path))
        return "Invalid payload: expected an ELF64 executable on USB";
    if(port<1 || port>65535)return "Invalid loader port";
    int fd=socket(AF_INET,SOCK_STREAM,0);
    if(fd<0)return "Unable to create local socket";
    struct timeval tv{2,0};
    setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
    setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
    struct sockaddr_in endpoint{};endpoint.sin_family=AF_INET;endpoint.sin_port=htons((uint16_t)port);
    endpoint.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if(connect(fd,(const struct sockaddr*)&endpoint,sizeof(endpoint))!=0){close(fd);return "No local ELF loader on 127.0.0.1:"+std::to_string(port);}
    const std::string command="file:"+p.path+"\n";
    std::size_t sent=0;
    while(sent<command.size()){
        ssize_t n=send(fd,command.data()+sent,command.size()-sent,0);
        if(n<=0){close(fd);return "Loader connection failed while sending request";}
        sent+=(std::size_t)n;
    }
    shutdown(fd,SHUT_WR);close(fd);
    return "Submitted to ELF loader: " + p.name + " (check payload result)";
}
} // namespace launcher
