// SPDX-License-Identifier: GPL-3.0-or-later
// Versioned, bounded, atomic pin settings. Never writes into the program assets.
#include "pin_settings.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace tracker {
static const char* kMain = "/download0/crypto-tracker.json";
static const char* kFallback = "/data/crypto-tracker/settings.json";

bool PinSettings::valid_id(const std::string& id) {
    if(id.empty() || id.size()>72)return false;
    for(unsigned char c:id)if(!std::isalnum(c) && c!='-')return false;
    return true;
}
bool PinSettings::contains(const std::string& id) const {
    return std::find(ids.begin(),ids.end(),id)!=ids.end();
}
bool PinSettings::toggle(const std::string& id) {
    if(!valid_id(id))return false;
    const auto found=std::find(ids.begin(),ids.end(),id);
    if(found!=ids.end()){ids.erase(found);return true;}
    if(ids.size()>=100)return false;
    ids.insert(ids.begin(),id);
    return true;
}
bool PinSettings::save_to(const std::string& path) const {
    std::string data="{\"schema\":1,\"pinned\":[";
    for(size_t i=0;i<ids.size();++i){
        if(!valid_id(ids[i]))return false;
        if(i)data+=',';
        data+='"'+ids[i]+'"';
    }
    data+="]}\n";
    if(data.size()>8192)return false;
    const std::string tmp=path+".tmp";
    FILE* f=std::fopen(tmp.c_str(),"wb");
    if(!f)return false;
    const bool ok=std::fwrite(data.data(),1,data.size(),f)==data.size()
        && std::fflush(f)==0 && ::fsync(::fileno(f))==0;
    const bool closed=(std::fclose(f)==0);
    if(!ok||!closed){::unlink(tmp.c_str());return false;}
    if(::rename(tmp.c_str(),path.c_str())!=0){::unlink(tmp.c_str());return false;}
    return true;
}
bool PinSettings::load_from(const std::string& path) {
    FILE* f=std::fopen(path.c_str(),"rb");
    if(!f)return false;
    char buf[8193]{};
    const size_t n=std::fread(buf,1,8193,f);
    std::fclose(f);
    if(n==0||n>8192)return false;
    std::string s(buf,n);
    if(s.find("\"schema\":1")==std::string::npos &&
       s.find("\"schema\" : 1")==std::string::npos)return false;
    size_t p=s.find("\"pinned\"");
    if(p==std::string::npos)return false;
    p=s.find('[',p);
    if(p==std::string::npos)return false;
    const size_t close=s.find(']',p);
    if(close==std::string::npos)return false;
    std::vector<std::string> next;
    ++p;
    while(p<close){
        while(p<close&&(s[p]==' '||s[p]=='\n'||s[p]=='\t'||s[p]==','))++p;
        if(p==close)break;
        if(s[p]!='"')return false;
        size_t e=s.find('"',++p);
        if(e==std::string::npos||e>close)return false;
        std::string id=s.substr(p,e-p);
        if(!valid_id(id))return false;
        if(std::find(next.begin(),next.end(),id)==next.end())next.push_back(id);
        if(next.size()>100)return false;
        p=e+1;
    }
    ids=std::move(next);loaded=true;return true;
}
bool PinSettings::load() {
    if(load_from(kMain)){persisted=true;return true;}
    if(load_from(kFallback)){persisted=true;return true;}
    return false;
}
bool PinSettings::save() {
    if(save_to(kMain)){persisted=true;return true;}
    ::mkdir("/data/crypto-tracker",0700);
    persisted=save_to(kFallback);
    return persisted;
}
} // namespace tracker
